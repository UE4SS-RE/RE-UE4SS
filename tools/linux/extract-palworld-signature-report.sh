#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
repo_root="$(cd -- "${script_dir}/../.." && pwd -P)"
server_root="${1:-${PALWORLD_SERVER_ROOT:-}}"

if [[ -z "${server_root}" ]]; then
    echo "usage: $0 <Palworld server root> [resolver ...]" >&2
    echo "or set PALWORLD_SERVER_ROOT" >&2
    exit 2
fi

if [[ $# -gt 0 ]]; then
    shift
fi

binary="${server_root}/Pal/Binaries/Linux/PalServer-Linux-Shipping"
manifest="${server_root}/steamapps/appmanifest_2394010.acf"

if [[ ! -f "${binary}" ]]; then
    echo "Palworld executable not found: ${binary}" >&2
    exit 3
fi

resolvers=(
    EngineVersion
    GUObjectArray
    FNameCtorWchar
    FNameToString
    GMalloc
    StaticConstructObjectInternal
    FTextFString
    FUObjectHashTablesGet
    GNatives
    ConsoleManagerSingleton
    UGameEngineTick
)
if [[ $# -gt 0 ]]; then
    resolvers=("$@")
fi

echo "Palworld Linux signature report"
echo "repository: ${repo_root}"
echo "executable: ${binary}"
if [[ -f "${manifest}" ]]; then
    awk -F '"' '/"appid"|"buildid"|"LastUpdated"/ {printf "%s: %s\n", $2, $4}' "${manifest}"
else
    echo "manifest: missing (${manifest})"
fi
echo "size: $(stat -c '%s' "${binary}")"
echo "sha256: $(sha256sum "${binary}" | awk '{print $1}')"
echo "elf_build_id: $(readelf -n "${binary}" | awk '/Build ID:/ {print $3; exit}')"
echo "engine_markers:"
strings -a -e l "${binary}" | grep -E -m 3 '\+\+UE5\+Release-5\.1|Jun 22 2026' || true
echo "load_segments:"
readelf -W -l "${binary}" | awk '$1 == "LOAD" {print}'
echo "resolver_results:"

scan_args=()
for resolver in "${resolvers[@]}"; do
    scan_args+=(--resolver "${resolver}")
done

exec cargo run \
    --quiet \
    --manifest-path "${repo_root}/deps/first/patternsleuth/Cargo.toml" \
    -p patternsleuth_cli \
    --release \
    -- scan \
    --path "${binary}" \
    "${scan_args[@]}" \
    --summary
