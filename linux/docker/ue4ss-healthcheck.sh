#!/usr/bin/env bash
set -euo pipefail

is_true() {
    case "${1:-}" in
        1|true|TRUE|yes|YES|on|ON) return 0 ;;
        *) return 1 ;;
    esac
}

server_pid=${UE4SS_SERVER_PID:-1}
kill -0 "$server_pid" 2>/dev/null

if ! is_true "${UE4SS_ENABLED:-false}" || ! is_true "${UE4SS_HEALTH_REQUIRE_READY:-true}"; then
    exit 0
fi

status_file="${UE4SS_STATE_DIR:-/palworld/ue4ss-state}/ue4ss.status.json"
[[ -r "$status_file" ]]
grep -Fq '"state": "platform_ready"' "$status_file"
grep -Eq '"process_id": [[:space:]]*'"$server_pid"'([,}])' "$status_file"
grep -Fq '"lua_readonly_bindings": {"state": "available"' "$status_file"
grep -Fq '"game_thread_scheduler": {"state": "available"' "$status_file"
grep -Fq '"native_ufunction_hooks": {"state": "available"' "$status_file"
grep -Fq '"blueprint_hooks": {"state": "available"' "$status_file"
grep -Fq '"begin_play_hooks": {"state": "available"' "$status_file"
grep -Fq '"object_notifications": {"state": "available"' "$status_file"
grep -Fq '"lua_mods": {"state": "available"' "$status_file"
