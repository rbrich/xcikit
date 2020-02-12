# Test build in Docker:
#   docker build -t xcikit-build .
#   docker run --rm -it xcikit-build

FROM rbrich/xcikit-debian:buster

WORKDIR /opt/xcikit

ADD benchmarks benchmarks/
ADD cmake cmake/
ADD examples examples/
ADD ext ext/
ADD share share/
ADD src src/
ADD tests tests/
ADD tools tools/
ADD bootstrap.sh \
    CMakeLists.txt \
    conanfile.py \
    config.h.in \
    xcikitConfig.cmake.in \
    build.sh \
    ./

RUN ./bootstrap.sh -y

# actual build is executed as default command so we have full TTY with colors
CMD ./build.sh -G Ninja
