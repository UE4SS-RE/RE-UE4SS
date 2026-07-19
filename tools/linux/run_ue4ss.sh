#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ue4ss_library="${script_dir}/libUE4SS.so"

usage() {
    echo "usage: $0 [--host-executable <ELF executable>] <command> [arguments ...]" >&2
}

canonical_path() {
    readlink -f -- "$1"
}

is_elf_file() {
    [[ "$(LC_ALL=C od -An -t x1 -N4 -- "$1" | tr -d '[:space:]')" == "7f454c46" ]]
}

host_executable=""
if [[ "${1:-}" == "--host-executable" ]]; then
    if [[ $# -lt 3 ]]; then
        usage
        exit 2
    fi
    host_executable="$2"
    shift 2
fi
if [[ $# -lt 1 ]]; then
    usage
    exit 2
fi

target_executable="$1"
shift

if [[ ! -f "${ue4ss_library}" ]]; then
    echo "UE4SS launcher: libUE4SS.so not found: ${ue4ss_library}" >&2
    exit 3
fi
if [[ ! -f "${target_executable}" || ! -x "${target_executable}" ]]; then
    echo "UE4SS launcher: target executable not found or not executable: ${target_executable}" >&2
    exit 4
fi
if [[ -z "${host_executable}" ]]; then
    host_executable="${target_executable}"
    if ! is_elf_file "${host_executable}"; then
        echo "UE4SS launcher: script commands require --host-executable <ELF executable>" >&2
        exit 5
    fi
fi
if [[ ! -f "${host_executable}" || ! -x "${host_executable}" ]]; then
    echo "UE4SS launcher: host executable not found or not executable: ${host_executable}" >&2
    exit 6
fi
if ! is_elf_file "${host_executable}"; then
    echo "UE4SS launcher: host executable is not an ELF file: ${host_executable}" >&2
    exit 7
fi

export UE4SS_LAUNCH_TARGET_EXE="$(canonical_path "${host_executable}")"
if [[ -v LD_PRELOAD ]]; then
    export UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=1
    export UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD="${LD_PRELOAD}"
else
    export UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=0
    export UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=""
fi

export UE4SS_MODULE_PATH="${ue4ss_library}"
preload_path="${ue4ss_library}"
if [[ "${preload_path}" == *[[:space:]:]* ]]; then
    exec {ue4ss_preload_fd}<"${ue4ss_library}"
    preload_path="/proc/self/fd/${ue4ss_preload_fd}"
fi

if [[ -n "${LD_PRELOAD:-}" ]]; then
    export LD_PRELOAD="${preload_path}:${LD_PRELOAD}"
else
    export LD_PRELOAD="${preload_path}"
fi

exec "${target_executable}" "$@"
