"""GPIO in the loop: the host drives STIM into the board and watches RESP."""

import re
import statistics

import pytest

GPIO_STATUS = re.compile(r"GPIO STIM=([01]) RESP=([01]) EDGES=(\d+)")

# Provisional limit, to be tightened from measured data. The figure includes
# the Raspberry Pi's own user-space and kernel latency, which dominates the
# firmware's few microseconds of interrupt latency.
MAX_P99_LATENCY_US = 1000


def gpio_status(dut):
    match = GPIO_STATUS.fullmatch(dut.command("GPIO"))
    assert match, "malformed GPIO status"
    stim, resp, edges = match.groups()
    return stim == "1", resp == "1", int(edges)


def test_gpio_status_format(dut):
    gpio_status(dut)


def test_gpio_rejects_arguments(dut):
    assert dut.command("GPIO NOW") == "ERR BAD_ARGS"


@pytest.mark.parametrize("level", [True, False])
def test_resp_follows_stim(dut, stim, level):
    if not level:
        stim.drive(True)
    stim.drive(level)
    assert gpio_status(dut)[:2] == (level, level)


def test_every_edge_is_counted(dut, stim):
    edges = 200
    before = gpio_status(dut)[2]
    for i in range(edges):
        stim.drive(i % 2 == 0)
    after = gpio_status(dut)[2]
    assert after - before == edges


def test_reaction_latency(stim, record_property):
    samples_us = []
    for i in range(500):
        samples_us.append(stim.drive(i % 2 == 0) / 1000)

    samples_us.sort()
    p50 = statistics.median(samples_us)
    p99 = samples_us[int(len(samples_us) * 0.99) - 1]
    worst = samples_us[-1]
    for name, value in (("latency_p50_us", p50), ("latency_p99_us", p99), ("latency_max_us", worst)):
        record_property(name, round(value, 1))
    print(f"\nreaction latency over {len(samples_us)} edges: "
          f"p50 {p50:.1f} us, p99 {p99:.1f} us, max {worst:.1f} us")

    assert p99 < MAX_P99_LATENCY_US
