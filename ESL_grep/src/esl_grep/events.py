"""FreeSWITCH event names exposed by the event socket.

The list mirrors ``EVENT_NAMES`` in FreeSWITCH's ``switch_event.c``.  The
special ``ALL`` entry is kept first because it is the default subscription.
"""

from __future__ import annotations

ALL_EVENTS = "ALL"

# mod_callcenter emits its internal queue/agent/tier events as a custom event
# with the ``callcenter::info`` subclass.  FreeSWITCH's event command wants the
# two tokens together: ``event json CUSTOM callcenter::info``.
CALLCENTER_EVENT = "CUSTOM"
CALLCENTER_SUBCLASS = "callcenter::info"
CALLCENTER_SUBSCRIPTION = f"{CALLCENTER_EVENT} {CALLCENTER_SUBCLASS}"

# Keep this list in the same order as FreeSWITCH's switch_event.c so it is
# easy to update when FreeSWITCH gains new core events.
EVENT_NAMES: tuple[str, ...] = (
    ALL_EVENTS,
    "CUSTOM",
    "CLONE",
    "CHANNEL_CREATE",
    "CHANNEL_DESTROY",
    "CHANNEL_STATE",
    "CHANNEL_CALLSTATE",
    "CHANNEL_ANSWER",
    "CHANNEL_HANGUP",
    "CHANNEL_HANGUP_COMPLETE",
    "CHANNEL_EXECUTE",
    "CHANNEL_EXECUTE_COMPLETE",
    "CHANNEL_HOLD",
    "CHANNEL_UNHOLD",
    "CHANNEL_BRIDGE",
    "CHANNEL_UNBRIDGE",
    "CHANNEL_PROGRESS",
    "CHANNEL_PROGRESS_MEDIA",
    "CHANNEL_OUTGOING",
    "CHANNEL_PARK",
    "CHANNEL_UNPARK",
    "CHANNEL_APPLICATION",
    "CHANNEL_ORIGINATE",
    "CHANNEL_UUID",
    "API",
    "LOG",
    "INBOUND_CHAN",
    "OUTBOUND_CHAN",
    "STARTUP",
    "SHUTDOWN",
    "PUBLISH",
    "UNPUBLISH",
    "TALK",
    "NOTALK",
    "SESSION_CRASH",
    "MODULE_LOAD",
    "MODULE_UNLOAD",
    "DTMF",
    "MESSAGE",
    "PRESENCE_IN",
    "NOTIFY_IN",
    "PRESENCE_OUT",
    "PRESENCE_PROBE",
    "MESSAGE_WAITING",
    "MESSAGE_QUERY",
    "ROSTER",
    "CODEC",
    "BACKGROUND_JOB",
    "DETECTED_SPEECH",
    "DETECTED_TONE",
    "PRIVATE_COMMAND",
    "HEARTBEAT",
    "TRAP",
    "ADD_SCHEDULE",
    "DEL_SCHEDULE",
    "EXE_SCHEDULE",
    "RE_SCHEDULE",
    "RELOADXML",
    "NOTIFY",
    "PHONE_FEATURE",
    "PHONE_FEATURE_SUBSCRIBE",
    "SEND_MESSAGE",
    "RECV_MESSAGE",
    "REQUEST_PARAMS",
    "CHANNEL_DATA",
    "GENERAL",
    "COMMAND",
    "SESSION_HEARTBEAT",
    "CLIENT_DISCONNECTED",
    "SERVER_DISCONNECTED",
    "SEND_INFO",
    "RECV_INFO",
    "RECV_RTCP_MESSAGE",
    "SEND_RTCP_MESSAGE",
    "CALL_SECURE",
    "NAT",
    "RECORD_START",
    "RECORD_STOP",
    "PLAYBACK_START",
    "PLAYBACK_STOP",
    "CALL_UPDATE",
    "FAILURE",
    "SOCKET_DATA",
    "MEDIA_BUG_START",
    "MEDIA_BUG_STOP",
    "CONFERENCE_DATA_QUERY",
    "CONFERENCE_DATA",
    "CALL_SETUP_REQ",
    "CALL_SETUP_RESULT",
    "CALL_DETAIL",
    "DEVICE_STATE",
    "TEXT",
    "SHUTDOWN_REQUESTED",
    "CERT_RELOAD",
)

# A small, useful subset used when the user asks for the call-oriented events.
CALL_EVENTS: tuple[str, ...] = (
    "CHANNEL_CREATE",
    "CHANNEL_ANSWER",
    "CHANNEL_HANGUP",
    "CHANNEL_HANGUP_COMPLETE",
    "CHANNEL_BRIDGE",
    "CHANNEL_UNBRIDGE",
    "CHANNEL_STATE",
    "CHANNEL_CALLSTATE",
    "CHANNEL_EXECUTE",
    "CHANNEL_EXECUTE_COMPLETE",
    "CHANNEL_PARK",
    "CHANNEL_UNPARK",
    "DTMF",
    "RECORD_START",
    "RECORD_STOP",
    "PLAYBACK_START",
    "PLAYBACK_STOP",
)

_EVENT_NAME_SET = frozenset(EVENT_NAMES)


def is_known_event(name: str) -> bool:
    return name in _EVENT_NAME_SET


def is_callcenter_event(event_name: str, subclass: str) -> bool:
    """Return whether an event came from mod_callcenter.

    mod_callcenter publishes every queue/agent/tier update as a ``CUSTOM``
    event with the ``callcenter::info`` subclass.
    """

    return (
        event_name.strip().upper() == CALLCENTER_EVENT
        and subclass.strip().casefold() == CALLCENTER_SUBCLASS.casefold()
    )
