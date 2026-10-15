# hil-bench

A small Hardware-in-the-Loop (HIL) test bench: bare-metal C firmware on an ARM
Cortex-M board, exercised automatically from a Raspberry Pi with pytest.

```
 ┌────────────────┐   UART over USB (commands)   ┌──────────────────────────┐
 │  Raspberry Pi  │◄────────────────────────────►│  TM4C123G LaunchPad      │
 │  pytest        │                              │  Cortex-M4, bare-metal C │
 │  (controller)  │   SWD via on-board ICDI      │  (device under test)     │
 │                │─────────────────────────────►│                          │
 └────────────────┘   (flashing)                 └──────────────────────────┘
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
| `host/hil/` | DUT connections (serial and simulator) and the firmware model |
| `tests/` | pytest suite |
| `tools/` | Flashing script |

## Protocol

ASCII lines terminated by `\n`, one response per request:

| Request | Response |
|---------|----------|
| `PING` | `PONG` |
| `VERSION` | `VERSION hil-bench-fw 0.1.0` |
| `LED <RED\|GREEN\|BLUE> <ON\|OFF>` | `OK` |
| `LEDS` | `LEDS RED=0 GREEN=1 BLUE=0` |
| `UPTIME` | `UPTIME <ms>` |

Errors: `ERR EMPTY`, `ERR UNKNOWN_COMMAND`, `ERR BAD_ARGS`, `ERR LINE_TOO_LONG`.

## Quick start

### Tests against the simulator (any machine)

```bash
pip install -r requirements.txt
pytest --dut sim
```

### On the Raspberry Pi with the board connected

```bash
sudo apt install gcc-arm-none-eabi cmake openocd
pip install -r requirements.txt

cmake -S firmware -B build -DCMAKE_TOOLCHAIN_FILE="$PWD/firmware/cmake/arm-none-eabi.cmake"
cmake --build build
tools/flash_tm4c123.sh build/firmware.elf

pytest --dut serial --port /dev/ttyACM0
```

## Roadmap

- [x] Phase 1: UART command protocol, LED control, uptime; pytest suite with simulator
- [ ] Phase 2: GPIO in the loop: the Pi drives a pin into the board and measures the reaction time
- [ ] Phase 3: automatic flashing before each test run
- [ ] Phase 4: GitHub Actions self-hosted runner on the Pi: build, flash, test, publish JUnit report
- [ ] Phase 5: CAN bus between board and Pi (python-can), fault injection, port to STM32 Nucleo
