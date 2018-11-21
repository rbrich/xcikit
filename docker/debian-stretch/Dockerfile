# docker build -t rbrich/xcikit-debian:stretch .

FROM debian:stretch-backports

RUN apt-get update

RUN printf 'APT::Get::Install-Recommends "false";\nAPT::Get::Install-Suggests "false";\n' >> '/etc/apt/apt.conf.d/recommends'

# Install development tools
RUN apt-get install -y build-essential git curl ca-certificates
RUN apt-get install -y python3-pip
RUN apt-get install -y -t stretch-backports cmake ninja-build

# Install xcikit dependencies
RUN apt-get install -y libfreetype6-dev libglfw3-dev

# Install Conan
RUN pip3 --no-cache-dir install conan
RUN conan profile new default --detect
RUN conan profile update "settings.compiler.libcxx=libstdc++11" default

# Cleanup
RUN apt-get clean
