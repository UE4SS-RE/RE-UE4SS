#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
harness="${repo_root}/tests/palworld/run_acceptance.sh"
lua_probe="${repo_root}/tests/palworld/LuaProbeMod/Scripts/main.lua"
cpp_probe="${repo_root}/tests/palworld/CppProbeMod/src/CppProbeMod.cpp"
lua_mod_source="${repo_root}/UE4SS/src/Mod/LuaMod.cpp"

if grep -Fq 'get_object_dumper_output_directory() + STR("\\")' "${lua_mod_source}"; then
    echo "Lua object dumper still uses a Windows-only path separator" >&2
    exit 3
fi
if grep -Fq 'working_dir + STR("\\CXXHeaderDump")' "${lua_mod_source}"; then
    echo "Lua CXX header generator still uses a Windows-only path separator" >&2
    exit 4
fi

if [[ ! -x "${harness}" ]]; then
    echo "acceptance harness is missing or not executable: ${harness}" >&2
    exit 1
fi

for required in \
    'RegisterConsoleCommandGlobalHandler' \
    'RegisterHook("/Script/Engine.Actor:K2_GetActorLocation"' \
    'actor:K2_GetActorLocation()' \
    'FindAllOf("Actor")' \
    'GetPropertyValue("bCanBeDamaged")' \
    'SetPropertyValue("bCanBeDamaged"' \
    'FName("UE4SSAcceptanceProbeObject"' \
    'StaticConstructObject(' \
    'DumpAllObjects()' \
    'GenerateSDK()' \
    'DumpUSMAP()'; do
    grep -Fq "${required}" "${lua_probe}"
done

for required in \
    'ACCEPT:CPP_START_MOD' \
    'ACCEPT:CPP_PROGRAM_START' \
    'ACCEPT:CPP_UNREAL_INIT' \
    'ACCEPT:CPP_UPDATE count='; do
    grep -Fq "${required}" "${cpp_probe}"
done

test_root="$(mktemp -d "${TMPDIR:-/tmp}/ue4ss-palworld-harness.XXXXXX")"
trap 'rm -rf -- "${test_root}"' EXIT

palworld_root="${test_root}/palworld"
server_directory="${palworld_root}/Pal/Binaries/Linux"
stage_directory="${test_root}/acceptance-stage"
mkdir -p -- "${server_directory}"

fake_library="${test_root}/libUE4SS.so"
fake_cpp_probe="${test_root}/main.so"
: > "${fake_library}"
: > "${fake_cpp_probe}"

fake_launcher="${test_root}/run_ue4ss.sh"
cat > "${fake_launcher}" <<'LAUNCHER'
#!/usr/bin/env bash
set -euo pipefail
exec "$@"
LAUNCHER
chmod +x -- "${fake_launcher}"

fake_server="${server_directory}/PalServer-Linux-Shipping"
cat > "${fake_server}" <<'SERVER'
#!/usr/bin/env bash
set -euo pipefail

stage="${UE4SS_ACCEPTANCE_STAGE_DIR:?}"
ue4ss_log="${stage}/UE4SS.log"

emit() {
    printf '%s\n' "$1"
    printf '%s\n' "$1" >> "${ue4ss_log}"
}

: > "${ue4ss_log}"
emit 'DIAG: engine_version=5.1'
for signature in \
    GUObjectArray FNameToString FNameCtorWchar GMalloc \
    StaticConstructObjectInternal FTextFString \
    GNatives ConsoleManagerSingleton UGameEngineTick; do
    diagnostic="DIAG: signature=${signature} status=resolved address=0x1"
    printf '%s\n' "${diagnostic}" >> "${ue4ss_log}"
    if [[ "${signature}" == "FNameCtorWchar" ]]; then
        printf '%s\n' 'DIAG[S_API] interleaved Steam output' ': signature=FNameCtorWchar status=resolved address=0x1'
    else
        printf '%s\n' "${diagnostic}"
    fi
done
emit 'DIAG: signature=FUObjectHashTablesGet status=missing address=0x0'

emit 'ACCEPT:LUA_MOD_STARTED'
emit 'ACCEPT:LUA_CONSOLE_REGISTERED'
emit 'ACCEPT:LUA_BLUEPRINT_HOOK'
emit 'ACCEPT:LUA_FIND_ALL count=3'
emit 'ACCEPT:LUA_PROPERTY_READ value=true'
emit 'ACCEPT:LUA_PROPERTY_WRITE value=false'
emit 'ACCEPT:LUA_PROPERTY_RESTORED value=true'
emit 'ACCEPT:LUA_FNAME value=UE4SSAcceptanceProbeObject'
emit 'ACCEPT:LUA_STATIC_CONSTRUCT_OBJECT'
emit 'ACCEPT:LUA_DUMP_OBJECTS_REQUESTED'
emit 'ACCEPT:LUA_CXX_HEADERS_COMPLETED'
emit 'ACCEPT:LUA_DUMP_USMAP_REQUESTED'
emit 'ACCEPT:CPP_START_MOD'
emit 'ACCEPT:CPP_PROGRAM_START'
emit 'ACCEPT:CPP_UNREAL_INIT'
if [[ "${FAKE_MISSING_CPP_UPDATE:-0}" != "1" ]]; then
    emit 'ACCEPT:CPP_UPDATE count=10'
fi

printf '%s\n' 'Class /Script/Pal.PalPlayerCharacter' > "${stage}/UE4SS_ObjectDump.txt"
mkdir -p -- "${stage}/CXXHeaderDump"
printf '%s\n' 'class PalPlayerCharacter;' > "${stage}/CXXHeaderDump/Pal.hpp"
printf '%s\n' 'PalPlayerCharacter' > "${stage}/Pal-5.1-test.usmap"
emit 'LogNet: GameNetDriver IpNetDriver listening on port 8211'

while true; do
    sleep 1
done
SERVER
chmod +x -- "${fake_server}"

run_harness() {
    PALWORLD_SERVER_ROOT="${palworld_root}" \
    PALWORLD_EXECUTABLE="${fake_server}" \
    UE4SS_LIBRARY="${fake_library}" \
    CPP_PROBE_LIBRARY="${fake_cpp_probe}" \
    UE4SS_LAUNCHER_SOURCE="${fake_launcher}" \
    ACCEPTANCE_STAGE_DIR="${stage_directory}" \
    SKIP_CPP_PROBE_BUILD=1 \
    KEEP_ACCEPTANCE_STAGE=1 \
    READY_TIMEOUT_SECONDS=5 \
    MARKER_TIMEOUT_SECONDS=5 \
    DUMPER_TIMEOUT_SECONDS=5 \
    "$@" \
    "${harness}"
}

run_harness env
grep -Fq 'PASS: Palworld acceptance probes completed' "${stage_directory}/acceptance-report.txt"
test -f "${stage_directory}/Mods/LuaProbeMod/enabled.txt"
test -f "${stage_directory}/Mods/CppProbeMod/dlls/main.so"

rm -rf -- "${stage_directory}"
set +e
FAKE_MISSING_CPP_UPDATE=1 run_harness env > "${test_root}/negative.log" 2>&1
negative_status=$?
set -e
if [[ ${negative_status} -eq 0 ]]; then
    echo "harness unexpectedly passed without the tenth C++ update marker" >&2
    exit 2
fi
grep -Fq 'timed out waiting for marker: ACCEPT:CPP_UPDATE count=10' "${test_root}/negative.log"

printf '%s\n' 'PASS: Palworld acceptance harness offline tests'
