"""The esl_grep Textual application."""

from __future__ import annotations

import asyncio
import re
from collections.abc import Coroutine
from datetime import datetime
from pathlib import Path
from typing import Any

from rich.text import Text
from textual import on
from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Horizontal, Vertical
from textual.widgets import (
    Button,
    DataTable,
    Footer,
    Input,
    Static,
    TextArea,
)
from textual.widgets.text_area import Selection

from .capture import JSON_FORMAT, JSONL_FORMAT, RAW_FORMAT, write_capture
from .config import EslConfig, load_config, save_config
from .esl import EslClient, EslEvent
from .events import is_callcenter_event
from .screens import (
    ConfirmScreen,
    CopyJsonScreen,
    SaveCaptureScreen,
    SettingsScreen,
)
from .theme import DETAIL_THEME_NAME, build_detail_theme

JSON_KEY_RE = re.compile(r'^\s*"([^"]+)"\s*:')


class ESLGrepApp(App[None]):
    """Browse and filter FreeSWITCH ESL events in a terminal."""

    TITLE = "ESL Grep"
    SUB_TITLE = "FreeSWITCH ESL event viewer"
    CSS_PATH = "app.tcss"

    BINDINGS = [
        Binding("ctrl+s", "save_capture", "Save"),
        Binding("ctrl+r", "toggle_stream", "Start/Stop"),
        Binding("ctrl+f", "open_filter", "Filter"),
        Binding("ctrl+y", "copy_detail", "Copy JSON", priority=True),
        Binding("ctrl+l", "clear_events", "Clear"),
        Binding("ctrl+q", "quit", "Quit"),
    ]

    def __init__(self) -> None:
        super().__init__()
        self.config = load_config()
        self.events: list[EslEvent] = []

        self._active_config: EslConfig | None = None
        self._state = "stopped"
        self._status_detail = ""
        self._run_token = 0
        self._stream_task: asyncio.Task[None] | None = None
        self._client: EslClient | None = None
        self._lock: asyncio.Lock | None = None
        self._background_tasks: set[asyncio.Task[None]] = set()
        self._save_snapshot: list[EslEvent] | None = None
        self._selected_event: EslEvent | None = None

    def compose(self) -> ComposeResult:
        with Vertical(id="toolbar"):
            with Horizontal(id="titlebar"):
                yield Static("ESL Grep", id="app-title")
                yield Static("○ Stopped", id="status", classes="stopped")
            with Horizontal(id="actions"):
                yield Button(
                    "Start",
                    id="start",
                    variant="success",
                    tooltip=(
                        "Start receiving FreeSWITCH ESL events using the "
                        "current settings."
                    ),
                )
                yield Button("Stop", id="stop", disabled=True)
                yield Button(
                    "Filter",
                    id="filter",
                    tooltip=(
                        "Choose which events to receive. "
                        "Changes apply the next time you start."
                    ),
                )
                yield Button(
                    "Settings",
                    id="settings",
                    tooltip="Configure the ESL server connection.",
                )
                yield Button(
                    "Save",
                    id="save",
                    disabled=True,
                    tooltip="Save the events received so far.",
                )
                yield Button("Clear", id="clear", disabled=True)

        with Horizontal(id="main"):
            with Vertical(id="event-pane", classes="pane"):
                with Horizontal(classes="pane-heading"):
                    yield Static("Received Events", classes="pane-title")
                    yield Static("0", id="event-count", classes="pane-count")
                yield DataTable(
                    id="event-table",
                    cursor_type="row",
                    zebra_stripes=True,
                )

            with Vertical(id="detail-pane", classes="pane"):
                with Horizontal(classes="pane-heading"):
                    yield Static("Event Detail", classes="pane-title")
                    yield Static("", id="detail-size", classes="pane-meta")
                    yield Button(
                        "Copy",
                        id="copy-detail",
                        compact=True,
                        tooltip=(
                            "Open the event JSON on a full screen. "
                            "Use Shift+mouse drag to select and copy just "
                            "the JSON."
                        ),
                    )
                yield Static(
                    "Select an event from the list on the left.",
                    id="detail-meta",
                    markup=False,
                )
                with Horizontal(id="detail-search"):
                    yield Input(
                        placeholder="Search key, then Enter (e.g. CC-Action)",
                        id="detail-key",
                    )
                    yield Static(
                        "",
                        id="detail-search-status",
                        classes="pane-meta",
                    )
                yield TextArea.code_editor(
                    "",
                    language="json",
                    theme="monokai",
                    read_only=True,
                    show_line_numbers=False,
                    show_cursor=True,
                    soft_wrap=True,
                    id="detail-body",
                )

        yield Footer()

    def on_mount(self) -> None:
        self._lock = asyncio.Lock()

        table = self.query_one("#event-table", DataTable)
        table.add_column("#", width=6)
        table.add_column("Time", width=13)
        table.add_column("Event", width=30)
        table.add_column("CC-Action", width=26)

        detail = self.query_one("#detail-body", TextArea)
        detail.register_theme(build_detail_theme())
        detail.theme = DETAIL_THEME_NAME

        self._show_placeholder()
        self._refresh_status()
        self._refresh_controls()

    async def on_unmount(self) -> None:
        for task in list(self._background_tasks):
            task.cancel()

        # The widget tree is already being torn down here, so avoid
        # _stop_stream(), which refreshes the status bar.
        self._run_token += 1
        task = self._stream_task
        self._stream_task = None
        client = self._client
        self._client = None

        if task is not None:
            task.cancel()
            try:
                await task
            except asyncio.CancelledError:
                pass
            except Exception:
                pass
        elif client is not None:
            await client.close()

    async def on_button_pressed(self, event: Button.Pressed) -> None:
        button_id = event.button.id
        if button_id == "start":
            await self._start_stream()
        elif button_id == "stop":
            await self._stop_stream()
        elif button_id == "filter":
            self._open_filter()
        elif button_id == "settings":
            self._open_settings()
        elif button_id == "save":
            self.action_save_capture()
        elif button_id == "copy-detail":
            self.action_copy_detail()
        elif button_id == "clear":
            self.action_clear_events()

    async def action_toggle_stream(self) -> None:
        if self._state in {"connecting", "streaming"}:
            await self._stop_stream()
        else:
            await self._start_stream()

    def action_open_filter(self) -> None:
        self._open_filter()

    def action_copy_detail(self) -> None:
        if len(self.screen_stack) > 1:
            # Another modal (settings, save, copy view) is already open.
            return

        event = self._selected_event
        body = self.query_one("#detail-body", TextArea).text
        if event is None or not body.strip():
            self.notify(
                "There is no event to copy. Select an event from the list "
                "on the left.",
                severity="warning",
            )
            return

        subtitle = event.display_name
        if event.raw:
            subtitle += "  ·  {} bytes".format(len(event.raw))
        self.push_screen(CopyJsonScreen(body, subtitle=subtitle))

    def action_save_capture(self) -> None:
        if not self.events:
            self.notify(
                "There are no events to save. Start receiving events first.",
                severity="warning",
            )
            return

        self._save_snapshot = list(self.events)
        default_path = Path.cwd() / "esl_capture_{}.json".format(
            datetime.now().strftime("%Y%m%d_%H%M%S")
        )
        self.push_screen(
            SaveCaptureScreen(
                default_path=default_path,
                event_count=len(self._save_snapshot),
            ),
            self._save_destination_chosen,
        )

    def action_clear_events(self) -> None:
        if not self.events:
            self.notify(
                "There are no events to clear.",
                severity="information",
            )
            return

        self.push_screen(
            ConfirmScreen(
                "Clear Events",
                "Clear all {} received events?".format(len(self.events)),
                confirm_label="Clear",
                danger=True,
            ),
            self._clear_confirmed,
        )

    def _open_filter(self) -> None:
        self.push_screen(
            SettingsScreen(self.config, filter_only=True),
            self._filter_applied,
        )

    def _open_settings(self) -> None:
        self.push_screen(SettingsScreen(self.config), self._settings_applied)

    def _filter_applied(self, config: EslConfig | None) -> None:
        if config is None:
            return

        self._apply_config(
            EslConfig(
                host=self.config.host,
                port=self.config.port,
                password=self.config.password,
                events=config.events,
                callcenter_only=config.callcenter_only,
            ),
            "Event filter",
        )

    def _settings_applied(self, config: EslConfig | None) -> None:
        if config is not None:
            self._apply_config(config, "Settings")

    def _apply_config(self, config: EslConfig, subject: str) -> None:
        if config == self.config:
            return

        active = self._state in {"connecting", "streaming"}
        self.config = config

        try:
            save_config(config)
        except OSError as exc:
            self.notify(
                "Could not save settings: {}".format(exc),
                severity="warning",
                timeout=8,
            )

        self._refresh_status()
        if active:
            message = (
                "{} saved. The active connection is unchanged; "
                "stop and start again to apply the new settings."
            ).format(subject)
        else:
            message = (
                "{} saved. The new settings apply the next time you start."
            ).format(subject)
        self.notify(message, timeout=8)

    async def _start_stream(self) -> None:
        if self._lock is None:
            self._lock = asyncio.Lock()

        async with self._lock:
            await self._stop_stream_locked()

            self._run_token += 1
            token = self._run_token
            config = self.config
            self._active_config = config

            self._set_state("connecting")
            self._stream_task = asyncio.create_task(
                self._stream_loop(config, token),
                name="esl-grep-stream",
            )

    async def _stop_stream(self) -> None:
        if self._lock is None:
            return
        async with self._lock:
            await self._stop_stream_locked()

    async def _stop_stream_locked(self) -> None:
        self._run_token += 1
        self._active_config = None

        task = self._stream_task
        self._stream_task = None
        client = self._client
        self._client = None

        if task is not None:
            task.cancel()
            try:
                await task
            except asyncio.CancelledError:
                pass
            except Exception:
                pass
        elif client is not None:
            await client.close()

        self._set_state("stopped")

    async def _stream_loop(self, config: EslConfig, token: int) -> None:
        client: EslClient | None = None
        error_detail = ""

        try:
            client = await EslClient.connect(
                config.host,
                config.port,
                config.password,
                timeout=5.0,
            )
            if token != self._run_token:
                return

            self._client = client
            await client.subscribe(config.subscription_events())
            if token != self._run_token:
                return

            self._set_state("streaming")
            async for event in client.events():
                if token != self._run_token:
                    break
                if config.accepts_event(event.event_name, event.subclass):
                    self._append_event(event)
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            if token == self._run_token:
                error_detail = str(exc) or exc.__class__.__name__
        finally:
            if client is not None:
                await client.close()
            if token == self._run_token:
                self._client = None
                self._stream_task = None
                self._active_config = None
                if error_detail:
                    self._set_state("error", error_detail)
                    self.notify(
                        error_detail,
                        title="ESL Connection Error",
                        severity="error",
                        timeout=10,
                    )
                else:
                    self._set_state("stopped")

    def _append_event(self, event: EslEvent) -> None:
        first_event = not self.events
        self.events.append(event)

        cc_action = ""
        if is_callcenter_event(event.event_name, event.subclass):
            cc_action = event.header("CC-Action")

        table = self.query_one("#event-table", DataTable)
        table.add_row(
            str(len(self.events)),
            datetime.fromtimestamp(event.received_at)
            .astimezone()
            .strftime("%H:%M:%S.%f")[:-3],
            event.display_name,
            cc_action,
            key=str(len(self.events)),
        )

        self._refresh_counts()
        if first_event:
            table.move_cursor(row=0)
            self._show_event(event)

    def _refresh_counts(self) -> None:
        self.query_one("#event-count", Static).update(str(len(self.events)))
        self._refresh_status()
        self._refresh_controls()

    def _refresh_controls(self) -> None:
        active = self._state in {"connecting", "streaming"}
        has_events = bool(self.events)

        self.query_one("#start", Button).disabled = active
        self.query_one("#stop", Button).disabled = not active
        self.query_one("#save", Button).disabled = not has_events
        self.query_one("#clear", Button).disabled = not has_events

    @staticmethod
    def _filter_label(config: EslConfig) -> str:
        if config.callcenter_only:
            return "Call center only (CUSTOM callcenter::info)"
        if config.all_events:
            return "ALL"
        return "{} events".format(len(config.events))

    def _refresh_status(self) -> None:
        status = self.query_one("#status", Static)
        state = self._state
        active_config = self._active_config or self.config

        if state == "streaming":
            message = "● Receiving  {}  ·  {} events  ·  {}".format(
                active_config.server,
                len(self.events),
                self._filter_label(active_config),
            )
            if self.config != active_config:
                message += "  ·  Settings changed (restart required)"
        elif state == "connecting":
            message = "◌ Connecting  {}".format(active_config.server)
        elif state == "error":
            detail = " ".join(self._status_detail.split())
            if len(detail) > 70:
                detail = detail[:67] + "..."
            message = "● Error  {}".format(detail)
        else:
            message = "○ Stopped  ·  Filter {}".format(
                self._filter_label(self.config)
            )

        status.update(Text(message))
        for class_name in ("stopped", "connecting", "streaming", "error"):
            status.remove_class(class_name)
        status.add_class(state)

    def _set_state(self, state: str, detail: str = "") -> None:
        self._state = state
        self._status_detail = detail
        self._refresh_status()
        self._refresh_controls()

    def on_data_table_row_highlighted(
        self, event: DataTable.RowHighlighted
    ) -> None:
        self._select_row(event.row_key)

    def on_data_table_row_selected(self, event: DataTable.RowSelected) -> None:
        self._select_row(event.row_key)

    def _select_row(self, row_key: object) -> None:
        key = getattr(row_key, "value", row_key)
        try:
            index = int(str(key)) - 1
        except (TypeError, ValueError):
            return

        if 0 <= index < len(self.events):
            self._show_event(self.events[index])

    @on(Input.Submitted, "#detail-key")
    def key_search_submitted(self, event: Input.Submitted) -> None:
        self._highlight_key(event.value)

    def _highlight_key(self, raw_query: str) -> None:
        query = raw_query.strip().strip('"').strip("'").strip()
        if query.endswith(":"):
            query = query[:-1].strip()

        status = self.query_one("#detail-search-status", Static)
        area = self.query_one("#detail-body", TextArea)

        if not query:
            area.selection = Selection.cursor(area.cursor_location)
            status.update("")
            return

        for row, line in enumerate(area.text.splitlines()):
            match = JSON_KEY_RE.match(line)
            if match and match.group(1).casefold() == query.casefold():
                area.move_cursor((row, 0), center=True)
                # Put the cursor at the line start while still selecting the
                # whole line, so scrolling lands on the beginning of the row.
                area.selection = Selection((row, len(line)), (row, 0))
                status.update("Found · line {}".format(row + 1))
                return

        area.selection = Selection.cursor(area.cursor_location)
        status.update("Key not found")

    def _show_event(self, event: EslEvent) -> None:
        self._selected_event = event
        meta = Text()
        meta.append(event.display_name, style="bold cyan")

        uuid = event.header("Unique-ID") or event.header("Channel-UUID")
        if uuid:
            meta.append("  ·  UUID: {}".format(uuid))
        if event.parse_error:
            meta.append("  ·  Parse error: {}".format(event.parse_error))

        self.query_one("#detail-meta", Static).update(meta)
        self.query_one("#detail-size", Static).update(
            "{} bytes".format(len(event.raw))
        )
        self.query_one("#detail-search-status", Static).update("")
        self.query_one("#detail-body", TextArea).load_text(event.body)

    def _show_placeholder(self) -> None:
        self._selected_event = None
        self.query_one("#detail-meta", Static).update(
            "Select an event from the list on the left."
        )
        self.query_one("#detail-size", Static).update("")
        self.query_one("#detail-search-status", Static).update("")
        self.query_one("#detail-body", TextArea).load_text("")

    def _save_destination_chosen(
        self, result: tuple[Path, str] | None
    ) -> None:
        records = self._save_snapshot
        self._save_snapshot = None
        if result is None or records is None:
            return

        path, fmt = result
        if path.exists():
            self.push_screen(
                ConfirmScreen(
                    "Overwrite File",
                    "The file already exists:\n{}\n\n"
                    "Overwrite it?".format(path),
                    confirm_label="Overwrite",
                    danger=True,
                ),
                lambda confirmed: (
                    self._perform_save(records, path, fmt)
                    if confirmed
                    else None
                ),
            )
        else:
            self._perform_save(records, path, fmt)

    def _perform_save(
        self, records: list[EslEvent], path: Path, fmt: str
    ) -> None:
        self._spawn(self._write_capture(records, path, fmt))

    async def _write_capture(
        self, records: list[EslEvent], path: Path, fmt: str
    ) -> None:
        try:
            size = await asyncio.to_thread(
                write_capture, path, records, fmt
            )
        except OSError as exc:
            self.notify(
                "Could not save the file: {}".format(exc),
                severity="error",
                timeout=10,
            )
            return

        format_name = {
            JSON_FORMAT: "Event JSON",
            RAW_FORMAT: "Raw ESL",
            JSONL_FORMAT: "JSON Lines",
        }.get(fmt, fmt)
        self.notify(
            "Saved {} events ({}, {} bytes): {}".format(
                len(records),
                format_name,
                size,
                path,
            ),
            timeout=8,
        )

    def _clear_confirmed(self, confirmed: bool) -> None:
        if not confirmed:
            return

        self.events.clear()
        table = self.query_one("#event-table", DataTable)
        table.clear()
        self._show_placeholder()
        self._refresh_counts()

    def _spawn(self, coroutine: Coroutine[Any, Any, None]) -> None:
        task = asyncio.create_task(coroutine)
        self._background_tasks.add(task)
        task.add_done_callback(self._background_tasks.discard)


def main() -> None:
    ESLGrepApp().run()


if __name__ == "__main__":
    main()
