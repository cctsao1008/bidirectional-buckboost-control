#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    printf 'error: arm-none-eabi-gcc is not installed or not in PATH\n' >&2
    exit 1
fi

if [[ ! -f "${ROOT_DIR}/third_party/libopencm3/lib/libopencm3_stm32f3.a" ]]; then
    bash "${ROOT_DIR}/tools/bootstrap-libopencm3.sh"
fi

make -C "${ROOT_DIR}/firmware" clean all
