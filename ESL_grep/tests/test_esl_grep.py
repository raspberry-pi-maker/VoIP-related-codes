from __future__ import annotations

import asyncio
import json
import stat
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from textual.screen import Screen
from textual.widgets import (
    Checkbox,
    DataTable,
    Input,
    Select,
    Static,
    TextArea,
)

from esl_grep.app import ESLGrepApp
from esl_grep.capture import (
    JSON_FORMAT,
    JSONL_FORMAT,
    RAW_FORMAT,
    encode_capture,
)
from esl_grep.config import EslConfig, load_config, save_config
from esl_grep.esl import EslClient, EslDisconnected, EslEvent, EslFrame
from esl_grep.events import (
    ALL_EVENTS,
    CALLCENTER_EVENT,
    CALLCENTER_SUBCLASS,
    CALLCENTER_SUBSCRIPTION,
    CALL_EVENTS,
    EVENT_NAMES,
    is_callcenter_event,
)
from esl_grep.screens import CopyJsonScreen, SettingsScreen
from esl_grep.theme import DETAIL_KEY_COLOR


def make_frame(
    name: str = "RELOADXML",
    extra: dict[str, str] | None = None,
) -> EslFrame:
    payload = {
        "Event-Name": name,
        "Event-Date-Local": "2026-11-06 12:29:00",
        "Unique-ID": "test-uuid",
    }
    if extra:
        payload.update(extra)
    body = json.dumps(payload).encode()
    headers = {
        "Content-Type": "text/event-json",
        "Content-Length": str(len(body)),
    }
    raw = (
        "Content-Type: text/event-json\r\n"
        "Content-Length: {}\r\n\r\n".format(len(body)).encode()
        + body
    )
    return EslFrame(headers=headers, body=body, raw=raw)


class EventListTests(unittest.TestCase):
    def test_catalog_is_unique_and_all_first(self) -> None:
        self.assertEqual(EVENT_NAMES[0], ALL_EVENTS)
        self.assertEqual(len(EVENT_NAMES), len(set(EVENT_NAMES)))

    def test_call_preset_is_subset(self) -> None:
        self.assertTrue(set(CALL_EVENTS).issubset(EVENT_NAMES))
        self.assertNotIn(ALL_EVENTS, CALL_EVENTS)


class CaptureTests(unittest.TestCase):
    def test_raw_capture_concatenates_frames(self) -> None:
        first = EslEvent.from_frame(make_frame())
        second = EslEvent.from_frame(make_frame("CHANNEL_CREATE"))
        raw = encode_capture([first, second], RAW_FORMAT)
        self.assertEqual(raw, first.raw + second.raw)

    def test_jsonl_capture_contains_packet_data(self) -> None:
        event = EslEvent.from_frame(make_frame())
        payload = encode_capture([event], JSONL_FORMAT).decode()
        record = json.loads(payload)
        self.assertEqual(record["event"], "RELOADXML")
        self.assertEqual(record["headers"]["Event-Name"], "RELOADXML")
        self.assertIn("Event-Name", record["body"])
        self.assertTrue(record["raw"].startswith("Content-Type:"))

    def test_json_capture_contains_only_event_detail_json(self) -> None:
        first = EslEvent.from_frame(make_frame())
        second = EslEvent.from_frame(make_frame("CHANNEL_CREATE"))
        records = json.loads(encode_capture([first, second], JSON_FORMAT))

        self.assertEqual(
            [record["Event-Name"] for record in records],
            ["RELOADXML", "CHANNEL_CREATE"],
        )
        self.assertEqual(records[0], json.loads(first.body))
        # The wrapper metadata of the JSON Lines format must not leak in.
        self.assertNotIn("received_at", records[0])
        self.assertNotIn("headers", records[0])
        self.assertNotIn("raw", records[0])


class ConfigTests(unittest.TestCase):
    def test_round_trip(self) -> None:
        config = EslConfig(
            host="192.0.2.10",
            port=8022,
            password="secret",
            events=("CHANNEL_CREATE", "RELOADXML"),
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "config.json"
            save_config(config, path)
            self.assertEqual(load_config(path), config)
            mode = stat.S_IMODE(path.stat().st_mode)
            self.assertEqual(mode, 0o600)

    def test_invalid_file_falls_back_to_defaults(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "config.json"
            path.write_text("not json", encoding="utf-8")
            self.assertEqual(load_config(path), EslConfig())


class CallcenterTests(unittest.TestCase):
    def test_detects_callcenter_events(self) -> None:
        self.assertTrue(
            is_callcenter_event(CALLCENTER_EVENT, CALLCENTER_SUBCLASS)
        )
        self.assertTrue(is_callcenter_event("custom", "CALLCENTER::INFO"))
        self.assertFalse(
            is_callcenter_event("CUSTOM", "conference::maintenance")
        )
        self.assertFalse(
            is_callcenter_event("CHANNEL_CREATE", CALLCENTER_SUBCLASS)
        )

    def test_callcenter_only_overrides_event_selection(self) -> None:
        config = EslConfig(
            events=("CHANNEL_CREATE",),
            callcenter_only=True,
        )
        self.assertEqual(
            config.subscription_events(), (CALLCENTER_SUBSCRIPTION,)
        )
        self.assertTrue(
            config.accepts_event(CALLCENTER_EVENT, CALLCENTER_SUBCLASS)
        )
        self.assertFalse(config.accepts_event("CHANNEL_CREATE"))

    def test_callcenter_only_round_trip(self) -> None:
        config = EslConfig(callcenter_only=True)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "config.json"
            save_config(config, path)
            loaded = load_config(path)
            self.assertTrue(loaded.callcenter_only)
            self.assertEqual(loaded, config)


class FrameParsingTests(unittest.IsolatedAsyncioTestCase):
    async def test_reads_a_complete_frame(self) -> None:
        reader = asyncio.StreamReader()
        reader.feed_data(make_frame().raw)
        client = EslClient(reader, object(), "127.0.0.1", 8021)  # type: ignore[arg-type]
        frame = await client._read_frame()
        self.assertEqual(frame.content_type, "text/event-json")
        self.assertEqual(json.loads(frame.body_text)["Event-Name"], "RELOADXML")

    async def test_eof_is_reported(self) -> None:
        reader = asyncio.StreamReader()
        reader.feed_eof()
        client = EslClient(reader, object(), "127.0.0.1", 8021)  # type: ignore[arg-type]
        with self.assertRaises(EslDisconnected):
            await client._read_frame()


class _CaptureWriter:
    def __init__(self) -> None:
        self.data = b""

    def write(self, data: bytes) -> None:
        self.data += data

    async def drain(self) -> None:
        return None


def _reply_reader() -> asyncio.StreamReader:
    reader = asyncio.StreamReader()
    reader.feed_data(
        b"Content-Type: command/reply\nReply-Text: +OK accepted\n\n"
    )
    return reader


class SubscriptionTests(unittest.IsolatedAsyncioTestCase):
    async def test_callcenter_subscription_keeps_subclass_case(self) -> None:
        writer = _CaptureWriter()
        client = EslClient(  # type: ignore[arg-type]
            _reply_reader(), writer, "127.0.0.1", 8021
        )
        await client.subscribe(
            EslConfig(callcenter_only=True).subscription_events()
        )
        self.assertEqual(
            writer.data,
            b"event json CUSTOM callcenter::info\n\n",
        )

    async def test_plain_event_names_are_uppercased(self) -> None:
        writer = _CaptureWriter()
        client = EslClient(  # type: ignore[arg-type]
            _reply_reader(), writer, "127.0.0.1", 8021
        )
        await client.subscribe(("reloadxml",))
        self.assertEqual(writer.data, b"event json RELOADXML\n\n")


class AppSmokeTests(unittest.IsolatedAsyncioTestCase):
    async def test_layout_settings_and_event_detail(self) -> None:
        app = ESLGrepApp()
        app.config = EslConfig(events=(ALL_EVENTS,))

        async with app.run_test(size=(120, 40)) as pilot:
            await pilot.pause()

            self.assertEqual(app.query_one("#event-table").row_count, 0)
            self.assertEqual(app._state, "stopped")

            await pilot.click("#settings")
            await pilot.pause()
            self.assertIsInstance(app.screen, SettingsScreen)

            search = app.screen.query_one("#event-search")
            search.value = "RELOADXML"
            await pilot.pause()
            self.assertEqual(
                app.screen.query_one("#event-list").option_count, 1
            )

            await pilot.press("escape")
            await pilot.pause()
            self.assertIsInstance(app.screen, Screen)

            app._append_event(EslEvent.from_frame(make_frame()))
            await pilot.pause()
            self.assertEqual(app.query_one("#event-table").row_count, 1)
            self.assertIn(
                "RELOADXML", app.query_one("#detail-body").text
            )

    async def test_copy_view_shows_only_event_json(self) -> None:
        app = ESLGrepApp()

        async with app.run_test(size=(100, 30)) as pilot:
            await pilot.pause()
            app._append_event(EslEvent.from_frame(make_frame()))
            await pilot.pause()
            detail_text = app.query_one("#detail-body", TextArea).text

            # Ctrl+Y is a priority binding, so it also works while the
            # read-only detail text area has focus (it normally owns Ctrl+Y).
            app.query_one("#detail-body", TextArea).focus()
            await pilot.press("ctrl+y")
            await pilot.pause()

            self.assertIsInstance(app.screen, CopyJsonScreen)
            # An opaque full-screen background hides the panes behind the
            # modal, so terminal Shift+drag can only select the JSON.
            self.assertEqual(app.screen.styles.background.a, 1.0)

            body = app.screen.query_one("#copy-body", TextArea)
            self.assertEqual(body.text, detail_text)
            self.assertTrue(body.read_only)
            self.assertIs(app.focused, body)

            self.assertTrue(body.soft_wrap)
            await pilot.press("f2")
            await pilot.pause()
            self.assertFalse(body.soft_wrap)

            await pilot.press("escape")
            await pilot.pause()
            self.assertNotIsInstance(app.screen, CopyJsonScreen)

            # The pane heading button opens the same view.
            await pilot.click("#copy-detail")
            await pilot.pause()
            self.assertIsInstance(app.screen, CopyJsonScreen)

    async def test_save_defaults_to_event_json_only(self) -> None:
        app = ESLGrepApp()

        async with app.run_test(size=(100, 30)) as pilot:
            await pilot.pause()
            app._append_event(EslEvent.from_frame(make_frame()))
            await pilot.pause()

            await pilot.click("#save")
            await pilot.pause()

            fmt = app.screen.query_one("#save-format", Select)
            path = app.screen.query_one("#save-path", Input)
            self.assertEqual(fmt.value, JSON_FORMAT)
            self.assertTrue(path.value.endswith(".json"))

            fmt.value = RAW_FORMAT
            await pilot.pause()
            self.assertTrue(path.value.endswith(".esl"))

    async def test_event_detail_json_is_syntax_highlighted(self) -> None:
        app = ESLGrepApp()

        async with app.run_test(size=(100, 30)) as pilot:
            await pilot.pause()
            app._append_event(EslEvent.from_frame(make_frame()))
            await pilot.pause()

            detail = app.query_one("#detail-body", TextArea)
            self.assertEqual(
                type(detail.document).__name__,
                "SyntaxAwareDocument",
            )
            self.assertTrue(detail._highlights)
            key_style = detail._theme.get_highlight("json.label")
            self.assertIsNotNone(key_style)
            self.assertEqual(
                key_style.color.get_truecolor().hex.upper(),
                DETAIL_KEY_COLOR.upper(),
            )

    async def test_detail_header_shows_uuid_on_one_line(self) -> None:
        app = ESLGrepApp()

        async with app.run_test(size=(100, 30)) as pilot:
            await pilot.pause()
            app._append_event(EslEvent.from_frame(make_frame()))
            await pilot.pause()

            meta = str(app.query_one("#detail-meta", Static).render())
            self.assertIn("RELOADXML", meta)
            self.assertIn("UUID: test-uuid", meta)
            self.assertNotIn("2026-11-06 12:29:00", meta)
            self.assertNotIn("\n", meta)

    async def test_cc_action_is_shown_only_for_callcenter_events(self) -> None:
        app = ESLGrepApp()

        async with app.run_test(size=(120, 40)) as pilot:
            await pilot.pause()
            app._append_event(
                EslEvent.from_frame(make_frame("CHANNEL_CREATE"))
            )
            app._append_event(
                EslEvent.from_frame(
                    make_frame(
                        CALLCENTER_EVENT,
                        {
                            "Event-Subclass": CALLCENTER_SUBCLASS,
                            "CC-Action": "agent-status-change",
                        },
                    )
                )
            )
            await pilot.pause()

            table = app.query_one("#event-table", DataTable)
            self.assertEqual(table.get_row_at(0)[3], "")
            self.assertEqual(
                table.get_row_at(1)[3], "agent-status-change"
            )

    async def test_key_lookup_highlights_matching_line(self) -> None:
        app = ESLGrepApp()

        async with app.run_test(size=(120, 40)) as pilot:
            await pilot.pause()
            app._append_event(
                EslEvent.from_frame(
                    make_frame(
                        CALLCENTER_EVENT,
                        {
                            "Event-Subclass": CALLCENTER_SUBCLASS,
                            "CC-Action": "agent-status-change",
                        },
                    )
                )
            )
            await pilot.pause()

            detail = app.query_one("#detail-body", TextArea)
            lines = detail.text.splitlines()

            app._highlight_key("cc-action")
            await pilot.pause()

            row = detail.selection.end[0]
            self.assertIn('"CC-Action"', lines[row])
            self.assertEqual(detail.selection.start[0], row)
            self.assertEqual(detail.selection.start[1], len(lines[row]))
            self.assertIn(
                "Found",
                str(app.query_one("#detail-search-status").render()),
            )

            app._highlight_key("No-Such-Key")
            await pilot.pause()
            self.assertTrue(detail.selection.is_empty)
            self.assertIn(
                "not found",
                str(app.query_one("#detail-search-status").render()),
            )

    async def test_filter_button_changes_only_events(self) -> None:
        app = ESLGrepApp()
        app.config = EslConfig(
            host="192.0.2.5",
            port=9000,
            password="secret",
            events=(ALL_EVENTS,),
        )

        with patch("esl_grep.app.save_config") as save_config:
            async with app.run_test(size=(100, 30)) as pilot:
                await pilot.pause()

                await pilot.click("#filter")
                await pilot.pause()
                screen = app.screen
                self.assertTrue(screen._filter_only)
                self.assertEqual(len(screen.query("#settings-host")), 0)
                self.assertIs(
                    app.focused, screen.query_one("#event-search")
                )

                screen._selected = {"CHANNEL_CREATE", "RELOADXML"}
                screen._populate("")
                await pilot.pause()
                await pilot.click("#settings-apply")
                await pilot.pause()

        self.assertEqual(app.config.host, "192.0.2.5")
        self.assertEqual(app.config.port, 9000)
        self.assertEqual(app.config.password, "secret")
        self.assertEqual(
            set(app.config.events), {"CHANNEL_CREATE", "RELOADXML"}
        )
        self.assertEqual(app._state, "stopped")
        save_config.assert_called_once()

    async def test_callcenter_checkbox_overrides_event_list(self) -> None:
        app = ESLGrepApp()
        app.config = EslConfig(events=(ALL_EVENTS,))

        with patch("esl_grep.app.save_config"):
            async with app.run_test(size=(100, 30)) as pilot:
                await pilot.pause()

                await pilot.click("#filter")
                await pilot.pause()
                screen = app.screen
                self.assertFalse(screen._callcenter_only)
                self.assertFalse(
                    screen.query_one("#event-list").disabled
                )

                screen.query_one("#callcenter-only", Checkbox).value = True
                await pilot.pause()
                self.assertTrue(screen.query_one("#event-list").disabled)

                await pilot.click("#settings-apply")
                await pilot.pause()

        self.assertTrue(app.config.callcenter_only)
        self.assertEqual(
            app.config.subscription_events(),
            ("CUSTOM callcenter::info",),
        )
