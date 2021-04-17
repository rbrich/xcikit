# CI builder with Emscripten (DockerHub public image)
#   docker build --target builder -t rbrich/xcikit-emscripten docker/emscripten/
#   docker run --rm -v $PWD:/xcikit -w /xcikit -it rbrich/xcikit-emscripten bash
#
# SSH builder (for Clion "Remote Host" toolchain)
# see https://blog.jetbrains.com/clion/2020/01/using-docker-with-clion/)
#   docker build --target builder-ssh --build-arg rootpw=pass1 -t xcikit-emscripten-ssh docker/emscripten/
#   docker run -p 127.0.0.1:2223:22 --cap-add sys_ptrace --name xcikit-emscripten-ssh xcikit-emscripten-ssh
#   ssh root@localhost -p 2223
# Cleanup known hosts:
#   ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2223"
# Example cmake arguments:
#   -DCMAKE_TOOLCHAIN_FILE=/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake
#   -DRUN_CONAN=1 -DXCI_GRAPHICS=0 -DXCI_DATA=0 -DFORCE_COLORS=1

FROM emscripten/emsdk:latest AS builder

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    ninja-build python3-setuptools && rm -rf /var/lib/apt/lists/*

COPY conan/profile /root/.conan/profiles/default
COPY conan/settings.yml /root/.conan/settings.yml
RUN echo "conan"; pip3 --no-cache-dir install conan && \
    conan config set general.revisions_enabled=1

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
