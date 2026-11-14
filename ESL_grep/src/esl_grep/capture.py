"""Serialization helpers for captured ESL event packets."""

from __future__ import annotations

import json
import os
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from .esl import EslEvent

RAW_FORMAT = "raw"
JSONL_FORMAT = "jsonl"
JSON_FORMAT = "json"
CAPTURE_FORMATS = (JSON_FORMAT, RAW_FORMAT, JSONL_FORMAT)


def encode_capture(events: Iterable[EslEvent], fmt: str) -> bytes:
    """Encode captured events into the requested file format."""

    event_list = list(events)
    if fmt == RAW_FORMAT:
        # These are complete ESL frames, so concatenating them is a faithful
        # capture of the event stream (Content-Length delimits each frame).
        return b"".join(event.raw for event in event_list)

    if fmt == JSON_FORMAT:
        # Store only the JSON shown in the event detail pane.  The result is a
        # single JSON array so the file remains a well-formed JSON document.
        records: list[Any] = []
        for event in event_list:
            body = event.body.strip()
            try:
                records.append(json.loads(body) if body else None)
            except (ValueError, TypeError):
                records.append(body)
        return (
            json.dumps(records, ensure_ascii=False, indent=2) + "\n"
        ).encode("utf-8")

    if fmt == JSONL_FORMAT:
        lines: list[str] = []
        for event in event_list:
            record = {
                "received_at": event.timestamp_iso,
                "event": event.event_name,
                "subclass": event.subclass,
                "content_type": event.content_type,
                "headers": event.headers,
                "body": event.body,
                "raw": event.raw.decode("utf-8", errors="replace"),
            }
            if event.parse_error:
                record["parse_error"] = event.parse_error
            lines.append(json.dumps(record, ensure_ascii=False))
        return ("\n".join(lines) + ("\n" if lines else "")).encode("utf-8")

    raise ValueError(f"unknown capture format: {fmt!r}")


def write_capture(path: Path, events: Iterable[EslEvent], fmt: str) -> int:
    """Write a capture and return the number of bytes written."""

    if fmt not in CAPTURE_FORMATS:
        raise ValueError(f"unknown capture format: {fmt!r}")

    payload = encode_capture(events, fmt)
    path = path.expanduser()
    path.parent.mkdir(parents=True, exist_ok=True)

    flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
    fd = os.open(path, flags, 0o600)
    with os.fdopen(fd, "wb") as handle:
        handle.write(payload)
    return len(payload)
