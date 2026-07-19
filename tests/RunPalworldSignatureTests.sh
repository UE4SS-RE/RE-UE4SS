#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${PALWORLD_SERVER_ROOT:-}" ]]; then
    echo "SKIP: PALWORLD_SERVER_ROOT is not set; the Palworld server binary is not redistributable"
    exit 77
fi

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <PalworldSignatureTests>" >&2
    exit 2
fi

binary="${PALWORLD_SERVER_ROOT}/Pal/Binaries/Linux/PalServer-Linux-Shipping"
expected_size="196281496"
expected_sha256="a05788ead7619db22a1509c43241c16d289ed7040e8bcbb2e36e13e223e822f9"
expected_build_id="217802a00653a9c4"

if [[ ! -f "${binary}" ]]; then
    echo "Palworld executable not found: ${binary}" >&2
    exit 3
fi

actual_size="$(stat -c '%s' "${binary}")"
actual_sha256="$(sha256sum "${binary}" | awk '{print $1}')"
actual_build_id="$(readelf -n "${binary}" | awk '/Build ID:/ {print $3; exit}')"

if [[ "${actual_size}" != "${expected_size}" ]]; then
    echo "Palworld executable size mismatch: expected ${expected_size}, got ${actual_size}" >&2
    exit 4
fi
if [[ "${actual_sha256}" != "${expected_sha256}" ]]; then
    echo "Palworld executable SHA-256 mismatch: expected ${expected_sha256}, got ${actual_sha256}" >&2
    exit 5
fi
if [[ "${actual_build_id}" != "${expected_build_id}" ]]; then
    echo "Palworld executable build ID mismatch: expected ${expected_build_id}, got ${actual_build_id}" >&2
    exit 6
fi

echo "Palworld Steam build 24088465: size=${actual_size} sha256=${actual_sha256} build_id=${actual_build_id}"
exec "$1" "${binary}"
