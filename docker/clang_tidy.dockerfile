# Debian 13 with Clang-Tidy 19
#
# CI builder (DockerHub public image), local build check:
#   docker buildx build --platform linux/amd64,linux/arm64/v8 --pull --build-arg UID=$(id -u) -t rbrich/xcikit-tidy:19 . -f docker/clang_tidy.dockerfile
#   docker run --platform linux/amd64 --rm -v $PWD:/src -w /src -it rbrich/xcikit-tidy:19
#   docker run --platform linux/arm64/v8 --rm -v $PWD:/src -w /src -it rbrich/xcikit-tidy:19
# CMake arguments (for Clion IDE):
#   -DFORCE_COLORS=1
#   -DCONAN_OPTIONS="-o;xcikit/*:system_sdl=True;-o;xcikit/*:system_vulkan=True;-o;xcikit/*:system_freetype=True;-o;xcikit/*:system_harfbuzz=True;-o;xcikit/*:system_benchmark=True;-o;xcikit/*:system_zlib=True;-o;xcikit/*:system_range_v3=True;-o;xcikit/*:with_hyperscan=True"

FROM debian:trixie-slim AS builder

# Clang with libstdc++. Alternatively, install also: libc++-dev libc++abi-dev
RUN echo "clang"; apt-get update && apt-get install --no-install-recommends -y \
    clang clang-tidy && rm -rf /var/lib/apt/lists/*

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    gdb ca-certificates curl cmake make ninja-build ccache git openssh-client \
    python3-minimal python3-pip libpython3-stdlib && \
    rm -rf /var/lib/apt/lists/*
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

RUN echo "xcikit deps"; apt-get update && apt-get install --no-install-recommends -y \
    librange-v3-dev libsdl3-dev glslang-tools libvulkan-dev libfreetype6-dev libharfbuzz-dev \
    libhyperscan-dev libbenchmark-dev && rm -rf /var/lib/apt/lists/*

RUN echo "conan"; pip3 install --no-cache-dir --break-system-packages conan

ARG UID=10002
RUN useradd -m -p np -u ${UID} -s /bin/bash builder
USER builder

COPY --chown=builder docker/conan /home/builder/conan
RUN conan config install /home/builder/conan && conan profile detect
ENV CONAN_DEFAULT_PROFILE=clang19

# Preinstall Conan deps
ENV XCIKIT=/home/builder/xcikit
COPY --chown=builder build.sh detect_system_deps.py conanfile.py conandata.yml VERSION $XCIKIT/
RUN $XCIKIT/build.sh deps
RUN $XCIKIT/build.sh deps --debug

# Configure ccache
RUN ccache -o max_size=200M

CMD ./build.sh --tidy -DCMAKE_DISABLE_PRECOMPILE_HEADERS=1
