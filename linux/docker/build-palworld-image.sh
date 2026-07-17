#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 <palworld-server-root> <ue4ss-package-or-directory> [image-tag]" >&2
}

if [[ $# -lt 2 || $# -gt 3 ]]; then
    usage
    exit 2
fi
if ! command -v docker >/dev/null || ! command -v rsync >/dev/null || ! command -v sha256sum >/dev/null; then
    echo "docker, rsync, and sha256sum are required" >&2
    exit 2
fi

server_root=$(realpath "$1")
ue4ss_source=$(realpath "$2")
image_tag=${3:-palworld-ue4ss-linux:experimental}
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
server_binary="$server_root/Pal/Binaries/Linux/PalServer-Linux-Shipping"
if [[ ! -x "$server_binary" ]]; then
    echo "PalServer-Linux-Shipping was not found under $server_root" >&2
    exit 2
fi

context=$(mktemp -d)
cleanup() {
    rm -rf "$context"
}
trap cleanup EXIT INT TERM

mkdir -p "$context/server" "$context/ue4ss"
rsync -a --delete --exclude=/Pal/Saved/ "$server_root/" "$context/server/"
if [[ -d "$ue4ss_source" ]]; then
    rsync -a "$ue4ss_source/" "$context/ue4ss/"
else
    tar -xzf "$ue4ss_source" -C "$context/ue4ss"
fi
if [[ ! -f "$context/ue4ss/libUE4SS.so" || ! -f "$context/ue4ss/libue4ss_preload.so" ]]; then
    echo "UE4SS input does not contain libUE4SS.so and libue4ss_preload.so at its root" >&2
    exit 2
fi

sha256sum "$context/server/Pal/Binaries/Linux/PalServer-Linux-Shipping" >"$context/palserver.sha256"
(
    cd "$context/ue4ss"
    sha256sum libUE4SS.so >libUE4SS.so.sha256
)
cp "$script_dir/palserver-entrypoint.sh" "$context/palserver-entrypoint.sh"
cp "$script_dir/ue4ss-healthcheck.sh" "$context/ue4ss-healthcheck.sh"

docker build \
    --file "$script_dir/Dockerfile.palworld-experimental" \
    --tag "$image_tag" \
    "$context"

echo "experimental image built: $image_tag"
