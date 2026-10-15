"""Python model of the firmware command protocol (see firmware/include/protocol.h).

Keep this in sync with firmware/src/protocol.c: the same test suite runs
against both, so any divergence shows up as a test that passes in simulation
and fails on hardware.
"""

from __future__ import annotations

import time

FW_VERSION = "0.1.0"
MAX_LINE = 64
LEDS = ("RED", "GREEN", "BLUE")


class SimFirmware:
    def __init__(self) -> None:
        self._start = time.monotonic()
        self._leds = {name: False for name in LEDS}

    def handle_line(self, line: str) -> str:
        if len(line) > MAX_LINE:
            return "ERR LINE_TOO_LONG"

        # The firmware splits on spaces only (not tabs), so mirror that exactly.
        tokens = [t for t in line.split(" ") if t]
        if not tokens:
            return "ERR EMPTY"
        if len(tokens) > 4:
            return "ERR BAD_ARGS"

        cmd, args = tokens[0], tokens[1:]

        if cmd == "PING":
            return "PONG" if not args else "ERR BAD_ARGS"
        if cmd == "VERSION":
            return f"VERSION hil-bench-fw {FW_VERSION}" if not args else "ERR BAD_ARGS"
        if cmd == "LED":
            if len(args) != 2 or args[0] not in LEDS or args[1] not in ("ON", "OFF"):
                return "ERR BAD_ARGS"
            self._leds[args[0]] = args[1] == "ON"
            return "OK"
        if cmd == "LEDS":
            if args:
                return "ERR BAD_ARGS"
            return "LEDS " + " ".join(f"{name}={int(self._leds[name])}" for name in LEDS)
        if cmd == "UPTIME":
            if args:
                return "ERR BAD_ARGS"
            return f"UPTIME {int((time.monotonic() - self._start) * 1000) & 0xFFFFFFFF}"
        return "ERR UNKNOWN_COMMAND"
