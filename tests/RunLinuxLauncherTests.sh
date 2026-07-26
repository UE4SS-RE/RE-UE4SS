#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
    echo "usage: $0 <launcher-source> <libUE4SS.so> <probe> <existing-preload.so> <stage-directory>" >&2
    exit 2
fi

launcher_source="$1"
ue4ss_library="$2"
probe_source="$3"
existing_preload="$4"
stage_directory="$5"

rm -rf -- "${stage_directory}"
launcher_directory="${stage_directory}/launcher path with spaces"
target_directory="${stage_directory}/target path with spaces"
mkdir -p -- "${launcher_directory}" "${target_directory}"

launcher="${launcher_directory}/run_ue4ss.sh"
target="${target_directory}/server probe"
cp -- "${launcher_source}" "${launcher}"
cp -- "${ue4ss_library}" "${launcher_directory}/libUE4SS.so"
cp -- "${probe_source}" "${target}"
chmod +x -- "${launcher}" "${target}"

existing_preload_value="${existing_preload}:${existing_preload}"
output="$(
    UE4SS_DISABLE_AUTO_START=1 \
    UE4SS_LAUNCH_TARGET_EXE=/stale/target \
    UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=0 \
    UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=stale.so \
    UE4SS_MODULE_PATH=/stale/libUE4SS.so \
    LD_PRELOAD="${existing_preload_value}" \
    "${launcher}" "${target}" "arg with spaces" "literal*glob" ""
)"

actual_preload="$(sed -n 's/^LD_PRELOAD=//p' <<<"${output}")"
if [[ "${actual_preload}" != /proc/self/fd/*":${existing_preload_value}" ]]; then
    echo "launcher did not preserve the inherited preload list: ${actual_preload}" >&2
    exit 3
fi
canonical_target="$(readlink -f -- "${target}")"
grep -Fqx "UE4SS_MODULE_PATH=${launcher_directory}/libUE4SS.so" <<<"${output}"
grep -Fqx "UE4SS_LAUNCH_TARGET_EXE=${canonical_target}" <<<"${output}"
grep -Fqx 'UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=1' <<<"${output}"
grep -Fqx "UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=${existing_preload_value}" <<<"${output}"
grep -Eq '^PRELOAD_OWNER=/proc/self/fd/[0-9]+$' <<<"${output}"
grep -Fqx "ARGC=4" <<<"${output}"
grep -Fqx "ARG[0]=${target}" <<<"${output}"
grep -Fqx "ARG[1]=arg with spaces" <<<"${output}"
grep -Fqx "ARG[2]=literal*glob" <<<"${output}"
grep -Fqx "ARG[3]=" <<<"${output}"

empty_preload_output="$(
    LD_PRELOAD= \
    UE4SS_DISABLE_AUTO_START=1 \
    "${launcher}" "${target}"
)"
grep -Fqx 'UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=1' <<<"${empty_preload_output}"
grep -Fqx 'UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=' <<<"${empty_preload_output}"
if [[ -e "${launcher_directory}/UE4SS.log" ]]; then
    echo "UE4SS_DISABLE_AUTO_START did not override launcher activation" >&2
    exit 4
fi

wrapper="${target_directory}/server wrapper.sh"
cat > "${wrapper}" <<WRAPPER
#!/usr/bin/env bash
set -euo pipefail
exec "${target}" "\$@"
WRAPPER
chmod +x -- "${wrapper}"

wrapper_output="$(
    env -u LD_PRELOAD \
        UE4SS_DISABLE_AUTO_START=1 \
        "${launcher}" --host-executable "${target}" "${wrapper}" "wrapped arg"
)"
grep -Fqx "UE4SS_LAUNCH_TARGET_EXE=${canonical_target}" <<<"${wrapper_output}"
grep -Fqx 'UE4SS_LAUNCH_LD_PRELOAD_WAS_SET=0' <<<"${wrapper_output}"
grep -Fqx 'UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD=' <<<"${wrapper_output}"
grep -Fqx 'ARGC=2' <<<"${wrapper_output}"
grep -Fqx 'ARG[1]=wrapped arg' <<<"${wrapper_output}"

missing_library_directory="${stage_directory}/missing library"
mkdir -p -- "${missing_library_directory}"
cp -- "${launcher_source}" "${missing_library_directory}/run_ue4ss.sh"
chmod +x -- "${missing_library_directory}/run_ue4ss.sh"
set +e
missing_library_output="$("${missing_library_directory}/run_ue4ss.sh" "${target}" 2>&1)"
missing_library_status=$?
set -e
if [[ ${missing_library_status} -eq 0 ]]; then
    echo "launcher unexpectedly succeeded without libUE4SS.so" >&2
    exit 5
fi
grep -Fq "libUE4SS.so not found" <<<"${missing_library_output}"

set +e
missing_target_output="$("${launcher}" "${stage_directory}/missing target" 2>&1)"
missing_target_status=$?
set -e
if [[ ${missing_target_status} -eq 0 ]]; then
    echo "launcher unexpectedly succeeded with a missing target" >&2
    exit 6
fi
grep -Fq "target executable not found" <<<"${missing_target_output}"

set +e
script_without_host_output="$("${launcher}" "${wrapper}" 2>&1)"
script_without_host_status=$?
set -e
if [[ ${script_without_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a script without --host-executable" >&2
    exit 7
fi
grep -Fq 'script commands require --host-executable' <<<"${script_without_host_output}"

set +e
non_elf_host_output="$("${launcher}" --host-executable "${wrapper}" "${wrapper}" 2>&1)"
non_elf_host_status=$?
set -e
if [[ ${non_elf_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a non-ELF host" >&2
    exit 8
fi
grep -Fq 'host executable is not an ELF file' <<<"${non_elf_host_output}"

set +e
missing_host_output="$("${launcher}" --host-executable "${stage_directory}/missing host" "${wrapper}" 2>&1)"
missing_host_status=$?
set -e
if [[ ${missing_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a missing host" >&2
    exit 9
fi
grep -Fq 'host executable not found or not executable' <<<"${missing_host_output}"

non_executable_host="${target_directory}/non executable host"
cp -- "${target}" "${non_executable_host}"
chmod -x -- "${non_executable_host}"
set +e
non_executable_host_output="$("${launcher}" --host-executable "${non_executable_host}" "${wrapper}" 2>&1)"
non_executable_host_status=$?
set -e
if [[ ${non_executable_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a non-executable host" >&2
    exit 10
fi
grep -Fq 'host executable not found or not executable' <<<"${non_executable_host_output}"

non_regular_host="${target_directory}/host directory"
mkdir -- "${non_regular_host}"
set +e
non_regular_host_output="$("${launcher}" --host-executable "${non_regular_host}" "${wrapper}" 2>&1)"
non_regular_host_status=$?
set -e
if [[ ${non_regular_host_status} -eq 0 ]]; then
    echo "launcher unexpectedly accepted a non-regular host" >&2
    exit 11
fi
grep -Fq 'host executable not found or not executable' <<<"${non_regular_host_output}"

rm -rf -- "${stage_directory}"
