"""Modal screens used by the esl_grep TUI."""

from __future__ import annotations

from pathlib import Path

from textual import on
from textual.app import ComposeResult
from textual.containers import Grid, Horizontal, Vertical
from textual.screen import ModalScreen
from textual.widgets import (
    Button,
    Checkbox,
    Input,
    Label,
    Select,
    SelectionList,
    Static,
    TextArea,
)
from textual.widgets.selection_list import Selection

from .capture import CAPTURE_FORMATS, JSON_FORMAT, JSONL_FORMAT, RAW_FORMAT
from .config import EslConfig, event_names_for_ui
from .events import ALL_EVENTS, CALL_EVENTS
from .theme import DETAIL_THEME_NAME, build_detail_theme


class ConfirmScreen(ModalScreen[bool]):
    """A small yes/no dialog."""

    BINDINGS = [("escape", "cancel", "Cancel")]

    def __init__(
        self,
        title: str,
        message: str,
        *,
        confirm_label: str = "Confirm",
        danger: bool = False,
    ) -> None:
        super().__init__()
        self._title = title
        self._message = message
        self._confirm_label = confirm_label
        self._danger = danger

    def compose(self) -> ComposeResult:
        with Vertical(id="confirm-dialog"):
            yield Label(self._title, id="confirm-title")
            yield Static(self._message, id="confirm-message", markup=False)
            with Horizontal(id="confirm-buttons"):
                yield Button("Cancel", id="confirm-cancel")
                yield Button(
                    self._confirm_label,
                    id="confirm-ok",
                    variant="error" if self._danger else "primary",
                )

    @on(Button.Pressed, "#confirm-cancel")
    def cancel_button(self) -> None:
        self.dismiss(False)

    @on(Button.Pressed, "#confirm-ok")
    def confirm_button(self) -> None:
        self.dismiss(True)

    def action_cancel(self) -> None:
        self.dismiss(False)

    CSS = """
    ConfirmScreen {
        align: center middle;
        background: $background 70%;
    }

    #confirm-dialog {
        width: 64;
        max-width: 90%;
        height: auto;
        padding: 1 2;
        border: thick $primary;
        background: $surface;
    }

    #confirm-title {
        height: auto;
        text-style: bold;
        color: $text;
        margin-bottom: 1;
    }

    #confirm-message {
        height: auto;
        color: $text-muted;
        margin-bottom: 1;
    }

    #confirm-buttons {
        height: 3;
        align: right middle;
    }

    #confirm-buttons Button {
        margin-left: 1;
        min-width: 10;
    }
    """


class SettingsScreen(ModalScreen[EslConfig]):
    """Server and event-subscription settings."""

    BINDINGS = [("escape", "cancel", "Cancel")]

    def __init__(
        self,
        config: EslConfig,
        *,
        filter_only: bool = False,
    ) -> None:
        super().__init__()
        self._config = config
        self._filter_only = filter_only
        self._all_events = event_names_for_ui(config.events)
        self._selected: set[str] = set(config.events) or {ALL_EVENTS}
        self._callcenter_only = config.callcenter_only
        self._visible: set[str] = set()
        self._last_visible_selected: set[str] = set()
        self._query = ""
        self._rebuilding = False

    def compose(self) -> ComposeResult:
        dialog_classes = "filter-only" if self._filter_only else None
        with Vertical(id="settings-dialog", classes=dialog_classes):
            yield Label(
                (
                    "Event Filter"
                    if self._filter_only
                    else "FreeSWITCH ESL Settings"
                ),
                id="settings-title",
            )

            if not self._filter_only:
                with Grid(id="server-grid"):
                    yield Label("Host", classes="field-label")
                    yield Label("Port", classes="field-label")
                    yield Label("Password", classes="field-label")
                    yield Input(
                        value=self._config.host,
                        placeholder="127.0.0.1",
                        id="settings-host",
                    )
                    yield Input(
                        value=str(self._config.port),
                        placeholder="8021",
                        type="integer",
                        id="settings-port",
                    )
                    yield Input(
                        value=self._config.password,
                        placeholder="ClueCon",
                        password=True,
                        id="settings-password",
                    )

            with Horizontal(id="event-toolbar"):
                yield Label("Events to receive", id="events-heading")
                yield Input(
                    placeholder="Search events...",
                    id="event-search",
                )
                yield Button("All", id="event-all", compact=True)
                yield Button("Calls", id="event-call", compact=True)
                yield Checkbox(
                    "Call center only",
                    value=self._callcenter_only,
                    compact=True,
                    tooltip=(
                        "Receive only mod_callcenter's CUSTOM "
                        "callcenter::info events."
                    ),
                    id="callcenter-only",
                )
                yield Button("None", id="event-none", compact=True)

            yield SelectionList[str](id="event-list", compact=True)
            yield Label("", id="selection-summary")

            with Horizontal(id="settings-buttons"):
                yield Button("Cancel", id="settings-cancel")
                yield Button(
                    "Apply Filter" if self._filter_only else "Apply",
                    id="settings-apply",
                    variant="primary",
                )

    def on_mount(self) -> None:
        self._populate("")
        self._apply_callcenter_state()
        if self._filter_only:
            if self._callcenter_only:
                self.query_one("#callcenter-only", Checkbox).focus()
            else:
                self.query_one("#event-search", Input).focus()
        else:
            self.query_one("#settings-host", Input).focus()

    def _populate(self, query: str) -> None:
        self._query = query
        needle = query.strip().upper()
        names = [
            name
            for name in self._all_events
            if not needle or needle in name.upper()
        ]

        widget = self.query_one("#event-list", SelectionList)
        self._rebuilding = True
        try:
            widget.clear_options()
            selections = []
            for name in names:
                label = "ALL  ·  all events" if name == ALL_EVENTS else name
                selections.append(
                    Selection(
                        label,
                        name,
                        initial_state=name in self._selected,
                    )
                )
            if selections:
                widget.add_options(selections)
        finally:
            self._rebuilding = False

        self._visible = set(names)
        self._last_visible_selected = set(widget.selected)
        self._update_summary()

    def _apply_callcenter_state(self) -> None:
        """Enable or disable the regular event controls.

        The checkbox overrides the list, so leaving it checked while the list
        is disabled makes the effective filter unambiguous.
        """

        enabled = not self._callcenter_only
        for selector in (
            "#event-search",
            "#event-list",
            "#event-all",
            "#event-call",
            "#event-none",
        ):
            self.query_one(selector).disabled = not enabled
        self._update_summary()

    def _update_summary(self) -> None:
        summary = self.query_one("#selection-summary", Label)
        if self._callcenter_only:
            summary.update(
                "Selected: call center events only — CUSTOM callcenter::info"
            )
            return
        if ALL_EVENTS in self._selected:
            summary.update("Selected: all events (ALL)")
            return
        if not self._selected:
            summary.update("Selected: none — select at least one event")
            return

        ordered = [name for name in self._all_events if name in self._selected]
        preview = ", ".join(ordered[:5])
        if len(ordered) > 5:
            preview += f" and {len(ordered) - 5} more"
        noun = "event" if len(ordered) == 1 else "events"
        summary.update(f"Selected: {len(ordered)} {noun} · {preview}")

    @on(Input.Changed, "#event-search")
    def search_changed(self, event: Input.Changed) -> None:
        self._populate(event.value)

    @on(Checkbox.Changed, "#callcenter-only")
    def callcenter_changed(self, event: Checkbox.Changed) -> None:
        self._callcenter_only = bool(event.value)
        self._apply_callcenter_state()

    @on(SelectionList.SelectedChanged, "#event-list")
    def selection_changed(self) -> None:
        if self._rebuilding:
            return

        widget = self.query_one("#event-list", SelectionList)
        selected_visible = set(widget.selected)
        added = selected_visible - self._last_visible_selected
        hidden_selection = self._selected - self._visible

        new_selection = hidden_selection | selected_visible

        if ALL_EVENTS in added:
            # ALL is exclusive; the server-side subscription ignores the rest.
            new_selection = {ALL_EVENTS}
        elif added - {ALL_EVENTS}:
            # Choosing a concrete event cancels the ALL shortcut.
            new_selection.discard(ALL_EVENTS)

        self._selected = new_selection
        self._last_visible_selected = selected_visible

        if ALL_EVENTS in added:
            self._populate(self._query)
        else:
            self._update_summary()

    @on(Button.Pressed, "#event-all")
    def select_all(self) -> None:
        self._selected = {ALL_EVENTS}
        self._populate(self._query)

    @on(Button.Pressed, "#event-call")
    def select_calls(self) -> None:
        self._selected = set(CALL_EVENTS)
        self._populate(self._query)

    @on(Button.Pressed, "#event-none")
    def select_none(self) -> None:
        self._selected = set()
        self._populate(self._query)

    @on(Button.Pressed, "#settings-cancel")
    def cancel_button(self) -> None:
        self.dismiss(None)

    @on(Button.Pressed, "#settings-apply")
    def apply_button(self) -> None:
        if not self._selected and not self._callcenter_only:
            self.notify(
                "Select at least one event to receive",
                severity="error",
            )
            return

        if ALL_EVENTS in self._selected:
            events = (ALL_EVENTS,)
        else:
            events = tuple(
                name
                for name in self._all_events
                if name != ALL_EVENTS and name in self._selected
            )

        if self._filter_only:
            self.dismiss(
                EslConfig(
                    host=self._config.host,
                    port=self._config.port,
                    password=self._config.password,
                    events=events,
                    callcenter_only=self._callcenter_only,
                )
            )
            return

        host = self.query_one("#settings-host", Input).value.strip()
        port_text = self.query_one("#settings-port", Input).value.strip()
        password = self.query_one("#settings-password", Input).value

        if not host:
            self.notify("Enter a host", severity="error")
            return
        if "\n" in host or "\r" in host:
            self.notify("The host must not contain newlines", severity="error")
            return
        if "\n" in password or "\r" in password:
            self.notify(
                "The password must not contain newlines",
                severity="error",
            )
            return
        try:
            port = int(port_text)
        except ValueError:
            self.notify("The port must be a number", severity="error")
            return
        if not 1 <= port <= 65535:
            self.notify(
                "The port must be between 1 and 65535",
                severity="error",
            )
            return

        self.dismiss(
            EslConfig(
                host=host,
                port=port,
                password=password,
                events=events,
                callcenter_only=self._callcenter_only,
            )
        )

    def action_cancel(self) -> None:
        self.dismiss(None)

    CSS = """
    SettingsScreen {
        align: center middle;
        background: $background 70%;
    }

    #settings-dialog {
        width: 110;
        max-width: 96%;
        height: 90%;
        max-height: 48;
        min-height: 20;
        padding: 1 2;
        border: thick $primary;
        background: $surface;
    }

    #settings-dialog.filter-only {
        height: 82%;
        max-height: 40;
        min-height: 16;
    }

    #settings-title {
        height: 1;
        text-style: bold;
        color: $text;
    }

    #server-grid {
        height: 4;
        grid-size: 3 2;
        grid-columns: 1fr 10 1fr;
        grid-rows: 1 3;
        grid-gutter: 0 1;
    }

    .field-label {
        height: 1;
        color: $text-muted;
    }

    #event-toolbar {
        height: 3;
    }

    #events-heading {
        width: 14;
        height: 3;
        content-align: left middle;
        text-style: bold;
        color: $text;
    }

    #event-search {
        width: 1fr;
    }

    #event-toolbar Button {
        min-width: 7;
        height: 3;
        margin-left: 1;
    }

    #callcenter-only {
        width: auto;
        height: 3;
        margin-left: 1;
        padding: 0 1;
        content-align: left middle;
        color: $text;
    }

    #event-list {
        height: 1fr;
        border: solid $panel;
        background: $surface;
    }

    #selection-summary {
        width: 1fr;
        height: 1;
        color: $text-muted;
        text-overflow: ellipsis;
    }

    #settings-buttons {
        height: 3;
        align: right middle;
    }

    #settings-buttons Button {
        min-width: 10;
        margin-left: 1;
    }
    """


class SaveCaptureScreen(ModalScreen[tuple[Path, str]]):
    """Choose where to save the current capture and in which format."""

    BINDINGS = [("escape", "cancel", "Cancel")]

    def __init__(self, default_path: Path, event_count: int) -> None:
        super().__init__()
        self._default_path = default_path
        self._event_count = event_count

    def compose(self) -> ComposeResult:
        with Vertical(id="save-dialog"):
            yield Label("Save Received Events", id="save-title")
            yield Static(
                f"Saving {self._event_count:,} received events.",
                id="save-count",
            )

            yield Label("File path", classes="save-label")
            yield Input(value=str(self._default_path), id="save-path")

            yield Label("Save format", classes="save-label")
            yield Select(
                [
                    ("Event JSON only (.json)", JSON_FORMAT),
                    ("Raw ESL packets (.esl)", RAW_FORMAT),
                    ("JSON Lines (.jsonl)", JSONL_FORMAT),
                ],
                value=JSON_FORMAT,
                allow_blank=False,
                id="save-format",
            )

            yield Static(
                "Event JSON saves only the JSON shown in each event detail "
                "as a single JSON array. Raw ESL keeps the received "
                "frames unchanged, and JSON Lines writes the headers and "
                "body as one structured record per line.",
                id="save-hint",
            )

            with Horizontal(id="save-buttons"):
                yield Button("Cancel", id="save-cancel")
                yield Button("Save", id="save-confirm", variant="primary")

    def on_mount(self) -> None:
        path_input = self.query_one("#save-path", Input)
        path_input.focus()
        # Put the cursor at the end so the timestamped filename is easy to edit.
        path_input.cursor_position = len(path_input.value)

    @on(Select.Changed, "#save-format")
    def format_changed(self, event: Select.Changed) -> None:
        if event.value not in CAPTURE_FORMATS:
            return
        suffixes = {
            JSON_FORMAT: ".json",
            RAW_FORMAT: ".esl",
            JSONL_FORMAT: ".jsonl",
        }
        path_input = self.query_one("#save-path", Input)
        current = Path(path_input.value.strip()).expanduser()
        if current.suffix in suffixes.values():
            path_input.value = str(current.with_suffix(suffixes[event.value]))
            path_input.cursor_position = len(path_input.value)

    @on(Button.Pressed, "#save-cancel")
    def cancel_button(self) -> None:
        self.dismiss(None)

    @on(Button.Pressed, "#save-confirm")
    def confirm_button(self) -> None:
        raw_path = self.query_one("#save-path", Input).value.strip()
        fmt = self.query_one("#save-format", Select).value

        if not raw_path:
            self.notify("Enter a file path to save", severity="error")
            return

        path = Path(raw_path).expanduser()
        if not path.is_absolute():
            path = Path.cwd() / path
        if path.exists() and path.is_dir():
            self.notify("The save path is a directory", severity="error")
            return
        if fmt not in CAPTURE_FORMATS:
            self.notify("Select a save format", severity="error")
            return

        self.dismiss((path, str(fmt)))

    def action_cancel(self) -> None:
        self.dismiss(None)

    CSS = """
    SaveCaptureScreen {
        align: center middle;
        background: $background 70%;
    }

    #save-dialog {
        width: 86;
        max-width: 94%;
        height: auto;
        padding: 1 2;
        border: thick $primary;
        background: $surface;
    }

    #save-title {
        height: 1;
        text-style: bold;
        color: $text;
        margin-bottom: 1;
    }

    #save-count {
        height: auto;
        color: $text-muted;
        margin-bottom: 1;
    }

    .save-label {
        height: 1;
        margin-top: 1;
        color: $text-muted;
    }

    #save-format {
        margin-bottom: 1;
    }

    #save-hint {
        height: auto;
        color: $text-muted;
        text-style: italic;
        margin-top: 1;
        margin-bottom: 1;
    }

    #save-buttons {
        height: 3;
        align: right middle;
    }

    #save-buttons Button {
        min-width: 10;
        margin-left: 1;
    }
    """


class CopyJsonScreen(ModalScreen[None]):
    """Show one event's JSON on a full screen so terminal selection is clean.

    Terminals handle ``Shift``+drag themselves, so the app cannot restrict
    the selection to the detail pane.  This screen hides every other pane
    behind an opaque background, leaving the JSON as the only text.
    """

    BINDINGS = [
        ("escape", "close", "Close"),
        ("f2", "toggle_wrap", "Wrap"),
    ]

    def __init__(self, text: str, *, subtitle: str = "") -> None:
        super().__init__()
        self._text = text
        self._subtitle = subtitle

    def compose(self) -> ComposeResult:
        with Vertical(id="copy-dialog"):
            with Horizontal(id="copy-heading"):
                yield Label("Copy Event JSON", id="copy-title")
                yield Static(self._subtitle, id="copy-subtitle")
            yield Static(
                "Shift+mouse drag to select the JSON; the terminal copies "
                "it to the clipboard.  Scroll: wheel/PgUp/PgDn  ·  "
                "Wrap: F2  ·  Close: Esc",
                id="copy-hint",
                markup=False,
            )
            yield TextArea.code_editor(
                self._text,
                language="json",
                theme="monokai",
                read_only=True,
                show_line_numbers=False,
                soft_wrap=True,
                id="copy-body",
            )

    def on_mount(self) -> None:
        body = self.query_one("#copy-body", TextArea)
        body.register_theme(build_detail_theme())
        body.theme = DETAIL_THEME_NAME
        body.focus()

    def action_close(self) -> None:
        self.dismiss(None)

    def action_toggle_wrap(self) -> None:
        body = self.query_one("#copy-body", TextArea)
        body.soft_wrap = not body.soft_wrap
        if body.soft_wrap:
            self.notify("Line wrapping on", timeout=2)
        else:
            self.notify(
                "Line wrapping off — use Left/Right to scroll long lines "
                "when copying",
                timeout=4,
            )

    CSS = """
    CopyJsonScreen {
        background: $surface;
    }

    #copy-dialog {
        width: 100%;
        height: 100%;
        padding: 0 1;
        background: $surface;
    }

    #copy-heading {
        height: 1;
        background: $surface;
    }

    #copy-title {
        width: auto;
        text-style: bold;
        color: $accent;
    }

    #copy-subtitle {
        width: 1fr;
        padding: 0 2;
        color: $text-muted;
        text-overflow: ellipsis;
    }

    #copy-hint {
        height: 1;
        color: $text-muted;
        text-style: italic;
        text-overflow: ellipsis;
    }

    #copy-body {
        height: 1fr;
        border: none;
        background: $surface;
    }
    """
