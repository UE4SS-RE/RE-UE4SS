#!/usr/bin/env bash
set -euo pipefail

is_true() {
    case "${1:-}" in
        1|true|TRUE|yes|YES|on|ON) return 0 ;;
        *) return 1 ;;
    esac
}

fail_or_vanilla() {
    local message=$1
    shift
    if is_true "${UE4SS_REQUIRED:-false}"; then
        echo "[ue4ss-entrypoint] error: $message" >&2
        exit 78
    fi
    echo "[ue4ss-entrypoint] warning: $message; starting PalServer vanilla" >&2
    exec "$@"
}

if [[ $# -eq 0 ]]; then
    set -- ./Pal/Binaries/Linux/PalServer-Linux-Shipping Pal
fi

if ! is_true "${UE4SS_ENABLED:-false}"; then
    echo "[ue4ss-entrypoint] UE4SS is disabled; starting PalServer vanilla" >&2
    exec "$@"
fi

export UE4SS_ROOT=${UE4SS_ROOT:-/palworld/Pal/Binaries/Linux/ue4ss}
export UE4SS_STATE_DIR=${UE4SS_STATE_DIR:-/palworld/ue4ss-state}
preload="$UE4SS_ROOT/libue4ss_preload.so"
core="$UE4SS_ROOT/libUE4SS.so"

if [[ ! -r "$preload" || ! -r "$core" ]]; then
    fail_or_vanilla "missing readable libue4ss_preload.so or libUE4SS.so under $UE4SS_ROOT" "$@"
fi
if ! mkdir -p "$UE4SS_STATE_DIR" 2>/dev/null || [[ ! -w "$UE4SS_STATE_DIR" ]]; then
    fail_or_vanilla "UE4SS_STATE_DIR is not writable: $UE4SS_STATE_DIR" "$@"
fi

if [[ -z "${UE4SS_EXPECTED_SHA256:-}" && -r "$UE4SS_ROOT/libUE4SS.so.sha256" ]]; then
    read -r UE4SS_EXPECTED_SHA256 _ <"$UE4SS_ROOT/libUE4SS.so.sha256"
    export UE4SS_EXPECTED_SHA256
fi
if [[ -z "${UE4SS_SERVER_SHA256:-}" && -r /palworld/.palserver.sha256 ]]; then
    read -r UE4SS_SERVER_SHA256 _ </palworld/.palserver.sha256
    export UE4SS_SERVER_SHA256
fi

export UE4SS_SCAN_UNREAL=${UE4SS_SCAN_UNREAL:-true}
export UE4SS_REQUIRED=${UE4SS_REQUIRED:-false}
export LD_PRELOAD="$preload${LD_PRELOAD:+:$LD_PRELOAD}"

echo "[ue4ss-entrypoint] enabling native UE4SS from $UE4SS_ROOT (required=$UE4SS_REQUIRED)" >&2
exec "$@"
