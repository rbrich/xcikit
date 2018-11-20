# docker build -t xcikit-build-test .
# docker run --rm -it xcikit-build-test

FROM rbrich/xcikit-debian:stretch

WORKDIR /opt/xcikit

ADD cmake cmake/
ADD examples examples/
ADD ext ext/
ADD share share/
ADD src src/
ADD tests tests/
ADD bootstrap.sh \
    CMakeLists.txt \
    conanfile.txt \
    config.h.in \
    xcikitConfig.cmake.in \
    build.sh \
    ./

RUN ./bootstrap.sh -y

WORKDIR /opt/xcikit
CMD ./build.sh
