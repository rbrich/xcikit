# CI builder (public):
#   docker build --target builder -t rbrich/xcikit:debian-11 docker/debian-11/
#   docker run --rm -v $PWD:/xcikit -w /xcikit -it rbrich/xcikit:debian-11 bash
# SSH builder (local, for Clion "Remote Host" toolchain):
#   docker build --target builder-ssh --build-arg rootpw=pass1 -t xcikit-ssh:debian-11 docker/debian-11/
#   docker run -p 222:22 -v $PWD:/xcikit --name xcikit-ssh -w /xcikit xcikit-ssh:debian-11
#   ssh root@localhost -p 222

FROM debian:bullseye-slim AS builder

RUN echo "gcc 10"; apt-get update && apt-get install --no-install-recommends -y \
    g++-10 && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 50 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++    50 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc      50

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    cmake ninja-build python3-minimal libpython3-stdlib && rm -rf /var/lib/apt/lists/*
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

RUN echo "conan"; apt-get update && apt-mark manual $(apt-mark showauto) && \
    apt-get install --no-install-recommends -y \
    python3-pip && pip3 --no-cache-dir install conan && \
    conan profile new default --detect && \
    conan profile update "settings.compiler.libcxx=libstdc++11" default && \
    conan remote add xcikit https://api.bintray.com/conan/rbrich/xcikit && \
    apt-get purge -y python3-pip $(apt-mark showauto) && \
    rm -rf /var/lib/apt/lists/*
ENV CONAN_REVISIONS_ENABLED=1

RUN echo "xcikit deps"; apt-get update && apt-get install --no-install-recommends -y \
    glslang-tools libvulkan-dev libfreetype6-dev librange-v3-dev \
    tao-pegtl-dev libhyperscan-dev catch2 libbenchmark-dev && rm -rf /var/lib/apt/lists/*

ADD 'https://github.com/source-foundry/Hack/releases/download/v3.003/Hack-v3.003-ttf.tar.gz' /srv

# https://docs.docker.com/engine/examples/running_ssh_service/
FROM builder AS builder-ssh

ARG rootpw

RUN echo "ssh"; apt-get update && apt-get install --no-install-recommends -y openssh-server && \
    echo "root:$rootpw" | chpasswd && mkdir /var/run/sshd && \
    sed -i 's/#*PermitRootLogin prohibit-password/PermitRootLogin yes/g' /etc/ssh/sshd_config && \
    sed -i 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' /etc/pam.d/sshd && \
    printf '#!/bin/sh\nexit 0\n' > /usr/sbin/policy-rc.d

# additional development tools
RUN apt-get install --no-install-recommends -y git gdb make ca-certificates

EXPOSE 22
CMD ["/usr/sbin/sshd", "-D", "-d"]
