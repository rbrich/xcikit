#!/usr/bin/env bash

set -e
cd $(dirname $0)
ROOT_DIR="$PWD"
BUILD_TYPE=MinSizeRel
GENERATOR="Unix Makefiles"

print_usage()
{
    echo "Usage: ./build.sh [<phase>, ...] [-G <cmake_generator>]"
    echo "Where: <phase> = deps | config | build | test | install | package (default: deps..install)"
    echo "       <cmake_generator> = \"Unix Makefiles\" | Ninja | ..."
}

phase()
{
    local PHASE="phase_$1"
    test -n "${!PHASE}" -o -n "${phase_default}" -a "$1" != "package"
}

# parse args...
phase_default=yes
while [[ $# > 0 ]] ; do
    case "$1" in
        all|deps|config|build|test|install|package )
            phase_default=
            declare "phase_$1=yes"
            shift 1 ;;
        -G )
            GENERATOR="$2"
            shift 2 ;;
        * )
            printf "Error: Unsupported option ${1}.\n\n"
            print_usage
            exit 1 ;;
    esac
done

echo "=== Settings ==="

PLATFORM="$(uname)-$(uname -m)"
VERSION="snapshot"
BUILD_CONFIG="${PLATFORM}-${BUILD_TYPE}"
[[ "${GENERATOR}" != "Unix Makefiles" ]] && BUILD_CONFIG="${BUILD_CONFIG}_${GENERATOR}"
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"
PACKAGE_NAME="xcikit-${VERSION}-${PLATFORM}"

echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"
phase package && echo "PACKAGE_NAME: ${PACKAGE_NAME}"
echo

mkdir -p "${BUILD_DIR}"

if phase deps; then
    echo "=== Install Dependencies ==="
    (cd "${BUILD_DIR}"; conan install "${ROOT_DIR}" --build missing)
    echo
fi

if phase config; then
    echo "=== Configure ==="
    (cd "${BUILD_DIR}"; cmake "${ROOT_DIR}" \
        -G "${GENERATOR}" \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
    )
    echo
fi

if phase build; then
    echo "=== Build ==="
    cmake --build ${BUILD_DIR}
    echo
fi

if phase test; then
    echo "=== Test ==="
    cmake --build ${BUILD_DIR} --target test
    echo
fi

if phase install; then
    echo "=== Install ==="
    cmake --build ${BUILD_DIR} --target install
    echo
fi

if phase package; then
    echo "=== Package ==="
    (cd "${INSTALL_DIR}/.."; mv "${INSTALL_DIR}" "${PACKAGE_NAME}"; \
     zip --move -r "${PACKAGE_NAME}.zip" ${PACKAGE_NAME} \
    )
    echo
fi

echo "=== Done ==="
