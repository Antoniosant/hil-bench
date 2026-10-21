# hil-bench

A small Hardware-in-the-Loop (HIL) test bench: bare-metal C firmware on an ARM
Cortex-M board, exercised automatically from a Raspberry Pi with pytest.

```
 ┌────────────────┐   UART over USB (commands)   ┌──────────────────────────┐
 │  Raspberry Pi  │◄────────────────────────────►│  TM4C123G LaunchPad      │
 │  pytest        │                              │  Cortex-M4, bare-metal C │
 │  (controller)  │   SWD via on-board ICDI      │  (device under test)     │
 │                │─────────────────────────────►│                          │
 │                │   (flashing)                 │                          │
 │                │                              │                          │
 │  GPIO17 ───────┼──── STIM ───────────────────►│ PB4  pin-change IRQ      │
 │  GPIO27 ◄──────┼──── RESP ────────────────────┤ PB5  copies STIM         │
 └────────────────┘                              └──────────────────────────┘
```

The same test suite runs against the real board (`--dut serial`) or against a
Python model of the firmware (`--dut sim`), so it also runs in CI without
hardware.

## Layout

| Path | Contents |
|------|----------|
| `firmware/src/` | Board-independent code: main loop and the command protocol |
| `firmware/include/` | `board.h` (board abstraction) and `protocol.h` (protocol spec) |
| `firmware/boards/tm4c123/` | Register-level drivers (GPIO, UART, SysTick), startup code and linker script |
| `host/hil/` | DUT connections (serial and simulator), GPIO stimulus (libgpiod and simulated) and the firmware model |
| `tests/` | pytest suite |
| `tools/` | Flashing script |

## Protocol

ASCII lines terminated by `\n`, one response per request:

| Request | Response |
|---------|----------|
| `PING` | `PONG` |
| `VERSION` | `VERSION hil-bench-fw 0.2.0` |
| `LED <RED\|GREEN\|BLUE> <ON\|OFF>` | `OK` |
| `LEDS` | `LEDS RED=0 GREEN=1 BLUE=0` |
| `UPTIME` | `UPTIME <ms>` |
| `GPIO` | `GPIO STIM=1 RESP=1 EDGES=42` |

Errors: `ERR EMPTY`, `ERR UNKNOWN_COMMAND`, `ERR BAD_ARGS`, `ERR LINE_TOO_LONG`.

## GPIO in the loop

The Pi drives the board's STIM input. A pin-change interrupt on the board
copies the level to RESP and counts every edge. The tests check that RESP
follows STIM, that no edge is missed, and measure the reaction time from the
Pi's write to the kernel timestamp of the RESP edge (reported in the JUnit XML
as `latency_p50_us`, `latency_p99_us` and `latency_max_us`).

| Raspberry Pi 4 header | LaunchPad | Signal |
|-----------------------|-----------|--------|
| pin 11 (GPIO17) | PB4 | STIM, Pi to board |
| pin 13 (GPIO27) | PB5 | RESP, board to Pi |
| pin 9 (GND) | GND | common ground |

Both sides use 3.3 V logic. Never wire the Pi's 5 V pins to a GPIO.

The measured latency includes the Pi's own user-space and kernel overhead,
which is far larger than the firmware's interrupt latency (a few microseconds
at 16 MHz). The figure tells you the bench works and catches regressions such
as moving the copy from the interrupt to the main loop; a logic analyser on
the two pins would isolate the firmware's share.

## Quick start

### Tests against the simulator (any machine)

```bash
pip install -r requirements.txt
pytest --dut sim
```

### On the Raspberry Pi with the board connected

```bash
sudo apt install gcc-arm-none-eabi cmake openocd
pip install -r requirements-pi.txt

cmake -S firmware -B build -DCMAKE_TOOLCHAIN_FILE="$PWD/firmware/cmake/arm-none-eabi.cmake"
cmake --build build
tools/flash_tm4c123.sh build/firmware.elf

pytest --dut serial --port=/dev/ttyACM0
```

Or flash and test in one step (pytest flashes the image, waits for the board
to answer `PING`, then runs the suite):

```bash
pytest --dut serial --port=/dev/ttyACM0 --flash=build/firmware.elf
```

Add `--gpio` when the GPIO wiring above is in place; without it the
GPIO-loop tests are skipped.

### Hardware tests in CI

The `hardware-tests` job in `.github/workflows/ci.yml` runs on a self-hosted
GitHub Actions runner on the Raspberry Pi (labels `self-hosted`, `hil`). On
every push to `master` it takes the firmware built in the cloud, flashes it
to the board and runs the suite on real hardware, publishing a JUnit report.
Pull requests never reach the Pi. The job is skipped until the repository
variable `HIL_RUNNER` is set to `true` (Settings > Secrets and variables >
Actions > Variables).

## Roadmap

- [x] Phase 1: UART command protocol, LED control, uptime; pytest suite with simulator
- [ ] Phase 2: GPIO in the loop: the Pi drives a pin into the board and measures the reaction time
- [ ] Phase 3: automatic flashing before each test run
- [ ] Phase 4: GitHub Actions self-hosted runner on the Pi: build, flash, test, publish JUnit report
- [ ] Phase 5: CAN bus between board and Pi (python-can), fault injection, port to STM32 Nucleo
