# Debian 12 ARM64 with GCC 12
#
# CI builder (DockerHub public image), local build check:
#   docker build --platform linux/arm64/v8 --pull --build-arg UID=$(id -u) -t rbrich/xcikit-debian-arm64:12 . -f docker/debian_12-arm64.dockerfile
#   docker run --platform linux/arm64/v8 --rm -v $PWD:/src -w /src -it rbrich/xcikit-debian-arm64:12
# CMake arguments (for Clion IDE):
#   -DFORCE_COLORS=1
#   -DCONAN_OPTIONS="-o;xcikit/*:system_sdl=True;-o;xcikit/*:system_vulkan=True;-o;xcikit/*:system_freetype=True;-o;xcikit/*:system_harfbuzz=True;-o;xcikit/*:system_benchmark=True;-o;xcikit/*:system_zlib=True;-o;xcikit/*:system_range_v3=True;-o;xcikit/*:with_hyperscan=True"

FROM debian:bookworm-slim AS builder

RUN echo 'deb https://deb.debian.org/debian bookworm-backports main' > /etc/apt/sources.list.d/backports.list

RUN echo "gcc"; apt-get update && apt-get install --no-install-recommends -y \
    g++ && rm -rf /var/lib/apt/lists/*

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    gdb ca-certificates curl make ninja-build ccache \
    python3-minimal python3-pip libpython3-stdlib && \
    rm -rf /var/lib/apt/lists/*

RUN echo "xcikit deps"; apt-get update && apt-get install --no-install-recommends -y \
    librange-v3-dev libsdl2-dev glslang-tools libvulkan-dev libfreetype6-dev libharfbuzz-dev \
    libvectorscan-dev libbenchmark-dev && rm -rf /var/lib/apt/lists/*

# Install CMake 3.30.3 from backports
RUN echo "cmake"; apt-get update && apt-get install --no-install-recommends -y -t bookworm-backports cmake \
    && rm -rf /var/lib/apt/lists/*
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

RUN echo "conan"; pip3 install --no-cache-dir --break-system-packages conan

ARG UID=10002
RUN useradd -m -p np -u ${UID} -s /bin/bash builder
USER builder

COPY --chown=builder docker/conan /home/builder/conan
RUN conan config install /home/builder/conan
ENV CONAN_DEFAULT_PROFILE=linux_gcc12_arm64

# Preinstall Conan deps
ENV XCIKIT=/home/builder/xcikit
COPY --chown=builder build.sh detect_system_deps.py conanfile.py conandata.yml VERSION $XCIKIT/
RUN $XCIKIT/build.sh deps
RUN $XCIKIT/build.sh deps --debug

# Configure ccache
RUN ccache -o max_size=200M

CMD ./build.sh
