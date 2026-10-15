#!/usr/bin/env bash
# Flash the firmware to a TM4C123G LaunchPad through its on-board ICDI debugger.
# Usage: tools/flash_tm4c123.sh [path/to/firmware.elf]
set -euo pipefail

ELF="${1:-build/firmware.elf}"

if [[ ! -f "$ELF" ]]; then
    echo "error: $ELF not found - build the firmware first" >&2
    exit 1
fi

openocd -f board/ti_ek-tm4c123gxl.cfg -c "program $ELF verify reset exit"
