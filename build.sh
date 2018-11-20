#!/bin/bash
set -e
cd $(dirname $0)

ROOT_DIR="$PWD"
BUILD_TYPE=MinSizeRel
BUILD_CONFIG="$(uname)_$(uname -m)_${BUILD_TYPE}"
BUILD_DIR="build/${BUILD_CONFIG}"
INSTALL_DIR="$PWD/artifacts/${BUILD_CONFIG}"

mkdir -p "${BUILD_DIR}"

(cd "${BUILD_DIR}"; conan install "${ROOT_DIR}" \
    --build glad)

(cd "${BUILD_DIR}"; cmake "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
)

cmake --build ${BUILD_DIR}
cmake --build ${BUILD_DIR} --target test
cmake --build ${BUILD_DIR} --target install
