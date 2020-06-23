# docker build -t xcikit:debian-11 docker/debian-11/
# docker run --rm -v $PWD:/mnt/xcikit -it xcikit:debian-11 bash

FROM debian:bullseye-slim

# Setup APT
RUN apt-get update

# GCC 10
RUN apt-get remove -y gcc-9-base && \
    apt-get install --no-install-recommends -y g++-10 && apt-get clean && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 50 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++    50 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc      50

# Dev tools
RUN apt-get install --no-install-recommends -y \
    cmake ninja-build python3-minimal curl ca-certificates && apt-get clean
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

# Install xcikit dependencies
RUN apt-get install --no-install-recommends -y \
    glslang-tools libvulkan-dev libfreetype6-dev \
    librange-v3-dev catch2 tao-pegtl-dev libbenchmark-dev && \
    apt-get clean

# Install Conan
RUN apt-get install --no-install-recommends -y python3-pip && \
    pip3 --no-cache-dir install conan && \
    conan profile new default --detect && \
    conan profile update "settings.compiler.libcxx=libstdc++11" default && \
    conan remote add xcikit https://api.bintray.com/conan/rbrich/xcikit && \
    apt-get purge -y python3-pip python3.8 && apt-get autoremove -y && apt-get clean
ENV CONAN_REVISIONS_ENABLED=1

WORKDIR /mnt/xcikit
