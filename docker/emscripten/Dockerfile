# CI builder with Emscripten (DockerHub public image)
#   docker build -t rbrich/xcikit-emscripten docker/emscripten/
#   docker run --rm -v $PWD:/xcikit -w /xcikit -it rbrich/xcikit-emscripten bash

FROM emscripten/emsdk:latest

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    ninja-build python3-setuptools && rm -rf /var/lib/apt/lists/*

COPY conan/profile /root/.conan/profiles/default
COPY conan/settings.yml /root/.conan/settings.yml
RUN echo "conan"; pip3 --no-cache-dir install conan && \
    conan remote add xcikit https://api.bintray.com/conan/rbrich/xcikit && \
    conan config set general.revisions_enabled=1
