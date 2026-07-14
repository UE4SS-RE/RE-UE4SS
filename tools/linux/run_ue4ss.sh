#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ue4ss_library="${script_dir}/libUE4SS.so"

if [[ $# -lt 1 ]]; then
    echo "usage: $0 <target executable> [arguments ...]" >&2
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
