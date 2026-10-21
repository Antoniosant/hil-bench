import subprocess
from pathlib import Path

import pytest

from hil.dut import SerialDut, SimDut
from hil.stimulus import GpiodStimulus, SimStimulus

FLASH_SCRIPT = Path(__file__).resolve().parent.parent / "tools" / "flash_tm4c123.sh"


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
    parser.addoption(
        "--flash",
        metavar="ELF",
        default=None,
        help="flash this firmware image to the board before the run (--dut=serial only)",
    )
    parser.addoption(
        "--gpio",
        action="store_true",
        help="the Pi's GPIOs are wired to the board, so run the GPIO-loop tests (--dut=serial)",
    )


@pytest.fixture(scope="session")
def dut(request):
    if request.config.getoption("--dut") == "serial":
        elf = request.config.getoption("--flash")
        if elf is not None:
            subprocess.run([str(FLASH_SCRIPT), elf], check=True)
        connection = SerialDut.connect(request.config.getoption("--port"))
    else:
        connection = SimDut()
    yield connection
    connection.close()


@pytest.fixture(scope="session")
def stimulus(request, dut):
    if isinstance(dut, SimDut):
        wire = SimStimulus(dut.firmware)
    elif request.config.getoption("--gpio"):
        wire = GpiodStimulus()
    else:
        pytest.skip("GPIO-loop tests need the wiring and --gpio")
    yield wire
    wire.close()


@pytest.fixture
def stim(stimulus):
    """The stimulus line, starting and ending each test low."""
    if stimulus.level:
        stimulus.drive(False)
    yield stimulus
    if stimulus.level:
        stimulus.drive(False)


@pytest.fixture(autouse=True)
def leds_off(dut):
    """Leave every test starting and ending with all LEDs off."""
    for led in ("RED", "GREEN", "BLUE"):
        dut.command(f"LED {led} OFF")
    yield
    for led in ("RED", "GREEN", "BLUE"):
        dut.command(f"LED {led} OFF")
