#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

ROOT_DIR="$PWD"
BUILD_TYPE="Release"
INSTALL_DEVEL=0
GENERATOR=
EMSCRIPTEN=0
PRECACHE_DEPS=0
PRECACHE_ARGS=()
JOBS_ARGS=()
CMAKE_ARGS=()
CONAN_ARGS=()
CONAN_PROFILE=${CONAN_DEFAULT_PROFILE_PATH:-default}
DETECT_ARGS=()
CSI=$'\x1b['

run() { echo "➤➤➤ $*"; "$@"; }

print_usage()
{
    echo "Usage: ./build.sh [PHASE, ...] [COMPONENT, ...] [PART, ...] [-G CMAKE_GENERATOR] [-j JOBS] [-D CMAKE_DEF, ...]"
    echo "                  [--debug|--minsize] [--emscripten] [--unity] [--tidy] [--update]"
    echo "Where: PHASE = clean | deps | config | build | test | install | package | graphviz (default: deps..install)"
    echo "       COMPONENT = core | data | script | graphics | text | widgets (default: all)"
    echo "       PART = libs | tools | examples | tests | benchmarks (default: all)"
    echo "Other options:"
    echo "      -G CMAKE_GENERATOR      Unix Makefiles | Ninja | ... (default: Ninja if available, Unix Makefiles otherwise)"
    echo "      -j JOBS                 Parallel jobs for build and test phases"
    echo "      -D CMAKE_DEF            Passed to cmake"
    echo "      --debug, --minsize      Sets CMAKE_BUILD_TYPE"
    echo "      --devel                 Install/package headers, CMake config, static libs."
    echo "      --emscripten            Target Emscripten (i.e. wrap with 'emcmake')"
    echo "      --unity                 CMAKE_UNITY_BUILD - batch all source files in each target together"
    echo "      --asan, --lsan, --ubsan, --tsan"
    echo "                              Build with sanitizers (Address, Leak, UB, Thread)"
    echo "      --tidy                  Run clang-tidy on each compiled file"
    echo "      --update                Passed to conan - update dependencies"
    echo "      --profile, -pr PROFILE  Passed to conan - use the profile"
    echo "      --precache-deps         Call precache_upstream_deps.py, useful for CI"
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
    echo -n "Found ninja ${NINJA_VERSION}"
    # strip last part: "1.10"
    NINJA_VERSION=$(echo "${NINJA_VERSION}" | cut -d. -f'1,2')
    # test major == 1 && minor >= 10 (we require Ninja > 1.10)
    if [[ "${NINJA_VERSION%%.*}" -eq "1" && "${NINJA_VERSION##*.}" -ge "10" ]]; then
        echo " OK"
        return 0
    else
        echo " NOT COMPATIBLE"
        return 1
    fi
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
part_default=yes
while [[ $# -gt 0 ]] ; do
    case "$1" in
        clean | deps | config | build | test | install | package | graphviz )
            phase_default=
            declare "phase_$1=yes"
            shift 1 ;;
        core | data | script | graphics | text | widgets )
            component_default=
            declare "component_$1=yes"
            shift 1 ;;
        libs | tools | examples | tests | benchmarks )
            part_default=
            declare "part_$1=yes"
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
        --asan )
            CMAKE_ARGS+=(-D'BUILD_WITH_ASAN=1')
            shift 1 ;;
        --lsan )
            CMAKE_ARGS+=(-D'BUILD_WITH_LSAN=1')
            shift 1 ;;
        --ubsan )
            CMAKE_ARGS+=(-D'BUILD_WITH_UBSAN=1')
            shift 1 ;;
        --tsan )
            CMAKE_ARGS+=(-D'BUILD_WITH_TSAN=1')
            shift 1 ;;
        --tidy )
            CMAKE_ARGS+=(-D'ENABLE_TIDY=1')
            shift 1 ;;
        --update )
            CONAN_ARGS+=('--update')
            shift 1 ;;
        --precache-deps )
            PRECACHE_DEPS=1
            shift 1 ;;
        --build-dir )
            BUILD_DIR="$2"
            shift 2 ;;
        --install-dir )
            INSTALL_DIR="$2"
            shift 2 ;;
        -pr | --profile )
            CONAN_PROFILE="$2"
            shift 2 ;;
        --toolchain )
            CMAKE_ARGS+=(-D"CMAKE_TOOLCHAIN_FILE=$2")
            PRECACHE_ARGS+=("--toolchain" "$2")
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

CONAN_ARGS+=("-pr=$CONAN_PROFILE" "-pr:b=$CONAN_PROFILE")

COMPONENTS=(data script graphics text widgets)
if [[ -z "$component_default" ]]; then
    # Disable components that were not selected
    for name in "${COMPONENTS[@]}" ; do
        component_var="component_$name"
        name_upper="$(echo "$name" | tr '[:lower:]' '[:upper:]')"
        if [[ -z "${!component_var}" ]] ; then
            CMAKE_ARGS+=(-D "XCI_${name_upper}=OFF")
            CONAN_ARGS+=(-o "xcikit:${name}=False")
        else
            CMAKE_ARGS+=(-D "XCI_${name_upper}=ON")
            CONAN_ARGS+=(-o "xcikit:${name}=True")
            DETECT_ARGS+=("${name}")
        fi
    done
else
    DETECT_ARGS+=("${COMPONENTS[@]}")
fi

PARTS=(tools examples tests benchmarks)
if [[ -z "$part_default" ]]; then
    # Disable parts that were not selected
    for name in "${PARTS[@]}" ; do
        part_var="part_$name"
        name_upper="$(echo "$name" | tr '[:lower:]' '[:upper:]')"
        if [[ -z "${!part_var}" ]] ; then
            CMAKE_ARGS+=(-D "BUILD_${name_upper}=OFF")
            CONAN_ARGS+=(-o "xcikit:${name}=False")
        else
            CMAKE_ARGS+=(-D "BUILD_${name_upper}=ON")
            CONAN_ARGS+=(-o "xcikit:${name}=True")
            DETECT_ARGS+=("${name}")
        fi
    done
else
    DETECT_ARGS+=("${PARTS[@]}")
    for name in "${PARTS[@]}" ; do
        CONAN_ARGS+=(-o "xcikit:${name}=True")
    done
fi

CMAKE_ARGS+=(-D"XCI_INSTALL_DEVEL=${INSTALL_DEVEL}")

# Ninja: force compiler colors, if the output goes to terminal
if [[ -t 1 && "${GENERATOR}" = "Ninja" ]]; then
    CMAKE_ARGS+=(-D'FORCE_COLORS=1')
fi

if [[ -z "$PYTHON" ]] ; then
    for name in py python3 ; do
        if command -v $name >/dev/null ; then PYTHON=$name; break; fi
    done
fi

header "Settings"
echo "CONAN_ARGS:   ${CONAN_ARGS[*]}"
echo "CMAKE_ARGS:   ${CMAKE_ARGS[*]}"
echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"
phase package && echo "PACKAGE_NAME: ${PACKAGE_NAME}"
echo "PYTHON:       ${PYTHON}"
echo

if phase clean; then
    header "Clean Previous Build"
    rm -vrf "${BUILD_DIR}"
    rm -vrf "${INSTALL_DIR}"
fi

mkdir -p "${BUILD_DIR}"

if phase deps; then
    header "Install Dependencies"
    if [[ "${PLATFORM}" == macos* && -n "${MACOSX_DEPLOYMENT_TARGET}" ]]; then
        CONAN_ARGS+=(-s "os.version=${MACOSX_DEPLOYMENT_TARGET}")
    fi
    if [[ "${PRECACHE_DEPS}" -eq 1 ]]; then
        "${PYTHON}" "${ROOT_DIR}/precache_upstream_deps.py" "${PRECACHE_ARGS[@]}"
    fi
    (
        run cd "${BUILD_DIR}"

        if [[ ! -f 'system_deps.txt' ]] ; then
            echo 'Checking for preinstalled dependencies...'
            "${PYTHON}" "${ROOT_DIR}/detect_system_deps.py" "${DETECT_ARGS[@]}" | tee 'system_deps.txt'
        fi
        # shellcheck disable=SC2207
        CONAN_ARGS+=($(tail -n1 'system_deps.txt'))

        run conan install "${ROOT_DIR}" \
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
        run cd "${BUILD_DIR}"
        # shellcheck disable=SC2207
        CMAKE_ARGS+=($(tail -n2 'system_deps.txt' | head -n1))
        XCI_CMAKE_COLORS=1 run ${WRAPPER} cmake "${ROOT_DIR}" \
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
    run ${WRAPPER} cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
    [[ "${GENERATOR}" = "Ninja" ]] && ninja -C "${BUILD_DIR}" -t cleandead
    echo
fi

[[ "${GENERATOR}" = "Ninja" ]] && export -n NINJA_STATUS

if phase test; then
    header "Test"
    (
        run cd "${BUILD_DIR}" && \
        run ctest --progress --output-on-failure --build-config "${BUILD_TYPE}" "${JOBS_ARGS[@]}"
    )
    echo
fi

if phase install; then
    header "Install"
    run cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" "${JOBS_ARGS[@]}" --target install
    echo
fi

if phase package; then
    header "Package"
    (
        run cd "${BUILD_DIR}" && \
        run cpack -G "TGZ;ZIP" -D "CPACK_PACKAGE_FILE_NAME=${PACKAGE_FILENAME}" -C "${BUILD_TYPE}" && \
        run mv -v "${BUILD_DIR}/${PACKAGE_FILENAME}"* "${PACKAGE_OUTPUT_DIR}"
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
