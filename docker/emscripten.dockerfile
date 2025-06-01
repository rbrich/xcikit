# CI builder with Emscripten (DockerHub public image), local build check:
#   podman build --platform linux/amd64 --manifest rbrich/xcikit-emscripten . -f docker/emscripten.dockerfile
#   podman build --platform linux/arm64 --build-arg ARCH=-arm64 --manifest rbrich/xcikit-emscripten . -f docker/emscripten.dockerfile
#   podman run --platform linux/amd64 --rm -v $PWD:/src -w /src -it rbrich/xcikit-emscripten
# CMake arguments (for Clion IDE):
#   -DFORCE_COLORS=1 -DXCI_WIDGETS=0 -DXCI_TEXT=0 -DXCI_GRAPHICS=0
#   -DCONAN_OPTIONS="-c;tools.cmake.cmaketoolchain:user_toolchain=['/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake']"

ARG ARCH
FROM docker.io/emscripten/emsdk:4.0.9$ARCH AS builder

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    gdb ninja-build python3-setuptools gpg git openssh-client && rm -rf /var/lib/apt/lists/*

# Ubuntu 22.04 has CMake 3.22.1, which is too old. Get newer version (https://apt.kitware.com/).
RUN echo "cmake"; apt-get purge -y cmake && \
    curl https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' > /etc/apt/sources.list.d/kitware.list && \
    apt-get update && apt-get install --no-install-recommends -y cmake && rm -rf /var/lib/apt/lists/*

RUN echo "conan"; pip3 --no-cache-dir install conan

COPY docker/conan /tmp/conan
RUN conan config install /tmp/conan
ENV CONAN_DEFAULT_PROFILE=emscripten

# Preinstall Conan deps
ENV XCIKIT=/tmp/xcikit
COPY build.sh detect_system_deps.py conanfile.py conandata.yml VERSION $XCIKIT/
RUN $XCIKIT/build.sh deps --emscripten script
RUN $XCIKIT/build.sh deps --emscripten script --debug

# Preinstall Emscripten ports (it has no explicit install command, workaround...)
RUN embuilder build zlib

CMD ./build.sh --emscripten core vfs data script
