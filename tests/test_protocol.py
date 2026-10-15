import re
import time

import pytest

LEDS = ("RED", "GREEN", "BLUE")


def test_ping(dut):
    assert dut.command("PING") == "PONG"


def test_version_is_semver(dut):
    assert re.fullmatch(r"VERSION hil-bench-fw \d+\.\d+\.\d+", dut.command("VERSION"))


def test_all_leds_start_off(dut):
    assert dut.command("LEDS") == "LEDS RED=0 GREEN=0 BLUE=0"


@pytest.mark.parametrize("led", LEDS)
def test_led_on_is_reported(dut, led):
    assert dut.command(f"LED {led} ON") == "OK"
    expected = " ".join(f"{name}={int(name == led)}" for name in LEDS)
    assert dut.command("LEDS") == f"LEDS {expected}"


@pytest.mark.parametrize("led", LEDS)
def test_led_off_after_on(dut, led):
    dut.command(f"LED {led} ON")
    assert dut.command(f"LED {led} OFF") == "OK"
    assert dut.command("LEDS") == "LEDS RED=0 GREEN=0 BLUE=0"


def test_uptime_advances(dut):
    first = int(dut.command("UPTIME").split()[1])
    time.sleep(0.2)
    second = int(dut.command("UPTIME").split()[1])
    assert 150 <= second - first <= 400


def test_ping_round_trip_under_50_ms(dut):
    start = time.perf_counter()
    dut.command("PING")
    assert time.perf_counter() - start < 0.05


@pytest.mark.parametrize(
    ("request_line", "expected"),
    [
        ("FOO", "ERR UNKNOWN_COMMAND"),
        ("ping", "ERR UNKNOWN_COMMAND"),
        ("PING extra", "ERR BAD_ARGS"),
        ("LED PURPLE ON", "ERR BAD_ARGS"),
        ("LED RED", "ERR BAD_ARGS"),
        ("LED RED MAYBE", "ERR BAD_ARGS"),
        ("LED RED ON NOW EXTRA", "ERR BAD_ARGS"),
        ("", "ERR EMPTY"),
        ("   ", "ERR EMPTY"),
        ("X" * 65, "ERR LINE_TOO_LONG"),
    ],
)
def test_error_responses(dut, request_line, expected):
    assert dut.command(request_line) == expected


def test_device_still_responds_after_errors(dut):
    dut.command("X" * 200)
    dut.command("GARBAGE")
    assert dut.command("PING") == "PONG"
