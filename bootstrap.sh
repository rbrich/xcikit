#!/usr/bin/env bash
#
# Download/generate files required for build or runtime
#

cd $(dirname "$0")

if [ ! -e "ext/glad/glad.h" ] ; then
    echo "=== Generate Glad ==="
    pip install --user glad
    ~/.local/bin/glad --profile="core" --api="gl=3.3" --generator="c" --spec="gl" \
        --no-loader --local-files --omit-khrplatform --extensions="GL_KHR_debug" \
        --out-path ext/glad/
fi

if [ ! -e "share/fonts/Hack/Hack-Regular.ttf" ] ; then
    echo "=== Download Hack font ==="
    HACK_VERSION="v3.003"
    HACK_ARCHIVE="Hack-${HACK_VERSION}-ttf.tar.gz"
    curl -LO https://github.com/source-foundry/Hack/releases/download/${HACK_VERSION}/${HACK_ARCHIVE}
    tar xf ${HACK_ARCHIVE} -C share/fonts
    mv share/fonts/ttf share/fonts/Hack
    rm ${HACK_ARCHIVE}
fi
