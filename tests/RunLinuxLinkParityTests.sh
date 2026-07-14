#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: $0 <cmake-libUE4SS.so> <xmake-libUE4SS.so>" >&2
    exit 2
fi

cmake_library="$1"
xmake_library="$2"

for library in "${cmake_library}" "${xmake_library}"; do
    if [[ ! -f "${library}" ]]; then
        echo "library not found: ${library}" >&2
        exit 3
    fi
done

needed_libraries() {
    readelf -d "$1" | sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p' | sort
}

cmake_needed="$(needed_libraries "${cmake_library}")"
xmake_needed="$(needed_libraries "${xmake_library}")"

if [[ "${cmake_needed}" != "${xmake_needed}" ]]; then
    echo "CMake and xmake DT_NEEDED entries differ" >&2
    diff -u <(printf '%s\n' "${cmake_needed}") <(printf '%s\n' "${xmake_needed}") >&2 || true
    exit 1
fi

printf '%s\n' 'PASS: CMake and xmake linked-library parity'
