FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

# Security updates intentionally follow the Ubuntu 22.04 repository while the
# distro, compiler, Rust toolchain, and source revisions remain pinned.
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
# hadolint ignore=DL3008
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        git \
        gnupg \
        ninja-build \
        xz-utils \
    && curl --fail --silent --show-error https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor -o /usr/share/keyrings/llvm-snapshot.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm-snapshot.gpg] http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" \
        > /etc/apt/sources.list.d/llvm-18.list \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        clang-18 \
        cmake \
        libc++-18-dev \
        libc++abi-18-dev \
        lld-18 \
    && rm -rf /var/lib/apt/lists/*

ARG RUST_VERSION=1.88.0

ENV RUSTUP_HOME=/opt/rustup \
    CARGO_HOME=/opt/cargo \
    PATH=/opt/cargo/bin:${PATH} \
    CC=clang-18 \
    CXX=clang++-18 \
    CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER=clang-18

RUN curl --proto '=https' --tlsv1.2 --fail --silent --show-error https://sh.rustup.rs \
        | sh -s -- -y --profile minimal --default-toolchain "${RUST_VERSION}"

ARG UE4SS_GIT_SHA=unknown
ARG UE4SS_WITH_UNREAL_ABI=ON
ARG UE4SS_WITH_UNREAL_SOURCE_GATE=ON
ARG UE4SS_WITH_UNREAL_RUNTIME=ON
ARG UE4SS_BUILD_JOBS=2

WORKDIR /src
COPY . .

RUN test -f deps/first/patternsleuth/patternsleuth/Cargo.toml \
    && cmake -S . -B build/linux-release -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=clang-18 \
        -DCMAKE_CXX_COMPILER=clang++-18 \
        -DBUILD_TESTING=ON \
        -DUE4SS_LINUX_MVP=ON \
        -DUE4SS_USE_LIBCXX=ON \
        -DUE4SS_WITH_UNREAL_ABI="${UE4SS_WITH_UNREAL_ABI}" \
        -DUE4SS_WITH_UNREAL_SOURCE_GATE="${UE4SS_WITH_UNREAL_SOURCE_GATE}" \
        -DUE4SS_WITH_UNREAL_RUNTIME="${UE4SS_WITH_UNREAL_RUNTIME}" \
        -DUE4SS_LINUX_GIT_SHA_OVERRIDE="${UE4SS_GIT_SHA}" \
    && cmake --build build/linux-release --parallel "${UE4SS_BUILD_JOBS}" \
    && ctest --test-dir build/linux-release --output-on-failure \
    && cmake --build build/linux-release --target package

FROM scratch AS artifact
COPY --from=builder /src/build/linux-release/UE4SS-Linux-x86_64-*.tar.gz /
CMD ["/artifact-only-image"]
