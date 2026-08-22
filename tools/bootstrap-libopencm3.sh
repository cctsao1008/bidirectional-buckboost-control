#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST_DIR="${ROOT_DIR}/third_party/libopencm3"
LIBOPENCM3_REV="2da12dc96e0b9e42a3332348dd9b02a0a17981f8"

mkdir -p "${ROOT_DIR}/third_party"

if [[ ! -d "${DEST_DIR}/.git" ]]; then
    git clone https://github.com/libopencm3/libopencm3.git "${DEST_DIR}"
fi

git -C "${DEST_DIR}" fetch origin
git -C "${DEST_DIR}" checkout --detach "${LIBOPENCM3_REV}"

JOBS="${JOBS:-$(nproc)}"
make -C "${DEST_DIR}" -j"${JOBS}"

printf 'libopencm3 ready at %s\n' "${LIBOPENCM3_REV}"
