"""Configuration and small on-disk persistence helpers."""

from __future__ import annotations

import json
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .events import (
    ALL_EVENTS,
    CALLCENTER_SUBSCRIPTION,
    EVENT_NAMES,
    is_callcenter_event,
)


@dataclass(frozen=True)
class EslConfig:
    """Connection settings used by the app."""

    host: str = "127.0.0.1"
    port: int = 8021
    password: str = "ClueCon"
    events: tuple[str, ...] = (ALL_EVENTS,)
    callcenter_only: bool = False

    def __post_init__(self) -> None:
        normalized: list[str] = []
        for item in self.events:
            name = str(item).strip().upper()
            if name and name not in normalized:
                normalized.append(name)
        object.__setattr__(
            self, "events", tuple(normalized) or (ALL_EVENTS,)
        )
        object.__setattr__(self, "callcenter_only", bool(self.callcenter_only))

    @property
    def server(self) -> str:
        return f"{self.host}:{self.port}"

    @property
    def all_events(self) -> bool:
        return ALL_EVENTS in self.events

    def subscription_events(self) -> tuple[str, ...]:
        """Return the subscription arguments sent to FreeSWITCH.

        ``callcenter_only`` takes precedence over the regular event selection
        and asks for mod_callcenter's custom event only.  ``ALL`` wins over
        individual names; this avoids sending a needlessly long ``event``
        command.
        """

        if self.callcenter_only:
            return (CALLCENTER_SUBSCRIPTION,)
        if self.all_events:
            return (ALL_EVENTS,)
        return self.events

    def accepts_event(self, event_name: str, subclass: str = "") -> bool:
        """Return whether an incoming event matches the client-side filter."""

        if self.callcenter_only:
            return is_callcenter_event(event_name, subclass)
        if self.all_events:
            return True
        return event_name in self.events

    def with_events(self, events: list[str] | tuple[str, ...]) -> EslConfig:
        return EslConfig(
            host=self.host,
            port=self.port,
            password=self.password,
            events=tuple(events),
            callcenter_only=self.callcenter_only,
        )


DEFAULT_CONFIG = EslConfig()


def default_config_path() -> Path:
    """Return the per-user config path, honouring XDG_CONFIG_HOME."""

    xdg = os.environ.get("XDG_CONFIG_HOME", "").strip()
    base = Path(xdg) if xdg else Path.home() / ".config"
    return base / "esl_grep" / "config.json"


def _clean_events(value: Any) -> tuple[str, ...]:
    if not isinstance(value, list):
        return (ALL_EVENTS,)

    cleaned: list[str] = []
    for item in value:
        if not isinstance(item, str):
            continue
        name = item.strip().upper()
        if name and name not in cleaned:
            cleaned.append(name)
    return tuple(cleaned) or (ALL_EVENTS,)


def load_config(path: Path | None = None) -> EslConfig:
    """Load settings, falling back to safe defaults on any bad input."""

    config_path = path or default_config_path()
    try:
        raw = json.loads(config_path.read_text(encoding="utf-8"))
    except (OSError, ValueError, TypeError):
        return DEFAULT_CONFIG

    if not isinstance(raw, dict):
        return DEFAULT_CONFIG

    host = raw.get("host", DEFAULT_CONFIG.host)
    if not isinstance(host, str) or not host.strip():
        host = DEFAULT_CONFIG.host
    else:
        host = host.strip()

    try:
        port = int(raw.get("port", DEFAULT_CONFIG.port))
    except (TypeError, ValueError):
        port = DEFAULT_CONFIG.port
    if not 1 <= port <= 65535:
        port = DEFAULT_CONFIG.port

    password = raw.get("password", DEFAULT_CONFIG.password)
    if not isinstance(password, str):
        password = DEFAULT_CONFIG.password

    callcenter_only = raw.get(
        "callcenter_only", DEFAULT_CONFIG.callcenter_only
    )

    return EslConfig(
        host=host,
        port=port,
        password=password,
        events=_clean_events(raw.get("events", [ALL_EVENTS])),
        callcenter_only=callcenter_only is True,
    )


def save_config(config: EslConfig, path: Path | None = None) -> None:
    """Persist settings atomically with owner-only permissions."""

    config_path = path or default_config_path()
    config_path.parent.mkdir(parents=True, exist_ok=True)

    payload = {
        "version": 1,
        "host": config.host,
        "port": config.port,
        "password": config.password,
        "events": list(config.events),
        "callcenter_only": config.callcenter_only,
    }

    temp_path = config_path.with_name(f".{config_path.name}.tmp")
    temp_path.write_text(
        json.dumps(payload, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    try:
        os.chmod(temp_path, 0o600)
    except OSError:
        pass
    temp_path.replace(config_path)


def event_names_for_ui(extra: tuple[str, ...] = ()) -> tuple[str, ...]:
    """Return the built-in list plus any valid names from an older config."""

    names = list(EVENT_NAMES)
    for name in extra:
        if name not in names:
            names.append(name)
    return tuple(names)
