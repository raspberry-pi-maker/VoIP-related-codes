from __future__ import annotations

import asyncio
import json
import unittest
from unittest.mock import patch

from esl_grep.app import ESLGrepApp
from esl_grep.config import EslConfig


def _event_frame(name: str, subclass: str = "") -> bytes:
    event = {
        "Event-Name": name,
        "Unique-ID": "integration-test",
    }
    if subclass:
        event["Event-Subclass"] = subclass
    payload = json.dumps(event).encode("utf-8")
    return (
        b"Content-Type: text/event-json\n"
        + b"Content-Length: "
        + str(len(payload)).encode("ascii")
        + b"\n\n"
        + payload
    )


class _FakeEslServer:
    def __init__(
        self,
        events: tuple[tuple[str, str], ...] = (
            ("CHANNEL_CREATE", ""),
            ("RELOADXML", ""),
        ),
    ) -> None:
        self.commands: list[str] = []
        self._events = events
        self._server: asyncio.AbstractServer | None = None
        self.port = 0

    async def start(self) -> None:
        self._server = await asyncio.start_server(
            self._handle_client,
            "127.0.0.1",
            0,
        )
        socket = self._server.sockets[0]
        self.port = socket.getsockname()[1]

    async def close(self) -> None:
        if self._server is not None:
            self._server.close()
            await self._server.wait_closed()

    async def _handle_client(
        self,
        reader: asyncio.StreamReader,
        writer: asyncio.StreamWriter,
    ) -> None:
        try:
            writer.write(b"Content-Type: auth/request\n\n")
            await writer.drain()

            self.commands.append((await reader.readline()).decode().strip())
            await reader.readline()  # blank line after the command
            writer.write(
                b"Content-Type: command/reply\n"
                b"Reply-Text: +OK accepted\n\n"
            )
            await writer.drain()

            self.commands.append((await reader.readline()).decode().strip())
            await reader.readline()
            writer.write(
                b"Content-Type: command/reply\n"
                b"Reply-Text: +OK event listener enabled json\n\n"
            )
            await writer.drain()

            for name, subclass in self._events:
                writer.write(_event_frame(name, subclass))
                await writer.drain()
                await asyncio.sleep(0.02)

            await asyncio.sleep(5.0)
        finally:
            writer.close()


class StreamIntegrationTests(unittest.IsolatedAsyncioTestCase):
    async def test_start_receives_events_and_stop_closes_session(self) -> None:
        server = _FakeEslServer()
        await server.start()
        app = ESLGrepApp()
        app.config = EslConfig(
            host="127.0.0.1",
            port=server.port,
            password="ClueCon",
            events=("ALL",),
        )

        try:
            async with app.run_test(size=(120, 40)) as pilot:
                await pilot.pause()
                await pilot.click("#start")

                for _ in range(40):
                    await pilot.pause(0.05)
                    if len(app.events) == 2:
                        break

                self.assertEqual(
                    [event.event_name for event in app.events],
                    ["CHANNEL_CREATE", "RELOADXML"],
                )
                self.assertEqual(app.query_one("#event-table").row_count, 2)
                self.assertEqual(app._state, "streaming")
                self.assertIn("event json ALL", server.commands)

                await pilot.click("#stop")
                await pilot.pause()
                self.assertEqual(app._state, "stopped")
                self.assertFalse(app.query_one("#start").disabled)
        finally:
            await server.close()

    async def test_filter_change_applies_on_next_start(self) -> None:
        server = _FakeEslServer()
        await server.start()
        app = ESLGrepApp()
        app.config = EslConfig(
            host="127.0.0.1",
            port=server.port,
            password="ClueCon",
            events=("ALL",),
        )

        with patch("esl_grep.app.save_config"):
            try:
                async with app.run_test(size=(120, 40)) as pilot:
                    await pilot.pause()
                    await pilot.click("#start")

                    for _ in range(40):
                        await pilot.pause(0.05)
                        if app._state == "streaming" and len(app.events) == 2:
                            break

                    self.assertEqual(len(server.commands), 2)
                    task_before = app._stream_task
                    client_before = app._client

                    await pilot.click("#filter")
                    await pilot.pause()
                    screen = app.screen
                    screen._selected = {"RELOADXML"}
                    screen._populate("")
                    await pilot.pause()
                    await pilot.click("#settings-apply")
                    await pilot.pause()

                    self.assertEqual(app.config.events, ("RELOADXML",))
                    self.assertEqual(app._active_config.events, ("ALL",))
                    self.assertIs(app._stream_task, task_before)
                    self.assertIs(app._client, client_before)
                    self.assertEqual(len(server.commands), 2)
                    self.assertIn(
                        "Settings changed",
                        str(app.query_one("#status").render()),
                    )

                    await pilot.click("#stop")
                    await pilot.pause()
                    await pilot.click("#start")

                    for _ in range(40):
                        await pilot.pause(0.05)
                        if (
                            app._state == "streaming"
                            and len(server.commands) >= 4
                            and len(app.events) == 3
                        ):
                            break

                    self.assertEqual(
                        server.commands[-2:],
                        ["auth ClueCon", "event json RELOADXML"],
                    )
                    self.assertEqual(
                        [event.event_name for event in app.events],
                        ["CHANNEL_CREATE", "RELOADXML", "RELOADXML"],
                    )
            finally:
                await server.close()

    async def test_callcenter_only_receives_only_callcenter_events(
        self,
    ) -> None:
        server = _FakeEslServer(
            events=(
                ("CUSTOM", "conference::maintenance"),
                ("CUSTOM", "callcenter::info"),
                ("CHANNEL_CREATE", ""),
            )
        )
        await server.start()
        app = ESLGrepApp()
        app.config = EslConfig(
            host="127.0.0.1",
            port=server.port,
            password="ClueCon",
            events=("ALL",),
            callcenter_only=True,
        )

        try:
            async with app.run_test(size=(120, 40)) as pilot:
                await pilot.pause()
                await pilot.click("#start")

                for _ in range(40):
                    await pilot.pause(0.05)
                    if len(app.events) >= 1:
                        break

                self.assertIn(
                    "event json CUSTOM callcenter::info",
                    server.commands,
                )
                self.assertEqual(len(app.events), 1)
                self.assertEqual(
                    app.events[0].display_name,
                    "CUSTOM:callcenter::info",
                )
        finally:
            await server.close()
