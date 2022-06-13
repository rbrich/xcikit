# Ubuntu 22.04 with Clang-Tidy 14
#
# CI builder (DockerHub public image), local build check:
#   docker build --pull --build-arg UID=$(id -u) -t rbrich/xcikit-ubuntu . -f docker/ubuntu/Dockerfile
#   docker run --rm -v $PWD:/src -w /src -it rbrich/xcikit-ubuntu
# CMake arguments (for Clion IDE):
#   -DFORCE_COLORS=1 -DCONAN_INSTALL=1
#   -DCONAN_OPTIONS="xcikit:system_glfw=True;xcikit:system_vulkan=True;xcikit:system_freetype=True;xcikit:system_harfbuzz=True;xcikit:system_benchmark=True;xcikit:system_zlib=True;xcikit:system_range_v3=True;xcikit:with_hyperscan=True"

FROM ubuntu:jammy AS builder

# Clang with libstdc++. Alternatively, install also: libc++-dev libc++abi-dev
RUN echo "clang"; apt-get update && apt-get install --no-install-recommends -y \
    clang clang-tidy && rm -rf /var/lib/apt/lists/*

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    gdb ca-certificates curl cmake make ninja-build \
    python3-minimal python3-pip libpython3-stdlib && \
    rm -rf /var/lib/apt/lists/*
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

RUN echo "xcikit deps"; apt-get update && apt-get install --no-install-recommends -y \
    librange-v3-dev libglfw3-dev glslang-tools libvulkan-dev libfreetype6-dev libharfbuzz-dev \
    libhyperscan-dev libbenchmark-dev && rm -rf /var/lib/apt/lists/*

RUN echo "conan"; pip3 --no-cache-dir install conan

ARG UID=10002
RUN useradd -m -p np -u ${UID} -s /bin/bash builder
USER builder

ENV CONAN_USER_HOME=/home/builder
RUN conan profile new default --detect && \
    conan profile update "settings.compiler.libcxx=libstdc++11" default && \
    conan profile update "settings.compiler.cppstd=20" default && \
    conan config set general.revisions_enabled=1

# Preinstall Conan deps
ENV XCIKIT=/home/builder/xcikit
COPY --chown=builder build.sh detect_system_deps.py conanfile.py conandata.yml VERSION $XCIKIT/
RUN $XCIKIT/build.sh deps
RUN $XCIKIT/build.sh deps --debug

CMD ./build.sh
