import pytest

from hil.dut import SerialDut, SimDut


def pytest_addoption(parser):
    parser.addoption(
        "--dut",
        choices=("sim", "serial"),
        default="sim",
        help="run against the simulator (default) or real hardware over serial",
    )
    parser.addoption(
        "--port",
        default="/dev/ttyACM0",
        help="serial port of the DUT when --dut=serial",
    )


@pytest.fixture(scope="session")
def dut(request):
    if request.config.getoption("--dut") == "serial":
        connection = SerialDut(request.config.getoption("--port"))
    else:
        connection = SimDut()
    yield connection
    connection.close()


@pytest.fixture(autouse=True)
def leds_off(dut):
    """Leave every test starting and ending with all LEDs off."""
    for led in ("RED", "GREEN", "BLUE"):
        dut.command(f"LED {led} OFF")
    yield
    for led in ("RED", "GREEN", "BLUE"):
        dut.command(f"LED {led} OFF")
