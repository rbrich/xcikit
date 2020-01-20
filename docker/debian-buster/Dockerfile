# docker build -t xcikit-debian:buster .
# docker run --rm -v $PWD/../..:/xcikit -it xcikit-debian:buster bash

FROM debian:buster-slim

# Setup APT
RUN apt update
RUN printf 'APT::Get::Install-Recommends "false";\nAPT::Get::Install-Suggests "false";\n' >> '/etc/apt/apt.conf.d/recommends'

# Install development tools
RUN apt install -y g++ git zip curl ca-certificates cmake ninja-build python3-pip

# Install xcikit dependencies
RUN apt install -y libfreetype6-dev libglfw3-dev

# Install Conan
RUN pip3 --no-cache-dir install conan
RUN conan profile new default --detect
RUN conan profile update "settings.compiler.libcxx=libstdc++11" default

# Cleanup
RUN apt clean
