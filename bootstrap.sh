#!/usr/bin/env bash
#
# Download/generate files required for build or runtime
#

cd $(dirname "$0")

YES=0
NO_CONAN_REMOTES=0
if [[ $1 = '-y' ]] ; then
    YES=1
fi
if [[ $1 = '--no-conan-remotes' ]] ; then
    NO_CONAN_REMOTES=1
fi

function ask {
    [[ $YES -eq 1 ]] && return 0
    while true ; do
        read -p "$1 [y/n/q] " -n 1 -r
        echo
        case $REPLY in
            [Yy]) return 0;;
            [Nn]) return 1;;
            [Qq]) exit 0;;
        esac
    done
}

function run { echo "$@" ; "$@"; }

if [[ ${NO_CONAN_REMOTES} -eq 0 ]] ; then
    echo "=== Check Conan remotes ==="
    conan_remotes=$(conan remote list)
    echo "${conan_remotes}"
    if ! echo "${conan_remotes}" | grep -q '^bincrafters:' && ask 'Add conan remote "bincrafters"?'; then
        run conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
    fi
    if ! echo "${conan_remotes}" | grep -q '^xcikit:' && ask 'Add conan remote "xcikit"?'; then
        run conan remote add xcikit https://api.bintray.com/conan/rbrich/xcikit
    fi
fi

# See: https://github.com/source-foundry/Hack
if [[ ! -e "share/fonts/Hack/Hack-Regular.ttf" ]] ; then
    echo "=== Download Hack font ==="
    HACK_VERSION="v3.003"
    HACK_ARCHIVE="Hack-${HACK_VERSION}-ttf.tar.gz"
    curl -LO "https://github.com/source-foundry/Hack/releases/download/${HACK_VERSION}/${HACK_ARCHIVE}"
    tar xf ${HACK_ARCHIVE} -C share/fonts
    mv share/fonts/ttf share/fonts/Hack
    rm ${HACK_ARCHIVE}
fi

# Needed for demo_vfs
if [[ ! -e "share.dar" ]] ; then
    echo "=== Create share.dar archive ==="
    (cd share; find shaders fonts -type f > file_list.txt)
    tools/pack_assets.py share.dar share/file_list.txt
    rm share/file_list.txt
fi
