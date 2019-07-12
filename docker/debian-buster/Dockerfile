# docker build -t rbrich/xcikit-debian:buster .

FROM debian:buster-slim

# Setup APT
RUN apt update
RUN printf 'APT::Get::Install-Recommends "false";\nAPT::Get::Install-Suggests "false";\n' >> '/etc/apt/apt.conf.d/recommends'

# Install development tools
RUN apt install -y g++ git zip curl ca-certificates cmake ninja-build python3

# Install xcikit dependencies
RUN apt install -y libfreetype6-dev libglfw3-dev

# Install Conan 1.17
RUN curl -LO https://dl.bintray.com/conan/installers/conan-ubuntu-64_1_17_0.deb && \
    dpkg -i conan-ubuntu-64_1_17_0.deb && \
    rm conan-ubuntu-64_1_17_0.deb
RUN conan profile new default --detect
RUN conan profile update "settings.compiler.libcxx=libstdc++11" default

# Cleanup
RUN apt clean
