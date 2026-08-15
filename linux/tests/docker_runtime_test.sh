#!/usr/bin/env bash
set -euo pipefail

entrypoint=$1
healthcheck=$2
temporary=$(mktemp -d)
cleanup() {
    rm -rf "$temporary"
}
trap cleanup EXIT INT TERM

disabled_output=$(UE4SS_ENABLED=false "$entrypoint" /bin/sh -c 'printf disabled')
[[ "$disabled_output" == "disabled" ]]

vanilla_output=$(
    UE4SS_ENABLED=true \
    UE4SS_REQUIRED=false \
    UE4SS_ROOT="$temporary/missing" \
    "$entrypoint" /bin/sh -c 'printf vanilla'
)
[[ "$vanilla_output" == "vanilla" ]]

set +e
UE4SS_ENABLED=true \
UE4SS_REQUIRED=true \
UE4SS_ROOT="$temporary/missing" \
"$entrypoint" /bin/true
required_status=$?
set -e
[[ $required_status -eq 78 ]]

UE4SS_SERVER_PID=$$ UE4SS_ENABLED=false "$healthcheck"

mkdir -p "$temporary/state"
cat >"$temporary/state/ue4ss.status.json" <<JSON
{
  "state": "platform_ready",
  "process_id": $$,
  "capabilities": {
    "lua_readonly_bindings": {"state": "available"},
    "game_thread_scheduler": {"state": "available"},
    "native_ufunction_hooks": {"state": "available"},
    "blueprint_hooks": {"state": "available"},
    "begin_play_hooks": {"state": "available"},
    "object_notifications": {"state": "available"},
    "lua_mods": {"state": "available"}
  }
}
JSON
UE4SS_SERVER_PID=$$ UE4SS_ENABLED=true UE4SS_STATE_DIR="$temporary/state" "$healthcheck"

sed -i 's/"state": "platform_ready"/"state": "degraded"/' "$temporary/state/ue4ss.status.json"
if UE4SS_SERVER_PID=$$ UE4SS_ENABLED=true UE4SS_STATE_DIR="$temporary/state" "$healthcheck"; then
    echo "healthcheck accepted a degraded UE4SS status" >&2
    exit 1
fi
sed -i 's/"state": "degraded"/"state": "platform_ready"/' "$temporary/state/ue4ss.status.json"

if UE4SS_SERVER_PID=$PPID UE4SS_ENABLED=true UE4SS_STATE_DIR="$temporary/state" "$healthcheck"; then
    echo "healthcheck accepted a stale UE4SS process id" >&2
    exit 1
fi

rm "$temporary/state/ue4ss.status.json"
if UE4SS_SERVER_PID=$$ UE4SS_ENABLED=true UE4SS_STATE_DIR="$temporary/state" "$healthcheck"; then
    echo "healthcheck accepted a missing UE4SS status document" >&2
    exit 1
fi
