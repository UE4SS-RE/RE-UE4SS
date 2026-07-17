#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat >&2 <<'EOF'
usage: docker_soak.sh <running-container> [output-directory]

Environment:
  UE4SS_SOAK_DURATION=24h                 healthy runtime to observe
  UE4SS_SOAK_INTERVAL=60s                 metric interval
  UE4SS_SOAK_STARTUP_TIMEOUT=300s         time allowed to become healthy
  UE4SS_SOAK_MAX_THREAD_GROWTH=32         sustained growth above ready baseline
  UE4SS_SOAK_THREAD_FAILURE_SAMPLES=3     consecutive excessive samples
  UE4SS_SOAK_MAX_RSS_GROWTH_KIB=0        optional RSS limit; zero disables it
  UE4SS_SOAK_HEALTH_FAILURE_SAMPLES=3     consecutive unhealthy samples
  UE4SS_SOAK_STOP_ON_SUCCESS=false        send the image STOPSIGNAL after success
  UE4SS_SOAK_STOP_TIMEOUT=45              Docker graceful-stop timeout in seconds
  UE4SS_SOAK_SAVE_ROOT=                   optional host bind mount to hash after stop
EOF
}

is_true() {
    case "${1:-}" in
        1|true|TRUE|yes|YES|on|ON) return 0 ;;
        *) return 1 ;;
    esac
}

duration_seconds() {
    local value=$1
    local label=$2
    if [[ ! "$value" =~ ^([0-9]+)([smhd]?)$ ]]; then
        echo "$label must be a non-negative integer followed by s, m, h, or d" >&2
        return 2
    fi
    local amount=${BASH_REMATCH[1]}
    local unit=${BASH_REMATCH[2]}
    local multiplier=1
    case "$unit" in
        s|'') multiplier=1 ;;
        m) multiplier=60 ;;
        h) multiplier=3600 ;;
        d) multiplier=86400 ;;
    esac
    printf '%s\n' "$((amount * multiplier))"
}

positive_integer() {
    local value=$1
    local label=$2
    if [[ ! "$value" =~ ^[1-9][0-9]*$ ]]; then
        echo "$label must be a positive integer" >&2
        return 2
    fi
}

if [[ $# -lt 1 || $# -gt 2 ]]; then
    usage
    exit 2
fi
if ! command -v docker >/dev/null; then
    echo "docker is required" >&2
    exit 2
fi

container=$1
duration=$(duration_seconds "${UE4SS_SOAK_DURATION:-24h}" UE4SS_SOAK_DURATION)
interval=$(duration_seconds "${UE4SS_SOAK_INTERVAL:-60s}" UE4SS_SOAK_INTERVAL)
startup_timeout=$(duration_seconds "${UE4SS_SOAK_STARTUP_TIMEOUT:-300s}" UE4SS_SOAK_STARTUP_TIMEOUT)
max_thread_growth=${UE4SS_SOAK_MAX_THREAD_GROWTH:-32}
thread_failure_samples=${UE4SS_SOAK_THREAD_FAILURE_SAMPLES:-3}
max_rss_growth=${UE4SS_SOAK_MAX_RSS_GROWTH_KIB:-0}
health_failure_samples=${UE4SS_SOAK_HEALTH_FAILURE_SAMPLES:-3}
stop_timeout=${UE4SS_SOAK_STOP_TIMEOUT:-45}

positive_integer "$duration" UE4SS_SOAK_DURATION
positive_integer "$interval" UE4SS_SOAK_INTERVAL
positive_integer "$startup_timeout" UE4SS_SOAK_STARTUP_TIMEOUT
positive_integer "$thread_failure_samples" UE4SS_SOAK_THREAD_FAILURE_SAMPLES
positive_integer "$health_failure_samples" UE4SS_SOAK_HEALTH_FAILURE_SAMPLES
positive_integer "$stop_timeout" UE4SS_SOAK_STOP_TIMEOUT
if [[ ! "$max_thread_growth" =~ ^[0-9]+$ || ! "$max_rss_growth" =~ ^[0-9]+$ ]]; then
    echo "thread and RSS growth limits must be non-negative integers" >&2
    exit 2
fi
if ! docker inspect "$container" >/dev/null 2>&1; then
    echo "container does not exist: $container" >&2
    exit 2
fi

started_at=$(date -u +%Y%m%dT%H%M%SZ)
output_directory=${2:-"build/soak-${container}-${started_at}"}
mkdir -p "$output_directory"
metrics_file="$output_directory/metrics.csv"
summary_file="$output_directory/summary.txt"
logs_file="$output_directory/container.log"
printf 'timestamp_utc,ready_elapsed_seconds,health,restarts,threads,rss_kib,save_files,save_bytes\n' >"$metrics_file"

monitor_start=$(date +%s)
ready_at=0
baseline_threads=0
baseline_rss=0
max_threads=0
max_rss=0
samples=0
health_failures=0
thread_failures=0
failure=""
initial_restarts=$(docker inspect "$container" --format '{{.RestartCount}}')

write_summary() {
    local result=$1
    local finished_at
    finished_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)
    {
        printf 'result=%s\n' "$result"
        printf 'container=%s\n' "$container"
        printf 'finished_at=%s\n' "$finished_at"
        printf 'duration_seconds=%s\n' "$duration"
        printf 'interval_seconds=%s\n' "$interval"
        printf 'samples=%s\n' "$samples"
        printf 'baseline_threads=%s\n' "$baseline_threads"
        printf 'max_threads=%s\n' "$max_threads"
        printf 'baseline_rss_kib=%s\n' "$baseline_rss"
        printf 'max_rss_kib=%s\n' "$max_rss"
        printf 'failure=%s\n' "$failure"
    } >"$summary_file"
}

while true; do
    now=$(date +%s)
    state=$(docker inspect "$container" --format '{{.State.Status}}')
    if [[ "$state" != running ]]; then
        failure="container left running state: $state"
        break
    fi
    restarts=$(docker inspect "$container" --format '{{.RestartCount}}')
    if [[ "$restarts" != "$initial_restarts" ]]; then
        failure="container restart count changed from $initial_restarts to $restarts"
        break
    fi
    health=$(docker inspect "$container" --format '{{if .State.Health}}{{.State.Health.Status}}{{else}}none{{end}}')
    host_pid=$(docker inspect "$container" --format '{{.State.Pid}}')
    if [[ ! -r "/proc/$host_pid/status" ]]; then
        failure="container init process is not readable on the host"
        break
    fi
    threads=$(awk '/^Threads:/{print $2}' "/proc/$host_pid/status")
    rss=$(awk '/^VmRSS:/{print $2}' "/proc/$host_pid/status")
    save_stats=$(docker exec "$container" sh -c \
        "find /palworld/Pal/Saved -type f -printf '%s\\n' 2>/dev/null | awk '{count += 1; bytes += \$1} END {print count + 0, bytes + 0}'" \
        2>/dev/null || printf '0 0')
    read -r save_files save_bytes <<<"$save_stats"

    ready=false
    if [[ "$health" == healthy ]] && docker exec "$container" /usr/local/bin/ue4ss-healthcheck >/dev/null 2>&1; then
        ready=true
    fi
    if [[ "$ready_at" -eq 0 ]]; then
        if [[ "$ready" == true ]]; then
            ready_at=$now
            baseline_threads=$threads
            baseline_rss=$rss
            max_threads=$threads
            max_rss=$rss
        elif (( now - monitor_start >= startup_timeout )); then
            failure="container did not pass the UE4SS readiness gate within ${startup_timeout}s"
            break
        else
            sleep "$interval"
            continue
        fi
    fi

    elapsed=$((now - ready_at))
    samples=$((samples + 1))
    (( threads > max_threads )) && max_threads=$threads
    (( rss > max_rss )) && max_rss=$rss
    printf '%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "$(date -u +%Y-%m-%dT%H:%M:%SZ)" "$elapsed" "$health" "$restarts" \
        "$threads" "$rss" "$save_files" "$save_bytes" >>"$metrics_file"

    if [[ "$ready" == true ]]; then
        health_failures=0
    else
        health_failures=$((health_failures + 1))
        if (( health_failures >= health_failure_samples )); then
            failure="UE4SS healthcheck failed for $health_failures consecutive samples"
            break
        fi
    fi
    if (( threads > baseline_threads + max_thread_growth )); then
        thread_failures=$((thread_failures + 1))
        if (( thread_failures >= thread_failure_samples )); then
            failure="thread count exceeded ready baseline by more than $max_thread_growth for $thread_failures samples"
            break
        fi
    else
        thread_failures=0
    fi
    if (( max_rss_growth > 0 && rss > baseline_rss + max_rss_growth )); then
        failure="RSS exceeded ready baseline by more than ${max_rss_growth} KiB"
        break
    fi
    if (( elapsed >= duration )); then
        break
    fi
    remaining=$((duration - elapsed))
    sleep_for=$interval
    (( remaining < sleep_for )) && sleep_for=$remaining
    (( sleep_for > 0 )) && sleep "$sleep_for"
done

docker logs "$container" >"$logs_file" 2>&1 || true
if [[ -z "$failure" ]] && grep -aEq \
    'Segmentation fault|Fatal error:|Assertion failed:|retaining trampoline|trampoline release failed|UE4SS_REQUIRED is true; refusing to start' \
    "$logs_file"; then
    failure="container log contains a fatal runtime or hook-retirement error"
fi

if [[ -z "$failure" ]] && is_true "${UE4SS_SOAK_STOP_ON_SUCCESS:-false}"; then
    docker stop --timeout "$stop_timeout" "$container" >/dev/null
    exit_code=$(docker inspect "$container" --format '{{.State.ExitCode}}')
    if [[ "$exit_code" != 0 && "$exit_code" != 130 && "$exit_code" != 143 ]]; then
        failure="graceful stop returned unexpected container exit code $exit_code"
    fi
    docker logs "$container" >"$logs_file" 2>&1 || true
    if [[ -z "$failure" ]] && ! grep -aFq 'UE4SS Linux core stopped' "$logs_file"; then
        failure="graceful stop did not log UE4SS core shutdown"
    fi
    if [[ -n "${UE4SS_SOAK_SAVE_ROOT:-}" ]]; then
        if ! save_root=$(realpath "${UE4SS_SOAK_SAVE_ROOT}" 2>/dev/null) || [[ ! -d "$save_root" ]]; then
            failure="UE4SS_SOAK_SAVE_ROOT is not a directory"
        else
            (
                cd "$save_root"
                find . -type f -print0 | sort -z | xargs -0 -r sha256sum
            ) >"$output_directory/save-sha256.txt"
        fi
    fi
fi

if [[ -n "$failure" ]]; then
    write_summary FAIL
    echo "FAIL: $failure" >&2
    echo "metrics=$metrics_file" >&2
    exit 1
fi

write_summary PASS
echo "PASS: UE4SS Docker soak gate completed"
echo "metrics=$metrics_file"
echo "summary=$summary_file"
