#!/usr/bin/env bash

set -e
cd $(dirname $0)
ROOT_DIR="$PWD"
BUILD_TYPE=MinSizeRel
GENERATOR="Unix Makefiles"
which ninja >/dev/null && GENERATOR="Ninja"
JOBS=
CMAKE_ARGS=()

print_usage()
{
    echo "Usage: ./build.sh [<phase>, ...] [-G <cmake_generator>] [-j <jobs>] [-D <cmake_def>, ...]"
    echo "Where: <phase> = deps | config | build | test | install | package (default: deps..install)"
    echo "       <cmake_generator> = \"Unix Makefiles\" | Ninja | ... (default: Ninja if available, Unix Makefiles otherwise)"
}

phase()
{
    local PHASE="phase_$1"
    test -n "${!PHASE}" -o -n "${phase_all}" -o \( -n "${phase_default}" -a "$1" != "package" \)
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
        -j )
            JOBS="-j $2"
            shift 2 ;;
        -D )
            CMAKE_ARGS+=(-D "$2")
            shift 2 ;;
        * )
            printf "Error: Unsupported option ${1}.\n\n"
            print_usage
            exit 1 ;;
    esac
done

echo "=== Settings ==="

ARCH="$(uname -m)"
PLATFORM="$(uname)"
[[ ${PLATFORM} = "Darwin" ]] && PLATFORM="macos${MACOSX_DEPLOYMENT_TARGET}"
VERSION=$(conan inspect . --raw version)$(git rev-parse --short HEAD 2>/dev/null | sed 's/^/+/' ; :)
BUILD_CONFIG="${PLATFORM}-${ARCH}-${BUILD_TYPE}"
[[ "${GENERATOR}" != "Unix Makefiles" ]] && BUILD_CONFIG="${BUILD_CONFIG}_${GENERATOR}"
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"
PACKAGE_DIR="xcikit-${VERSION}"
PACKAGE_NAME="${PACKAGE_DIR}-${PLATFORM}-${ARCH}.zip"

echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"
phase package && echo "PACKAGE_NAME: ${PACKAGE_NAME}"
echo

mkdir -p "${BUILD_DIR}"

if phase deps; then
    echo "=== Install Dependencies ==="
    CONAN_SETTINGS=
    if [[ -n "${MACOSX_DEPLOYMENT_TARGET}" ]]; then
        CONAN_SETTINGS="-s os.version=${MACOSX_DEPLOYMENT_TARGET}"
    fi
    (
        cd "${BUILD_DIR}"
        conan install "${ROOT_DIR}" ${CONAN_SETTINGS} --build missing
    )
    echo
fi

if phase config; then
    echo "=== Configure ==="
    (
        cd "${BUILD_DIR}"
        cmake "${ROOT_DIR}" \
            -G "${GENERATOR}" \
            -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
            -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
            "${CMAKE_ARGS[@]}"
    )
    echo
fi

if phase build; then
    echo "=== Build ==="
    cmake --build ${BUILD_DIR} -- ${JOBS}
    echo
fi

if phase test; then
    echo "=== Test ==="
    cmake --build ${BUILD_DIR} --target test -- ${JOBS}
    echo
fi

if phase install; then
    echo "=== Install ==="
    cmake --build ${BUILD_DIR} --target install -- ${JOBS}
    echo
fi

if phase package; then
    echo "=== Package ==="
    (
        cd "${INSTALL_DIR}/.."
        mv "${INSTALL_DIR}" "${PACKAGE_DIR}"
        rm -f "${PACKAGE_NAME}"
        zip --move -r "${PACKAGE_NAME}" ${PACKAGE_DIR}
    )
    echo
fi

echo "=== Done ==="
