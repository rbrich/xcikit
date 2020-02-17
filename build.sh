#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ROOT_DIR="$PWD"
BUILD_TYPE=MinSizeRel
GENERATOR=
command -v ninja >/dev/null && GENERATOR="Ninja"
JOBS_ARGS=()
CMAKE_ARGS=()

print_usage()
{
    echo "Usage: ./build.sh [<phase>, ...] [-G <cmake_generator>] [-j <jobs>] [-D <cmake_def>, ...]"
    echo "Where: <phase> = clean | deps | config | build | test | install | package (default: deps..install)"
    echo "       <cmake_generator> = \"Unix Makefiles\" | Ninja | ... (default: Ninja if available, Unix Makefiles otherwise)"
}

phase()
{
    local PHASE="phase_$1"
    test -n "${!PHASE}" -o -n "${phase_all}" -o \( -n "${phase_default}" -a "$1" != "clean" -a "$1" != "package" \)
}

# parse args...
phase_default=yes
phase_all=
while [[ $# -gt 0 ]] ; do
    case "$1" in
        all|clean|deps|config|build|test|install|package )
            phase_default=
            declare "phase_$1=yes"
            shift 1 ;;
        -G )
            GENERATOR="$2"
            shift 2 ;;
        -j )
            JOBS_ARGS+=(-j "$2")
            shift 2 ;;
        -D )
            CMAKE_ARGS+=(-D "$2")
            shift 2 ;;
        -D* )
            CMAKE_ARGS+=("$1")
            shift 1 ;;
        * )
            printf "Error: Unsupported option: %s.\n\n" "$1"
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
[[ -n "${GENERATOR}" ]] && BUILD_CONFIG="${BUILD_CONFIG}_${GENERATOR}"
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"
PACKAGE_DIR="xcikit-${VERSION}"
PACKAGE_NAME="${PACKAGE_DIR}-${PLATFORM}-${ARCH}.zip"
[[ -n "${GENERATOR}" ]] && CMAKE_ARGS+=(-G "${GENERATOR}")

echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"
phase package && echo "PACKAGE_NAME: ${PACKAGE_NAME}"
echo

if phase clean; then
    echo "=== Clean Previous Build ==="
    rm -vrf "${BUILD_DIR}"
    rm -vrf "${INSTALL_DIR}"
fi

mkdir -p "${BUILD_DIR}"

if phase deps; then
    echo "=== Install Dependencies ==="
    CONAN_ARGS=()
    if [[ -n "${MACOSX_DEPLOYMENT_TARGET}" ]]; then
        CONAN_ARGS+=(-s "os.version=${MACOSX_DEPLOYMENT_TARGET}")
    fi
    (
        cd "${BUILD_DIR}"
        conan install "${ROOT_DIR}" \
            --build missing \
            -s "build_type=${BUILD_TYPE}" \
            "${CONAN_ARGS[@]}"
    )
    echo
fi

if phase config; then
    echo "=== Configure ==="
    (
        cd "${BUILD_DIR}"
        cmake "${ROOT_DIR}" \
            "${CMAKE_ARGS[@]}" \
            -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
            -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
    )
    echo
fi

if phase build; then
    echo "=== Build ==="
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
    echo
fi

if phase test; then
    echo "=== Test ==="
    ( cd "${BUILD_DIR}" && ctest --build-config "${BUILD_TYPE}" "${JOBS_ARGS[@]}" )
    echo
fi

if phase install; then
    echo "=== Install ==="
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}" --target install
    echo
fi

if phase package; then
    echo "=== Package ==="
    (
        cd "${INSTALL_DIR}/.."
        mv "${INSTALL_DIR}" "${PACKAGE_DIR}"
        rm -f "${PACKAGE_NAME}"
        cmake -E tar cfv "${PACKAGE_NAME}" --format=zip "${PACKAGE_DIR}"
        mv "${PACKAGE_DIR}" "${INSTALL_DIR}"
    )
    echo
fi

echo "=== Done ==="
