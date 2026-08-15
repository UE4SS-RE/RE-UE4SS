#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
output_directory="${1:-${repository_root}/build/container-package}"
build_jobs="${UE4SS_BUILD_JOBS:-2}"

if [[ ! "${build_jobs}" =~ ^[1-9][0-9]*$ ]]; then
    echo "UE4SS_BUILD_JOBS must be a positive integer" >&2
    exit 2
fi

git -C "${repository_root}" submodule update --init deps/first/patternsleuth deps/first/Unreal
mkdir -p "${output_directory}"

image_name="ue4ss-linux-artifact:local"
git_sha="$(git -C "${repository_root}" rev-parse --short HEAD 2>/dev/null || echo unknown)"
if [[ -n "$(git -C "${repository_root}" status --porcelain --untracked-files=normal)" ]]; then
    git_sha="${git_sha}-dirty"
fi
docker build \
    --file "${repository_root}/linux/docker/Dockerfile.builder" \
    --target artifact \
    --tag "${image_name}" \
    --build-arg "UE4SS_GIT_SHA=${git_sha}" \
    --build-arg "UE4SS_BUILD_JOBS=${build_jobs}" \
    "${repository_root}"

container_id="$(docker create "${image_name}")"
trap 'docker rm -f "${container_id}" >/dev/null 2>&1 || true' EXIT
artifact_name="UE4SS-Linux-x86_64-${git_sha}.tar.gz"
docker cp "${container_id}:/${artifact_name}" "${output_directory}/${artifact_name}"

echo "UE4SS Linux package written to ${output_directory}/${artifact_name}"
