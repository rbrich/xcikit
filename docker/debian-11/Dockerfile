# docker build -t xcikit:debian-11 docker/debian-11/
# docker run --rm -v $PWD:/mnt/xcikit -it xcikit:debian-11 bash

FROM debian:bullseye-slim

# Setup APT
RUN apt-get update

# C++ compiler
RUN apt-get install --no-install-recommends -y \
    g++-10 && apt-get clean && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 50 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++    50 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc      50

# Python
RUN apt-get install --no-install-recommends -y \
    python3-pip && apt-get clean

# Dev tools
RUN apt-get install --no-install-recommends -y \
    git cmake ninja-build curl ca-certificates && apt-get clean
ENV CMAKE_GENERATOR=Ninja

# Install xcikit dependencies
RUN apt-get install --no-install-recommends -y \
    glslang-tools libvulkan-dev libfreetype6-dev libglfw3-dev libbenchmark-dev && \
    apt-get clean

# Install Conan
RUN pip3 --no-cache-dir install conan && \
    conan profile new default --detect && \
    conan profile update "settings.compiler.libcxx=libstdc++11" default && \
    conan remote add xcikit https://api.bintray.com/conan/rbrich/xcikit
ENV CONAN_REVISIONS_ENABLED=1

WORKDIR /mnt/xcikit
