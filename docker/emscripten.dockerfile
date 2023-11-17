# CI builder with Emscripten (DockerHub public image), local build check:
#   docker build --pull --build-arg UID=$(id -u) -t rbrich/xcikit-emscripten . -f docker/emscripten.dockerfile
#   docker run --rm -v $PWD:/src -w /src -it rbrich/xcikit-emscripten
# CMake arguments (for Clion IDE):
#   -DFORCE_COLORS=1 -DXCI_WIDGETS=0 -DXCI_TEXT=0 -DXCI_GRAPHICS=0
#   -DCONAN_OPTIONS="-c;tools.cmake.cmaketoolchain:user_toolchain=['/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake']"

FROM emscripten/emsdk:3.1.48 AS builder

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    gdb ninja-build python3-setuptools && rm -rf /var/lib/apt/lists/*

RUN echo "conan"; pip3 --no-cache-dir install conan

ARG UID=10001
RUN useradd -m -p np -u ${UID} -s /bin/bash builder
USER builder

COPY --chown=builder docker/conan /home/builder/conan
RUN conan config install /home/builder/conan
ENV CONAN_DEFAULT_PROFILE=emscripten

# Preinstall Conan deps
ENV XCIKIT=/home/builder/xcikit
COPY --chown=builder build.sh detect_system_deps.py conanfile.py conandata.yml VERSION $XCIKIT/
RUN $XCIKIT/build.sh deps --emscripten script
RUN $XCIKIT/build.sh deps --emscripten script --debug

# Preinstall Emscripten ports (it has no explicit install command, workaround...)
RUN echo "int main(){}" > ~/dummy.c
RUN cd ; emcc -s USE_ZLIB=1 dummy.c -o dummy.out && rm dummy.*

CMD ./build.sh --emscripten core vfs data script
