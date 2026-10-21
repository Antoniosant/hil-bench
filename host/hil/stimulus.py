"""Host side of the GPIO loop: drive the board's STIM pin, watch RESP come back.

Wiring on the bench (Raspberry Pi 4 header -> TM4C123G LaunchPad):

    Pi pin 11 (GPIO17)  ->  PB4  (STIM)
    Pi pin 13 (GPIO27)  <-  PB5  (RESP)
    Pi pin 9  (GND)     --  GND

Both sides are 3.3 V logic, so no level shifting is needed.
"""

from __future__ import annotations

import time
from datetime import timedelta

STIM_LINE = 17
RESP_LINE = 27


class StimulusTimeout(Exception):
    """RESP did not follow STIM within the timeout."""


class GpiodStimulus:
    """Real wires, through the Linux GPIO character device (libgpiod v2)."""

    def __init__(self, chip: str = "/dev/gpiochip0", stim_line: int = STIM_LINE, resp_line: int = RESP_LINE):
        import gpiod  # imported lazily so simulator runs need no libgpiod
        from gpiod.line import Direction, Edge, Value

        self._gpiod = gpiod
        self._value = Value
        self._stim = stim_line
        self.level = False
        self._request = gpiod.request_lines(
            chip,
            consumer="hil-bench",
            config={
                stim_line: gpiod.LineSettings(direction=Direction.OUTPUT, output_value=Value.INACTIVE),
                resp_line: gpiod.LineSettings(direction=Direction.INPUT, edge_detection=Edge.BOTH),
            },
        )

    def drive(self, level: bool, timeout: float = 0.1) -> int:
        """Change STIM and wait for RESP to follow. Returns the reaction time in ns.

        level must differ from the current one: no edge, nothing to wait for.

        The start time is taken in user space just before the write and the end
        time is the kernel's timestamp of the RESP edge, both on CLOCK_MONOTONIC,
        so the figure includes the Pi's own write latency, not just the board's.
        """
        self._drain()
        start = time.monotonic_ns()
        self._request.set_value(self._stim, self._value.ACTIVE if level else self._value.INACTIVE)
        self.level = level

        wanted = (
            self._gpiod.EdgeEvent.Type.RISING_EDGE if level else self._gpiod.EdgeEvent.Type.FALLING_EDGE
        )
        deadline = time.monotonic() + timeout
        while (remaining := deadline - time.monotonic()) > 0:
            if not self._request.wait_edge_events(timedelta(seconds=remaining)):
                break
            for event in self._request.read_edge_events():
                if event.event_type == wanted:
                    return event.timestamp_ns - start
        raise StimulusTimeout(f"RESP did not go {'high' if level else 'low'} within {timeout} s")

    def _drain(self) -> None:
        """Discard edge events left over from before this stimulus."""
        while self._request.wait_edge_events(timedelta(0)):
            self._request.read_edge_events()

    def close(self) -> None:
        self._request.release()


class SimStimulus:
    """The same wire, modelled against the in-process firmware simulator."""

    def __init__(self, firmware) -> None:
        self._fw = firmware

    @property
    def level(self) -> bool:
        return self._fw.stim

    def drive(self, level: bool, timeout: float = 0.1) -> int:
        self._fw.set_stim(level)
        if self._fw.resp != level:
            raise StimulusTimeout(f"RESP did not go {'high' if level else 'low'}")
        return 0

    def close(self) -> None:
        pass
