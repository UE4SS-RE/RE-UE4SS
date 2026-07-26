#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
palworld_root="${PALWORLD_SERVER_ROOT:-}"
ue4ss_library="${UE4SS_LIBRARY:-${repo_root}/build_linux/Game__Shipping__Linux/lib/libUE4SS.so}"
launcher_source="${UE4SS_LAUNCHER_SOURCE:-${repo_root}/tools/linux/run_ue4ss.sh}"
settings_source="${UE4SS_SETTINGS_SOURCE:-${repo_root}/assets/UE4SS-settings.ini}"
stage_directory="${ACCEPTANCE_STAGE_DIR:-${repo_root}/build_linux/palworld-acceptance-stage}"
cpp_probe_build_directory="${CPP_PROBE_BUILD_DIR:-${repo_root}/build_linux/palworld-cpp-probe}"
server_executable="${PALWORLD_EXECUTABLE:-${palworld_root}/Pal/Binaries/Linux/PalServer-Linux-Shipping}"
ready_regex="${PALWORLD_READY_REGEX:-Running Palworld dedicated server on|LogNet: GameNetDriver .* listening on port}"
ready_timeout="${READY_TIMEOUT_SECONDS:-180}"
marker_timeout="${MARKER_TIMEOUT_SECONDS:-180}"
dumper_timeout="${DUMPER_TIMEOUT_SECONDS:-600}"
dumper_delay_ms="${UE4SS_ACCEPTANCE_DUMPER_DELAY_MS:-30000}"

if [[ -z "${palworld_root}" ]]; then
    echo "PALWORLD_SERVER_ROOT must point to an installed Palworld dedicated server" >&2
    exit 2
fi
for required_file in "${ue4ss_library}" "${launcher_source}" "${settings_source}" "${server_executable}"; do
    if [[ ! -f "${required_file}" ]]; then
        echo "required file not found: ${required_file}" >&2
        exit 3
    fi
done
if [[ ! -x "${server_executable}" ]]; then
    echo "Palworld server executable is not executable: ${server_executable}" >&2
    exit 4
fi

cpp_probe_library="${CPP_PROBE_LIBRARY:-}"
if [[ "${SKIP_CPP_PROBE_BUILD:-0}" != "1" ]]; then
    cmake -S "${repo_root}/tests/palworld/CppProbeMod" -B "${cpp_probe_build_directory}" -G Ninja \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_BUILD_TYPE=Release \
        -DUE4SS_ROOT="${repo_root}" \
        -DUE4SS_LIBRARY="${ue4ss_library}"
    cmake --build "${cpp_probe_build_directory}" --parallel
    cpp_probe_library="${cpp_probe_build_directory}/libCppProbeMod.so"
fi
if [[ -z "${cpp_probe_library}" || ! -f "${cpp_probe_library}" ]]; then
    echo "C++ probe library not found: ${cpp_probe_library:-<unset>}" >&2
    exit 5
fi

rm -rf -- "${stage_directory}"
mkdir -p -- \
    "${stage_directory}/Mods/LuaProbeMod/Scripts" \
    "${stage_directory}/Mods/CppProbeMod/dlls" \
    "${stage_directory}/UE4SS-crashes"

install -m 755 "${launcher_source}" "${stage_directory}/run_ue4ss.sh"
install -m 755 "${ue4ss_library}" "${stage_directory}/libUE4SS.so"
install -m 644 "${settings_source}" "${stage_directory}/UE4SS-settings.ini"
install -m 644 "${repo_root}/tests/palworld/LuaProbeMod/Scripts/main.lua" "${stage_directory}/Mods/LuaProbeMod/Scripts/main.lua"
install -m 644 "${cpp_probe_library}" "${stage_directory}/Mods/CppProbeMod/dlls/main.so"
: > "${stage_directory}/Mods/LuaProbeMod/enabled.txt"
: > "${stage_directory}/Mods/CppProbeMod/enabled.txt"

steamclient_source="${palworld_root}/linux64/steamclient.so"
steamclient_destination="$(dirname -- "${server_executable}")/steamclient.so"
if [[ ! -f "${steamclient_destination}" && -f "${steamclient_source}" ]]; then
    install -m 755 "${steamclient_source}" "${steamclient_destination}"
fi

server_log="${stage_directory}/palworld-server.log"
report_file="${stage_directory}/acceptance-report.txt"
: > "${server_log}"
: > "${report_file}"

server_pid=""
cleanup() {
    if [[ -n "${server_pid}" ]] && kill -0 "${server_pid}" 2>/dev/null; then
        kill -TERM "${server_pid}" 2>/dev/null || true
        for _ in {1..20}; do
            if ! kill -0 "${server_pid}" 2>/dev/null; then
                break
            fi
            sleep 0.25
        done
        if kill -0 "${server_pid}" 2>/dev/null; then
            kill -KILL "${server_pid}" 2>/dev/null || true
        fi
        wait "${server_pid}" 2>/dev/null || true
    fi
}
trap cleanup EXIT INT TERM

has_crash_marker() {
    grep -Eqi 'ACCEPT:FAIL|DIAG: inactive_reason=|UE4SS: initialization failed|Fatal error|Segmentation fault|signal (6|7|8|11)' "${server_log}"
}

wait_for_log_regex() {
    local description="$1"
    local pattern="$2"
    local timeout_seconds="$3"
    local started_at=${SECONDS}

    while ! grep -Eq "${pattern}" "${server_log}"; do
        if has_crash_marker; then
            echo "server reported a crash or acceptance failure while waiting for ${description}" >&2
            tail -n 120 "${server_log}" >&2
            return 1
        fi
        if ! kill -0 "${server_pid}" 2>/dev/null; then
            echo "server exited while waiting for ${description}" >&2
            tail -n 120 "${server_log}" >&2
            return 1
        fi
        if (( SECONDS - started_at >= timeout_seconds )); then
            echo "timed out waiting for marker: ${description}" >&2
            tail -n 120 "${server_log}" >&2
            return 1
        fi
        sleep 0.25
    done
}

wait_for_file_regex() {
    local description="$1"
    local file="$2"
    local pattern="$3"
    local timeout_seconds="$4"
    local started_at=${SECONDS}

    while [[ ! -f "${file}" ]] || ! grep -Eq "${pattern}" "${file}"; do
        if has_crash_marker; then
            echo "server reported a crash or acceptance failure while waiting for ${description}" >&2
            tail -n 120 "${server_log}" >&2
            return 1
        fi
        if ! kill -0 "${server_pid}" 2>/dev/null; then
            echo "server exited while waiting for ${description}" >&2
            tail -n 120 "${server_log}" >&2
            return 1
        fi
        if (( SECONDS - started_at >= timeout_seconds )); then
            echo "timed out waiting for marker: ${description}" >&2
            if [[ -f "${file}" ]]; then
                tail -n 120 "${file}" >&2
            fi
            return 1
        fi
        sleep 0.25
    done
}

wait_for_file() {
    local description="$1"
    local timeout_seconds="$2"
    local started_at=${SECONDS}

    while [[ ! -s "${description}" ]]; do
        if has_crash_marker; then
            echo "server reported a crash or acceptance failure while waiting for ${description}" >&2
            return 1
        fi
        if ! kill -0 "${server_pid}" 2>/dev/null; then
            echo "server exited while waiting for ${description}" >&2
            return 1
        fi
        if (( SECONDS - started_at >= timeout_seconds )); then
            echo "timed out waiting for file: ${description}" >&2
            return 1
        fi
        sleep 0.25
    done
}

server_started_at="$(date +%s)"
(
    cd -- "${palworld_root}"
    UE4SS_DIAGNOSE=1 \
    UE4SS_CRASH_LOG_DIR="${stage_directory}/UE4SS-crashes" \
    UE4SS_ACCEPTANCE_RUN_DUMPERS=1 \
    UE4SS_ACCEPTANCE_DUMPER_DELAY_MS="${dumper_delay_ms}" \
    UE4SS_ACCEPTANCE_STAGE_DIR="${stage_directory}" \
        exec "${stage_directory}/run_ue4ss.sh" "${server_executable}" Pal "$@"
) > "${server_log}" 2>&1 &
server_pid=$!

wait_for_log_regex "${ready_regex}" "${ready_regex}" "${ready_timeout}"
ready_at="$(date +%s)"

ue4ss_log="${stage_directory}/UE4SS.log"
wait_for_file_regex 'DIAG: engine_version=5.1' "${ue4ss_log}" 'DIAG: engine_version=5\.1' "${marker_timeout}"
for signature in \
    GUObjectArray FNameToString FNameCtorWchar GMalloc \
    StaticConstructObjectInternal FTextFString \
    GNatives ConsoleManagerSingleton UGameEngineTick; do
    wait_for_file_regex "DIAG: signature=${signature} status=resolved" "${ue4ss_log}" \
        "DIAG: signature=${signature} status=resolved" "${marker_timeout}"
done
wait_for_file_regex 'DIAG: optional signature=FUObjectHashTablesGet' "${ue4ss_log}" \
    'DIAG: signature=FUObjectHashTablesGet status=(resolved|missing)' "${marker_timeout}"

for marker in \
    'ACCEPT:LUA_MOD_STARTED' \
    'ACCEPT:LUA_CONSOLE_REGISTERED' \
    'ACCEPT:LUA_BLUEPRINT_HOOK' \
    'ACCEPT:LUA_FIND_ALL count=' \
    'ACCEPT:LUA_PROPERTY_READ value=' \
    'ACCEPT:LUA_PROPERTY_WRITE value=' \
    'ACCEPT:LUA_PROPERTY_RESTORED value=' \
    'ACCEPT:LUA_FNAME value=UE4SSAcceptanceProbeObject' \
    'ACCEPT:LUA_STATIC_CONSTRUCT_OBJECT' \
    'ACCEPT:CPP_START_MOD' \
    'ACCEPT:CPP_PROGRAM_START' \
    'ACCEPT:CPP_UNREAL_INIT' \
    'ACCEPT:CPP_UPDATE count=10' \
    'ACCEPT:LUA_DUMP_OBJECTS_REQUESTED' \
    'ACCEPT:LUA_CXX_HEADERS_COMPLETED' \
    'ACCEPT:LUA_DUMP_USMAP_REQUESTED'; do
    wait_for_log_regex "${marker}" "${marker}" "${marker_timeout}"
done

wait_for_file "${ue4ss_log}" "${marker_timeout}"

object_dump="${stage_directory}/UE4SS_ObjectDump.txt"
wait_for_file "${object_dump}" "${dumper_timeout}"
if ! grep -aFq 'PalPlayerCharacter' "${object_dump}"; then
    echo "object dump does not contain PalPlayerCharacter: ${object_dump}" >&2
    exit 7
fi

cxx_header_directory="${stage_directory}/CXXHeaderDump"
cxx_header=""
cxx_started_at=${SECONDS}
while [[ -z "${cxx_header}" ]]; do
    cxx_header="$(find "${cxx_header_directory}" -type f -name '*.hpp' -size +0c -print -quit 2>/dev/null || true)"
    if [[ -n "${cxx_header}" ]]; then
        break
    fi
    if has_crash_marker || ! kill -0 "${server_pid}" 2>/dev/null; then
        echo "server failed while waiting for CXX header output" >&2
        exit 8
    fi
    if (( SECONDS - cxx_started_at >= dumper_timeout )); then
        echo "timed out waiting for CXX header output" >&2
        exit 8
    fi
    sleep 0.25
done
if ! grep -aRFq 'PalPlayerCharacter' "${cxx_header_directory}"; then
    echo "CXX header output does not contain PalPlayerCharacter: ${cxx_header_directory}" >&2
    exit 9
fi

usmap_started_at=${SECONDS}
usmap_file=""
while [[ -z "${usmap_file}" ]]; do
    usmap_file="$(find "${stage_directory}" -maxdepth 1 -type f -name '*.usmap' -size +0c -print -quit)"
    if [[ -n "${usmap_file}" ]]; then
        break
    fi
    if has_crash_marker || ! kill -0 "${server_pid}" 2>/dev/null; then
        echo "server failed while waiting for USMAP output" >&2
        exit 10
    fi
    if (( SECONDS - usmap_started_at >= dumper_timeout )); then
        echo "timed out waiting for USMAP output" >&2
        exit 10
    fi
    sleep 0.25
done
if ! grep -aFq 'PalPlayerCharacter' "${usmap_file}"; then
    echo "USMAP output does not contain PalPlayerCharacter: ${usmap_file}" >&2
    exit 11
fi

server_sha256="$(sha256sum "${server_executable}" | awk '{print $1}')"
cat > "${report_file}" <<REPORT
PASS: Palworld acceptance probes completed
server_root=${palworld_root}
server_executable=${server_executable}
server_sha256=${server_sha256}
startup_seconds=$((ready_at - server_started_at))
ue4ss_log=${ue4ss_log}
object_dump=${object_dump}
cxx_header_directory=${cxx_header_directory}
usmap=${usmap_file}
server_log=${server_log}
REPORT

printf '%s\n' "PASS: Palworld acceptance probes completed"
printf '%s\n' "Report: ${report_file}"
