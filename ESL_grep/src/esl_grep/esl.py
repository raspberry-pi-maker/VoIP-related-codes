"""A small asyncio client for the FreeSWITCH Event Socket Library protocol.

This module deliberately implements just the inbound-event-socket subset used
by the TUI: authenticate, subscribe to ``event json`` events, and consume event
frames.  Raw frames are retained so they can be written to disk unchanged.
"""

from __future__ import annotations

import asyncio
import json
import time
from collections.abc import AsyncIterator
from dataclasses import dataclass
from typing import Any
from urllib.parse import unquote

MAX_HEADER_BYTES = 1024 * 1024
MAX_BODY_BYTES = 64 * 1024 * 1024


class EslError(Exception):
    """Base class for ESL failures."""


class EslConnectionError(EslError):
    """The TCP connection could not be established or was lost."""


class EslProtocolError(EslError):
    """The server sent a malformed frame."""


class EslAuthError(EslError):
    """Authentication failed."""


class EslTimeoutError(EslError):
    """A command did not receive a reply in time."""


class EslDisconnected(EslConnectionError):
    """The remote end closed or rejected the event socket session."""


@dataclass(frozen=True)
class EslFrame:
    """One complete ESL protocol frame."""

    headers: dict[str, str]
    body: bytes
    raw: bytes

    def header(self, name: str, default: str = "") -> str:
        wanted = name.casefold()
        for key, value in self.headers.items():
            if key.casefold() == wanted:
                return value
        return default

    @property
    def content_type(self) -> str:
        return self.header("Content-Type").strip().casefold()

    @property
    def body_text(self) -> str:
        return self.body.decode("utf-8", errors="replace")


def _parse_header_block(data: bytes) -> dict[str, str]:
    headers: dict[str, str] = {}
    for raw_line in data.splitlines():
        line = raw_line.decode("utf-8", errors="replace")
        if not line:
            continue
        if ":" not in line:
            raise EslProtocolError(f"malformed ESL header line: {line!r}")
        key, value = line.split(":", 1)
        key = key.strip()
        if not key:
            raise EslProtocolError("empty ESL header name")
        headers[key] = value.lstrip(" \t")
    return headers


def _header(headers: dict[str, str], name: str, default: str = "") -> str:
    wanted = name.casefold()
    for key, value in headers.items():
        if key.casefold() == wanted:
            return value
    return default


def _stringify_json_value(value: Any) -> str:
    if isinstance(value, str):
        return value
    if isinstance(value, (dict, list)):
        return json.dumps(value, ensure_ascii=False)
    if value is None:
        return ""
    return str(value)


def _parse_plain_event(text: str) -> tuple[dict[str, str], str]:
    """Split a text/event-plain payload into headers and body."""

    head = text
    body = ""
    for separator in ("\r\n\r\n", "\n\n"):
        if separator in text:
            head, body = text.split(separator, 1)
            break

    headers: dict[str, str] = {}
    for line in head.splitlines():
        if not line:
            continue
        if ":" not in line:
            # FreeSWITCH event content is header-like.  Keep malformed lines
            # visible rather than discarding the whole packet.
            headers.setdefault("(unparsed)", line)
            continue
        key, value = line.split(":", 1)
        headers[key.strip()] = value.lstrip(" \t")
    return headers, body


@dataclass(frozen=True)
class EslEvent:
    """An event parsed from one ESL frame.

    ``raw`` is the exact frame as received (frame headers plus payload), while
    ``headers`` contains the event's own headers.  For JSON events the JSON
    members are exposed as headers and the pretty-printed JSON is placed in
    ``body`` for convenient inspection.
    """

    raw: bytes
    received_at: float
    content_type: str
    headers: dict[str, str]
    body: str
    event_name: str
    subclass: str = ""
    parse_error: str = ""

    def header(self, name: str, default: str = "") -> str:
        """Return an event header value, ignoring case."""

        wanted = name.casefold()
        for key, value in self.headers.items():
            if key.casefold() == wanted:
                return value
        return default

    @property
    def display_name(self) -> str:
        if self.subclass:
            return f"{self.event_name}:{self.subclass}"
        return self.event_name

    @property
    def timestamp_iso(self) -> str:
        return time.strftime(
            "%Y-%m-%dT%H:%M:%S%z",
            time.localtime(self.received_at),
        )

    @classmethod
    def from_frame(cls, frame: EslFrame) -> EslEvent:
        received_at = time.time()
        content_type = frame.content_type

        if content_type == "text/event-json":
            return cls._from_json_frame(frame, received_at)

        text = frame.body_text
        headers, body = _parse_plain_event(text)
        return cls(
            raw=frame.raw,
            received_at=received_at,
            content_type=content_type or "text/event-plain",
            headers=headers,
            body=body,
            event_name=_header(headers, "Event-Name") or "(unknown event)",
            subclass=_header(headers, "Event-Subclass"),
        )

    @classmethod
    def _from_json_frame(cls, frame: EslFrame, received_at: float) -> EslEvent:
        text = frame.body_text
        try:
            payload = json.loads(text)
            if not isinstance(payload, dict):
                raise ValueError("JSON event is not an object")
        except (ValueError, TypeError) as exc:
            return cls(
                raw=frame.raw,
                received_at=received_at,
                content_type=frame.content_type,
                headers={},
                body=text,
                event_name="(JSON parse error)",
                parse_error=str(exc),
            )

        headers = {
            str(key): _stringify_json_value(value)
            for key, value in payload.items()
        }
        pretty = json.dumps(payload, ensure_ascii=False, indent=2)
        return cls(
            raw=frame.raw,
            received_at=received_at,
            content_type=frame.content_type,
            headers=headers,
            body=pretty,
            event_name=_header(headers, "Event-Name") or "(unknown event)",
            subclass=_header(headers, "Event-Subclass"),
        )


class EslClient:
    """Inbound ESL client using FreeSWITCH's plain-text event socket protocol."""

    def __init__(
        self,
        reader: asyncio.StreamReader,
        writer: asyncio.StreamWriter,
        host: str,
        port: int,
    ) -> None:
        self._reader = reader
        self._writer = writer
        self.host = host
        self.port = port
        self._closed = False

    @classmethod
    async def connect(
        cls,
        host: str = "127.0.0.1",
        port: int = 8021,
        password: str = "ClueCon",
        timeout: float = 5.0,
    ) -> EslClient:
        if "\n" in host or "\r" in host:
            raise ValueError("host must not contain newlines")
        if "\n" in password or "\r" in password:
            raise ValueError("password must not contain newlines")
        if not 1 <= int(port) <= 65535:
            raise ValueError("port must be between 1 and 65535")

        try:
            reader, writer = await asyncio.wait_for(
                asyncio.open_connection(
                    host,
                    int(port),
                    limit=MAX_HEADER_BYTES,
                ),
                timeout=timeout,
            )
        except asyncio.TimeoutError as exc:
            raise EslConnectionError(
                f"Connection to {host}:{port} timed out"
            ) from exc
        except OSError as exc:
            raise EslConnectionError(
                f"Connection to {host}:{port} failed: {exc}"
            ) from exc

        client = cls(reader, writer, host, int(port))
        try:
            greeting = await client._read_frame(timeout=timeout)
            if greeting.content_type == "text/disconnect-notice":
                raise EslDisconnected(
                    greeting.body_text.strip()
                    or "The server refused the connection"
                )
            if greeting.content_type != "auth/request":
                raise EslProtocolError(
                    "Unexpected first frame: "
                    f"{greeting.content_type or '(none)'}"
                )

            reply = await client._send_command(f"auth {password}", timeout=timeout)
            reply_text = reply.header("Reply-Text")
            if not reply_text.startswith("+OK"):
                raise EslAuthError(
                    "ESL authentication failed: "
                    + (unquote(reply_text) or "no response")
                )
        except BaseException:
            await client.close()
            raise

        return client

    async def subscribe(self, events: tuple[str, ...] | list[str]) -> None:
        """Subscribe to events.

        Each item may be a plain event name (``CHANNEL_CREATE``) or a
        ``CUSTOM`` subclass pair (``CUSTOM callcenter::info``).  Event names
        are upper-cased for FreeSWITCH, while a custom subclass keeps its
        original case because FreeSWITCH treats it case-sensitively.
        """

        if not events:
            raise ValueError("at least one event must be selected")

        names: list[str] = []
        for item in events:
            tokens = str(item).split()
            if not tokens:
                continue
            for index, token in enumerate(tokens):
                # The first token is the event name; any following token is a
                # CUSTOM subclass.
                name = token if index else token.upper()
                if name not in names:
                    names.append(name)
        if not names:
            raise ValueError("at least one event must be selected")

        # ALL makes individual names redundant and keeps the command short.
        if "ALL" in names:
            names = ["ALL"]

        reply = await self._send_command("event json " + " ".join(names))
        reply_text = reply.header("Reply-Text")
        if not reply_text.startswith("+OK"):
            raise EslProtocolError(
                "Event subscription failed: "
                + (unquote(reply_text) or "no response")
            )

    async def events(self) -> AsyncIterator[EslEvent]:
        while True:
            frame = await self._read_frame()
            content_type = frame.content_type

            if content_type.startswith("text/event-"):
                yield EslEvent.from_frame(frame)
                continue

            if content_type == "text/disconnect-notice":
                message = (
                    frame.body_text.strip()
                    or "The server closed the connection"
                )
                raise EslDisconnected(message)

            if content_type in {
                "command/reply",
                "api/response",
                "log/data",
                "text/event-plain",
                "text/event-json",
                "",
            }:
                # Command/API frames are consumed during the handshake.  If
                # FreeSWITCH sends a keepalive frame, ignore it.
                continue

            # Unknown text frames are not events, but shouldn't kill the
            # session.  They are intentionally ignored.

    async def _send_command(
        self,
        command: str,
        *,
        timeout: float = 5.0,
    ) -> EslFrame:
        if "\n" in command or "\r" in command:
            raise ValueError("ESL commands must be a single line")
        if self._closed:
            raise EslDisconnected("The connection is closed")

        self._writer.write(command.encode("utf-8") + b"\n\n")
        await self._writer.drain()
        return await self._read_frame(timeout=timeout)

    async def _read_frame(self, timeout: float | None = None) -> EslFrame:
        if timeout is None:
            return await self._read_frame_inner()
        try:
            return await asyncio.wait_for(
                self._read_frame_inner(),
                timeout=timeout,
            )
        except asyncio.TimeoutError as exc:
            raise EslTimeoutError(
                "Timed out waiting for an ESL response"
            ) from exc

    async def _read_frame_inner(self) -> EslFrame:
        header_bytes = bytearray()
        lines: list[bytes] = []

        while True:
            try:
                line = await self._reader.readline()
            except (ValueError, asyncio.LimitOverrunError) as exc:
                raise EslProtocolError("ESL headers are too long") from exc
            except (ConnectionError, OSError) as exc:
                raise EslDisconnected(f"Connection lost: {exc}") from exc

            if not line:
                raise EslDisconnected("FreeSWITCH closed the connection")

            if line in (b"\n", b"\r\n"):
                # Tolerate an empty line between frames, but don't treat it as
                # a frame unless it follows real headers.
                if not lines:
                    continue
                header_bytes.extend(line)
                break

            lines.append(line)
            header_bytes.extend(line)
            if len(header_bytes) > MAX_HEADER_BYTES:
                raise EslProtocolError("ESL headers are too long")

        headers = _parse_header_block(b"".join(lines))
        length_text = _header(headers, "Content-Length", "0").strip()
        try:
            content_length = int(length_text or "0")
        except ValueError as exc:
            raise EslProtocolError(
                f"Invalid Content-Length: {length_text!r}"
            ) from exc

        if content_length < 0 or content_length > MAX_BODY_BYTES:
            raise EslProtocolError(
                f"Content-Length out of range: {content_length}"
            )

        try:
            body = (
                await self._reader.readexactly(content_length)
                if content_length
                else b""
            )
        except asyncio.IncompleteReadError as exc:
            raise EslDisconnected(
                "The connection closed while reading the event body"
            ) from exc
        except (ConnectionError, OSError) as exc:
            raise EslDisconnected(f"Connection lost: {exc}") from exc

        return EslFrame(
            headers=headers,
            body=body,
            raw=bytes(header_bytes) + body,
        )

    async def close(self) -> None:
        if self._closed:
            return
        self._closed = True
        self._writer.close()
        try:
            await asyncio.wait_for(self._writer.wait_closed(), timeout=1.0)
        except (asyncio.TimeoutError, ConnectionError, OSError):
            pass
