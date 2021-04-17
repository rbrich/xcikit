# CI builder (DockerHub public image)
#   docker build --target builder -t rbrich/xcikit-debian docker/debian/
#   docker run --rm -v $PWD:/xcikit -w /xcikit -it rbrich/xcikit-debian bash
#
# SSH builder (for Clion "Remote Host" toolchain)
# see https://blog.jetbrains.com/clion/2020/01/using-docker-with-clion/)
#   docker build --target builder-ssh --build-arg rootpw=pass1 -t xcikit-debian-ssh docker/debian/
#   docker run -p 127.0.0.1:2222:22 --cap-add sys_ptrace --name xcikit-debian-ssh xcikit-debian-ssh
#   ssh root@localhost -p 2222
# Cleanup known hosts:
#   ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"
# Example cmake arguments:
#   -DRUN_CONAN=1 -DFORCE_COLORS=1

FROM debian:bullseye-slim AS builder

RUN echo "gcc 10"; apt-get update && apt-get install --no-install-recommends -y \
    g++-10 && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 50 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 50 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++    50 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc      50

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    ca-certificates curl cmake ninja-build python3-minimal libpython3-stdlib && \
    rm -rf /var/lib/apt/lists/*
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

RUN echo "conan"; apt-get update && apt-mark manual $(apt-mark showauto) && \
    apt-get install --no-install-recommends -y \
    python3-pip && pip3 --no-cache-dir install conan && \
    conan profile new default --detect && \
    conan profile update "settings.compiler.libcxx=libstdc++11" default && \
    conan config set general.revisions_enabled=1 && \
    apt-get purge -y python3-pip $(apt-mark showauto) && \
    rm -rf /var/lib/apt/lists/*

RUN echo "xcikit deps"; apt-get update && apt-get install --no-install-recommends -y \
    librange-v3-dev libglfw3-dev glslang-tools libvulkan-dev libfreetype6-dev \
    libhyperscan-dev catch2 libbenchmark-dev && rm -rf /var/lib/apt/lists/*


# --------------------------------------------------------------------------------------------------
# https://docs.docker.com/engine/examples/running_ssh_service/
FROM builder AS builder-ssh

ARG rootpw

RUN apt-get update && apt-get install --no-install-recommends -y gdb git make rsync openssh-server
RUN echo "root:$rootpw" | chpasswd && mkdir /run/sshd && \
    echo 'PermitRootLogin yes' > "/etc/ssh/sshd_config.d/docker.conf"

EXPOSE 22

# -D    do not become a daemon
CMD ["/usr/sbin/sshd", "-D"]
