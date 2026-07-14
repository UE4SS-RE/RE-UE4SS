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

output="$(
    UE4SS_DISABLE_AUTO_START=1 \
    LD_PRELOAD="${existing_preload}" \
    "${launcher}" "${target}" "arg with spaces" "literal*glob" ""
)"

actual_preload="$(sed -n 's/^LD_PRELOAD=//p' <<<"${output}")"
if [[ "${actual_preload}" != /proc/self/fd/*":${existing_preload}" ]]; then
    echo "launcher did not prepend its inherited UE4SS handle: ${actual_preload}" >&2
    exit 3
fi
grep -Fqx "UE4SS_MODULE_PATH=${launcher_directory}/libUE4SS.so" <<<"${output}"
grep -Eq '^PRELOAD_OWNER=/proc/self/fd/[0-9]+$' <<<"${output}"
grep -Fqx "ARGC=4" <<<"${output}"
grep -Fqx "ARG[0]=${target}" <<<"${output}"
grep -Fqx "ARG[1]=arg with spaces" <<<"${output}"
grep -Fqx "ARG[2]=literal*glob" <<<"${output}"
grep -Fqx "ARG[3]=" <<<"${output}"

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
    exit 4
fi
grep -Fq "libUE4SS.so not found" <<<"${missing_library_output}"

set +e
missing_target_output="$("${launcher}" "${stage_directory}/missing target" 2>&1)"
missing_target_status=$?
set -e
if [[ ${missing_target_status} -eq 0 ]]; then
    echo "launcher unexpectedly succeeded with a missing target" >&2
    exit 5
fi
grep -Fq "target executable not found" <<<"${missing_target_output}"

rm -rf -- "${stage_directory}"
