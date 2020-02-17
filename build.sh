#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ROOT_DIR="$PWD"
BUILD_TYPE=MinSizeRel
GENERATOR=
JOBS_ARGS=()
CMAKE_ARGS=()
CSI=$'\x1b['

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

setup_ninja()
{
    command -v ninja >/dev/null || return 1
    local NINJA_VERSION
    # e.g. "1.10.0
    NINJA_VERSION=$(ninja --version)
    # strip last part: "1.10"
    NINJA_VERSION=${NINJA_VERSION%.*}
    # test major == 1 && minor >= 10 (we require Ninja > 1.10)
    [[ "${NINJA_VERSION%%.*}" -eq "1" && "${NINJA_VERSION##*.}" -ge "10" ]] || \
        return 1
    return 0
}

header()
{
    if [[ -t 1 ]]; then
        echo "${CSI}1m=== ${1} ===${CSI}0m"
    else
        echo "=== ${1} ==="
    fi
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

header "Settings"

ARCH="$(uname -m)"
PLATFORM="$(uname)"
[[ ${PLATFORM} = "Darwin" ]] && PLATFORM="macos${MACOSX_DEPLOYMENT_TARGET}"
VERSION=$(conan inspect . --raw version)$(git rev-parse --short HEAD 2>/dev/null | sed 's/^/+/' ; :)
BUILD_CONFIG="${PLATFORM}-${ARCH}-${BUILD_TYPE}"
[[ -z "${GENERATOR}" ]] && setup_ninja && GENERATOR="Ninja"
[[ -n "${GENERATOR}" ]] && BUILD_CONFIG="${BUILD_CONFIG}_${GENERATOR}"
[[ -n "${GENERATOR}" ]] && CMAKE_ARGS+=(-G "${GENERATOR}")
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"
PACKAGE_DIR="xcikit-${VERSION}"
PACKAGE_NAME="${PACKAGE_DIR}-${PLATFORM}-${ARCH}.zip"

echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"
phase package && echo "PACKAGE_NAME: ${PACKAGE_NAME}"
echo

if phase clean; then
    header "Clean Previous Build"
    rm -vrf "${BUILD_DIR}"
    rm -vrf "${INSTALL_DIR}"
fi

mkdir -p "${BUILD_DIR}"

if phase deps; then
    header "Install Dependencies"
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
    header "Configure"
    (
        cd "${BUILD_DIR}"
        cmake "${ROOT_DIR}" \
            "${CMAKE_ARGS[@]}" \
            -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
            -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}"
    )
    echo
fi

# Ninja: enable colors, if the output goes to terminal (only for build step)
if [[ -t 1 && "${GENERATOR}" = "Ninja" ]]; then
    export NINJA_STATUS="${CSI}1m[${CSI}32m%p ${CSI}0;32m%f${CSI}0m/${CSI}32m%t ${CSI}36m%es${CSI}0m ${CSI}1m]${CSI}0m "
    CMAKE_ARGS+=(-D FORCE_COLORS=1)
fi

if phase build; then
    header "Build"
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
    echo
fi

[[ "${GENERATOR}" = "Ninja" ]] && export -n NINJA_STATUS

if phase test; then
    header "Test"
    ( cd "${BUILD_DIR}" && ctest --build-config "${BUILD_TYPE}" "${JOBS_ARGS[@]}" )
    echo
fi

if phase install; then
    header "Install"
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}" --target install
    echo
fi

if phase package; then
    header "Package"
    (
        cd "${INSTALL_DIR}/.."
        mv "${INSTALL_DIR}" "${PACKAGE_DIR}"
        rm -f "${PACKAGE_NAME}"
        echo "${PACKAGE_NAME}"
        cmake -E tar cf "${PACKAGE_NAME}" --format=zip "${PACKAGE_DIR}"
        mv "${PACKAGE_DIR}" "${INSTALL_DIR}"
    )
    echo
fi

header "Done"
