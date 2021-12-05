# CI builder with Emscripten (DockerHub public image)
#   docker build --build-arg UID=$(id -u) -t rbrich/xcikit-emscripten . -f docker/emscripten/Dockerfile
#   docker run --rm -v $PWD:/xcikit -w /xcikit -it rbrich/xcikit-emscripten bash
# CMake arguments (for Clion IDE):
#   -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
#   -DFORCE_COLORS=1 -DCONAN_INSTALL=1 -DXCI_WIDGETS=0 -DXCI_TEXT=0 -DXCI_GRAPHICS=0

FROM emscripten/emsdk:latest AS builder

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    gdb ninja-build python3-setuptools && rm -rf /var/lib/apt/lists/*

RUN echo "conan"; pip3 --no-cache-dir install conan

ARG UID=1000
RUN useradd -m -p np -u ${UID} -s /bin/bash builder
USER builder

ENV CONAN_USER_HOME=/home/builder
COPY --chown=builder docker/emscripten/conan/profile $CONAN_USER_HOME/.conan/profiles/default
COPY --chown=builder docker/emscripten/conan/settings.yml $CONAN_USER_HOME/.conan/settings.yml
RUN conan config set general.revisions_enabled=1

# Preinstall Conan deps
ENV XCIKIT=$CONAN_USER_HOME/xcikit
COPY --chown=builder build.sh detect_system_deps.py conanfile.py conandata.yml VERSION $XCIKIT/
RUN $XCIKIT/build.sh deps --emscripten script
RUN $XCIKIT/build.sh deps --emscripten script --debug
