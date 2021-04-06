#!/usr/bin/env bash
#
# Download/generate files required for build or runtime
#

set -e
cd "$(dirname "$0")"

function run { echo "$@" ; "$@"; }

# See: https://github.com/source-foundry/Hack
if [[ ! -e "share/fonts/Hack/Hack-Regular.ttf" ]] ; then
    echo "=== Download Hack font ==="
    HACK_VERSION="v3.003"
    HACK_ARCHIVE="Hack-${HACK_VERSION}-ttf.tar.gz"
    run curl -LO "https://github.com/source-foundry/Hack/releases/download/${HACK_VERSION}/${HACK_ARCHIVE}"
    run tar xf ${HACK_ARCHIVE} -C share/fonts
    run mv share/fonts/ttf share/fonts/Hack
    run rm ${HACK_ARCHIVE}
fi
