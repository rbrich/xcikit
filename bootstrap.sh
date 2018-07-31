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

# See: https://github.com/source-foundry/Hack
if [ ! -e "share/fonts/Hack/Hack-Regular.ttf" ] ; then
    echo "=== Download Hack font ==="
    HACK_VERSION="v3.003"
    HACK_ARCHIVE="Hack-${HACK_VERSION}-ttf.tar.gz"
    curl -LO "https://github.com/source-foundry/Hack/releases/download/${HACK_VERSION}/${HACK_ARCHIVE}"
    tar xf ${HACK_ARCHIVE} -C share/fonts
    mv share/fonts/ttf share/fonts/Hack
    rm ${HACK_ARCHIVE}
fi

# See: https://github.com/martinmoene/string-view-lite
if [ ! -e "ext/nonstd/string_view.hpp" ] ; then
    echo "=== Download string_view lite (nonstd) ==="
    curl -L "https://raw.githubusercontent.com/martinmoene/string-view-lite/master/include/nonstd/string_view.hpp" \
         -o "ext/nonstd/string_view.hpp"
fi
