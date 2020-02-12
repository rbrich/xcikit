# docker build -t xcikit-debian:buster .
# docker run --rm -v $PWD/../..:/xcikit -it xcikit-debian:buster bash

FROM debian:buster-slim

# Setup APT
RUN apt-get update
RUN printf 'APT::Get::Install-Recommends "false";\nAPT::Get::Install-Suggests "false";\n' \
    >> '/etc/apt/apt.conf.d/recommends'

# Install development tools
RUN apt-get install -y g++ git zip curl ca-certificates \
    cmake ninja-build python3-pip glslang-tools

# Install xcikit dependencies
RUN apt-get install -y libfreetype6-dev libglfw3-dev libbenchmark-dev

# Install Conan
RUN pip3 --no-cache-dir install conan
RUN conan profile new default --detect
RUN conan profile update "settings.compiler.libcxx=libstdc++11" default

# Cleanup
RUN apt-get clean
