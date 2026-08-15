#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 <palworld-server-root> [linux-build-dir] [sandbox-dir]" >&2
}

if [[ $# -lt 1 || $# -gt 3 ]]; then
    usage
    exit 2
fi

server_root=$(realpath "$1")
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(realpath "$script_dir/../..")
build_dir=$(realpath "${2:-$repo_root/build/linux-debug/linux}")
sandbox_dir=${3:-$repo_root/build/palworld-conformance-sandbox}

server_binary="$server_root/Pal/Binaries/Linux/PalServer-Linux-Shipping"
core="$build_dir/libUE4SS.so"
preload="$build_dir/libue4ss_preload.so"
if [[ ! -x "$server_binary" || ! -f "$core" || ! -f "$preload" ]]; then
    echo "missing PalServer binary or UE4SS Linux build artifacts" >&2
    exit 2
fi
if ! command -v rsync >/dev/null || ! command -v sha256sum >/dev/null; then
    echo "rsync and sha256sum are required" >&2
    exit 2
fi

mkdir -p "$sandbox_dir"
sandbox_dir=$(realpath "$sandbox_dir")
rsync -a --delete --exclude=/Pal/Saved/ "$server_root/" "$sandbox_dir/"

ue4ss_root="$sandbox_dir/Pal/Binaries/Linux/ue4ss"
state_dir="$sandbox_dir/ue4ss-conformance-state"
mod_root="$ue4ss_root/Mods/LinuxReadOnlyConformance"
mkdir -p "$ue4ss_root" "$state_dir" "$mod_root/Scripts"
cp "$core" "$ue4ss_root/libUE4SS.so"
cp "$preload" "$ue4ss_root/libue4ss_preload.so"
cp "$repo_root/linux/packaging/Mods/LinuxReadOnlyConformance/Scripts/main.lua" "$mod_root/Scripts/main.lua"
cp "$repo_root/linux/packaging/Mods/LinuxReadOnlyConformance/enabled.example" "$mod_root/enabled.txt"

extra_mod_dirs=()
if [[ -n "${UE4SS_EXTRA_MOD_DIR:-}" ]]; then
    extra_mod_dirs+=("$UE4SS_EXTRA_MOD_DIR")
fi
if [[ -n "${UE4SS_EXTRA_MOD_DIRS:-}" ]]; then
    IFS=: read -r -a configured_extra_mod_dirs <<<"$UE4SS_EXTRA_MOD_DIRS"
    for configured_extra_mod_dir in "${configured_extra_mod_dirs[@]}"; do
        [[ -n "$configured_extra_mod_dir" ]] && extra_mod_dirs+=("$configured_extra_mod_dir")
    done
fi

declare -A installed_extra_mod_names=()
for configured_extra_mod_dir in "${extra_mod_dirs[@]}"; do
    extra_mod_dir=$(realpath "$configured_extra_mod_dir")
    if [[ ! -f "$extra_mod_dir/Scripts/main.lua" ]]; then
        echo "extra mod directory must contain Scripts/main.lua: $extra_mod_dir" >&2
        exit 2
    fi
    extra_mod_name=$(basename "$extra_mod_dir")
    if [[ "$extra_mod_name" == "LinuxReadOnlyConformance" ]]; then
        echo "extra mod directory must use a distinct mod directory name" >&2
        exit 2
    fi
    normalized_extra_mod_name=${extra_mod_name,,}
    if [[ -n "${installed_extra_mod_names[$normalized_extra_mod_name]:-}" ]]; then
        echo "duplicate extra mod directory name: $extra_mod_name" >&2
        exit 2
    fi
    installed_extra_mod_names[$normalized_extra_mod_name]=1
    mkdir -p "$ue4ss_root/Mods/$extra_mod_name"
    cp -a "$extra_mod_dir/." "$ue4ss_root/Mods/$extra_mod_name/"
    : >"$ue4ss_root/Mods/$extra_mod_name/enabled.txt"
done

server_sha=$(sha256sum "$server_binary" | cut -d' ' -f1)
run_log="$state_dir/palserver.stdout.log"
port=${UE4SS_CONFORMANCE_PORT:-18400}
query_port=${UE4SS_CONFORMANCE_QUERY_PORT:-$((port + 10000))}
settle_seconds=${UE4SS_CONFORMANCE_SETTLE_SECONDS:-2}
if ! [[ "$settle_seconds" =~ ^(0|[1-9][0-9]*)$ ]] || (( settle_seconds > 60 )); then
    echo "UE4SS_CONFORMANCE_SETTLE_SECONDS must be an integer between 0 and 60" >&2
    exit 2
fi

server_pid=""
cleanup() {
    trap - EXIT INT TERM
    if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
        kill -INT "$server_pid" 2>/dev/null || true
        for _ in {1..20}; do
            kill -0 "$server_pid" 2>/dev/null || break
            sleep 0.25
        done
        if kill -0 "$server_pid" 2>/dev/null; then
            kill -INT "$server_pid" 2>/dev/null || true
        fi
        for _ in {1..20}; do
            kill -0 "$server_pid" 2>/dev/null || break
            sleep 0.25
        done
        if kill -0 "$server_pid" 2>/dev/null; then
            kill -TERM "$server_pid" 2>/dev/null || true
            sleep 1
        fi
        if kill -0 "$server_pid" 2>/dev/null; then
            kill -KILL "$server_pid" 2>/dev/null || true
        fi
        wait "$server_pid" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

(
    cd "$sandbox_dir"
    exec env \
        UE4SS_SCAN_UNREAL=true \
        UE4SS_SERVER_SHA256="$server_sha" \
        UE4SS_REQUIRED=true \
        UE4SS_ROOT="$ue4ss_root" \
        UE4SS_STATE_DIR="$state_dir" \
        LD_PRELOAD="$ue4ss_root/libue4ss_preload.so" \
        ./Pal/Binaries/Linux/PalServer-Linux-Shipping Pal \
            -port="$port" -queryport="$query_port" -publicport="$port" -players=1
) >"$run_log" 2>&1 &
server_pid=$!

status_file="$state_dir/ue4ss.status.json"
ready_since=-1
# Keep the harness deadline above the core's default 120-second registry
# readiness deadline so a slow server reports the actual UE4SS gate failure.
for _ in {1..300}; do
    if ! kill -0 "$server_pid" 2>/dev/null; then
        echo "PalServer exited before UE4SS became ready" >&2
        tail -80 "$run_log" >&2 || true
        exit 1
    fi
    if [[ -f "$status_file" ]] &&
       grep -Fq '"state": "platform_ready"' "$status_file" &&
       grep -Fq '"process_id": '"$server_pid"',' "$status_file" &&
       grep -Fq '"lua_readonly_bindings": {"state": "available"' "$status_file" &&
       grep -Fq '"game_thread_scheduler": {"state": "available"' "$status_file" &&
       grep -Fq '"native_ufunction_hooks": {"state": "available"' "$status_file" &&
       grep -Fq '"process_internal": {"state": "available"' "$status_file" &&
       grep -Fq '"blueprint_hooks": {"state": "available"' "$status_file" &&
       grep -Fq '"begin_play_hooks": {"state": "available"' "$status_file" &&
       grep -Fq '"object_notifications": {"state": "available"' "$status_file" &&
       grep -Fq '"lua_mods": {"state": "available"' "$status_file" &&
       grep -Fq '[LinuxReadOnlyConformance] PASS:' "$run_log"; then
        if (( ready_since < 0 )); then
            ready_since=$SECONDS
        fi
        if (( SECONDS - ready_since < settle_seconds )); then
            sleep 0.5
            continue
        fi
        if grep -aEq '\[UE4SS Linux\].*callback failed:|failed during protected load:|\[UE4SS Linux\] Lua mod .* failed for ' "$run_log"; then
            echo "a Lua mod reported a protected-load or callback failure during the settle window" >&2
            grep -aE '\[UE4SS Linux\].*callback failed:|failed during protected load:|\[UE4SS Linux\] Lua mod .* failed for ' "$run_log" >&2 || true
            exit 1
        fi
        echo "PASS: native PalServer read-only Lua conformance completed"
        echo "server_sha256=$server_sha"
        echo "status=$status_file"
        exit 0
    fi
    ready_since=-1
    sleep 0.5
done

echo "timed out waiting for native PalServer Lua conformance" >&2
tail -80 "$run_log" >&2 || true
exit 1
