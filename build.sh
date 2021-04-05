#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ROOT_DIR="$PWD"
BUILD_TYPE="Release"
INSTALL_DEVEL=0
GENERATOR=
EMSCRIPTEN=0
JOBS_ARGS=()
CMAKE_ARGS=()
CONAN_ARGS=()
CSI=$'\x1b['

print_usage()
{
    echo "Usage: ./build.sh [PHASE, ...] [COMPONENT, ...] [-G CMAKE_GENERATOR] [-j JOBS] [-D CMAKE_DEF, ...]"
    echo "                  [--debug|--minsize] [--emscripten] [--unity] [--tidy] [--update]"
    echo "Where: PHASE = clean | deps | config | build | test | install | package | graphviz (default: deps..install)"
    echo "       COMPONENT = core | data | script | graphics | text | widgets | all (default: all)"
    echo "Other options:"
    echo "      -G CMAKE_GENERATOR      Unix Makefiles | Ninja | ... (default: Ninja if available, Unix Makefiles otherwise)"
    echo "      -j JOBS                 Parallel jobs for build and test phases"
    echo "      -D CMAKE_DEF            Passed to cmake"
    echo "      --debug, --minsize      Sets CMAKE_BUILD_TYPE"
    echo "      --devel                 Install/package headers, CMake config, static libs."
    echo "      --emscripten            Target Emscripten (i.e. wrap with 'emcmake')"
    echo "      --unity                 CMAKE_UNITY_BUILD - batch all source files in each target together"
    echo "      --tidy                  Run clang-tidy on each compiled file"
    echo "      --update                Passed to conan - update dependencies"
    echo "      --profile, -pr PROFILE  Passed to conan - use the profile"
    echo "      --build-dir             Build directory (default: ./build/<build-config>)"
    echo "      --install-dir           Installation directory (default: ./artifacts/<build-config>)"
    echo "      --toolchain FILE        CMAKE_TOOLCHAIN_FILE - select build toolchain"
}

phase()
{
    local PHASE="phase_$1"
    test -n "${!PHASE}" -o \( -n "${phase_default}" -a "$1" != "clean" -a "$1" != "package" -a "$1" != "graphviz" \)
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
component_default=yes
component_all=
while [[ $# -gt 0 ]] ; do
    case "$1" in
        clean | deps | config | build | test | install | package | graphviz )
            phase_default=
            declare "phase_$1=yes"
            shift 1 ;;
        core | data | script | graphics | text | widgets | all )
            component_default=
            declare "component_$1=yes"
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
        --debug )
            BUILD_TYPE="Debug"
            shift 1 ;;
        --minsize )
            BUILD_TYPE="MinSizeRel"
            shift 1 ;;
        --devel )
            INSTALL_DEVEL=1
            shift 1 ;;
        --emscripten )
            EMSCRIPTEN=1
            shift 1 ;;
        --unity )
            # Batch all source files in each target together. This is best to be
            # sure a unity build works. It might not be best for speed or memory
            # consumption, but seems not worse then smaller batches in my tests.
            CMAKE_ARGS+=(-D'CMAKE_UNITY_BUILD=1' -D'CMAKE_UNITY_BUILD_BATCH_SIZE=0')
            shift 1 ;;
        --tidy )
            CMAKE_ARGS+=(-D'ENABLE_TIDY=1')
            shift 1 ;;
        --update )
            CONAN_ARGS+=('--update')
            shift 1 ;;
        --build-dir )
            BUILD_DIR="$2"
            shift 2 ;;
        --install-dir )
            INSTALL_DIR="$2"
            shift 2 ;;
        -pr | --profile )
            CONAN_ARGS+=('--profile' "$2")
            shift 2 ;;
        --toolchain )
            CMAKE_ARGS+=(-D"CMAKE_TOOLCHAIN_FILE=$2")
            shift 2 ;;
        -h | --help )
            print_usage
            exit 0 ;;
        * )
            printf 'Error: Unknown option: %s\n\n' "$1"
            print_usage
            exit 1 ;;
    esac
done

header "Settings"

ARCH="$(uname -m)"
PLATFORM="$(uname)"
if [[ "${EMSCRIPTEN}" -eq 1 ]] ; then
    PLATFORM="emscripten"
    ARCH="wasm"
elif [[ ${PLATFORM} = "Darwin" ]] ; then
    PLATFORM="macos${MACOSX_DEPLOYMENT_TARGET}"
fi

VERSION=$(conan inspect . --raw version)$(git rev-parse --short HEAD 2>/dev/null | sed 's/^/+/' ; :)
BUILD_CONFIG="${PLATFORM}-${ARCH}-${BUILD_TYPE}"
[[ -z "${GENERATOR}" ]] && setup_ninja && GENERATOR="Ninja"
[[ -n "${GENERATOR}" ]] && BUILD_CONFIG="${BUILD_CONFIG}-${GENERATOR// }"
[[ -n "${GENERATOR}" ]] && CMAKE_ARGS+=(-G "${GENERATOR}")
BUILD_DIR=${BUILD_DIR:-"${ROOT_DIR}/build/${BUILD_CONFIG}"}
INSTALL_DIR=${INSTALL_DIR:-"${ROOT_DIR}/artifacts/${BUILD_CONFIG}"}
PACKAGE_OUTPUT_DIR="${ROOT_DIR}/artifacts"
PACKAGE_FILENAME="xcikit-${VERSION}-${PLATFORM}-${ARCH}"
[[ ${BUILD_TYPE} != "Release" ]] && PACKAGE_FILENAME="${PACKAGE_FILENAME}-${BUILD_TYPE}"

if [[ -z "$component_default" && -z "$component_all" ]]; then
    # Disable components that were not selected
    for name in data script graphics text widgets ; do
        component_var="component_$name"
        if [[ -z "${!component_var}" ]] ; then
            CMAKE_ARGS+=(-D "XCI_${name^^}=OFF")
            CONAN_ARGS+=(-o "xcikit:${name}=False")
        else
            CMAKE_ARGS+=(-D "XCI_${name^^}=ON")
            CONAN_ARGS+=(-o "xcikit:${name}=True")
        fi
    done
fi

CMAKE_ARGS+=(-D"XCI_INSTALL_DEVEL=${INSTALL_DEVEL}")

# Ninja: force compiler colors, if the output goes to terminal
if [[ -t 1 && "${GENERATOR}" = "Ninja" ]]; then
    CMAKE_ARGS+=(-D'FORCE_COLORS=1')
fi

echo "CMAKE_ARGS:   ${CMAKE_ARGS[*]}"
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
        WRAPPER=
        [[ "$EMSCRIPTEN" -eq 1 ]] && WRAPPER=emcmake
        cd "${BUILD_DIR}"
        XCI_CMAKE_COLORS=1 ${WRAPPER} cmake "${ROOT_DIR}" \
            "${CMAKE_ARGS[@]}" \
            -D"CMAKE_BUILD_TYPE=${BUILD_TYPE}" \
            -D"CMAKE_INSTALL_PREFIX=${INSTALL_DIR}" \
            -D'CMAKE_EXPORT_COMPILE_COMMANDS=ON'
    )
    echo
fi

# Enable colored Ninja status, if the output goes to terminal (only for build step)
if [[ -t 1 && "${GENERATOR}" = "Ninja" ]]; then
    export NINJA_STATUS="${CSI}1m[${CSI}32m%p ${CSI}0;32m%f${CSI}0m/${CSI}32m%t ${CSI}36m%es${CSI}0m ${CSI}1m]${CSI}0m "
fi

if phase build; then
    header "Build"
    WRAPPER=
    [[ "$EMSCRIPTEN" -eq 1 ]] && WRAPPER=emmake
    ${WRAPPER} cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
    [[ "${GENERATOR}" = "Ninja" ]] && ninja -C "${BUILD_DIR}" -t cleandead
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
        cd "${BUILD_DIR}" && \
        cpack -G "TGZ;ZIP" -D "CPACK_PACKAGE_FILE_NAME=${PACKAGE_FILENAME}" -C "${BUILD_TYPE}" && \
        mv -v "${BUILD_DIR}/${PACKAGE_FILENAME}"* "${PACKAGE_OUTPUT_DIR}"
    )
    echo
fi

if phase graphviz; then
    header "Graphviz"
    (
        cd "${BUILD_DIR}"
        cmake --graphviz=deps.dot "${ROOT_DIR}"
        dot -T svg -o deps.svg deps.dot
        command -v open >/dev/null && open deps.svg
    )
    echo
fi

header "Done"
