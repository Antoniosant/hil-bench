"""Connections to the device under test (DUT).

Both classes expose the same `command()` call, so every test runs unchanged
against the real board over serial or against the in-process simulator.
"""

from __future__ import annotations

import time

from hil.sim import SimFirmware


class DutTimeout(Exception):
    """The DUT did not answer within the timeout."""


class SerialDut:
    """Real hardware reached through a serial port (e.g. /dev/ttyACM0)."""

    def __init__(self, port: str, baudrate: int = 115200, timeout: float = 1.0):
        import serial  # imported lazily so simulator runs need no pyserial

        self._serial = serial.Serial(port, baudrate=baudrate, timeout=timeout)
        # Drop the boot banner or any partial output from before we connected.
        time.sleep(0.1)
        self._serial.reset_input_buffer()

    @classmethod
    def connect(cls, port: str, timeout: float = 10.0) -> "SerialDut":
        """Open the port and wait until the firmware answers PING.

        Right after flashing, the debugger's USB serial port can disappear and
        come back while the board resets, so failures to open or read the port
        are retried, reopening it each time, until the timeout.
        """
        import serial

        deadline = time.monotonic() + timeout
        last_error: Exception | None = None
        while time.monotonic() < deadline:
            dut = None
            try:
                dut = cls(port)
                if dut.command("PING") == "PONG":
                    return dut
            except (serial.SerialException, OSError, DutTimeout) as exc:
                last_error = exc
            if dut is not None:
                dut.close()
            time.sleep(0.2)
        raise DutTimeout(f"{port}: no PONG within {timeout} s (last error: {last_error})")

    def command(self, line: str) -> str:
        self._serial.write((line + "\n").encode("ascii"))
        raw = self._serial.readline()
        if not raw.endswith(b"\n"):
            raise DutTimeout(f"no response to {line!r} (got {raw!r})")
        return raw.decode("ascii").strip()

    def close(self) -> None:
        self._serial.close()


class SimDut:
    """In-process model of the firmware protocol, for runs without hardware."""

    def __init__(self) -> None:
        self.firmware = SimFirmware()

    def command(self, line: str) -> str:
        return self.firmware.handle_line(line)

    def close(self) -> None:
        pass
