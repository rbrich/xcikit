# docker build -t xcikit-build-test .
# docker run --rm -it xcikit-build-test

FROM rbrich/xcikit-debian:stretch

WORKDIR /opt/xcikit

ADD .conan .conan/
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
    ./

RUN ./bootstrap.sh

RUN conan profile new --detect default && \
    conan profile update settings.compiler.libcxx="libstdc++11" default

RUN mkdir build
WORKDIR /opt/xcikit/build

RUN conan install .. --build glad

RUN cmake ..
RUN make

CMD make test
