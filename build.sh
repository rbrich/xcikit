#!/bin/bash
set -e
cd $(dirname $0)

ROOT_DIR="$PWD"
BUILD_TYPE=MinSizeRel
BUILD_CONFIG="$(uname)_$(uname -m)_${BUILD_TYPE}"
BUILD_DIR="${ROOT_DIR}/build/${BUILD_CONFIG}"
INSTALL_DIR="${ROOT_DIR}/artifacts/${BUILD_CONFIG}"

mkdir -p "${BUILD_DIR}"

echo "=== Settings ==="
echo "BUILD_DIR:    ${BUILD_DIR}"
echo "INSTALL_DIR:  ${INSTALL_DIR}"
echo

echo "=== Install Dependencies ==="
(cd "${BUILD_DIR}"; conan install "${ROOT_DIR}" \
    --build missing)
echo

echo "=== Configure ==="
(cd "${BUILD_DIR}"; cmake "${ROOT_DIR}" \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
)
echo

echo "=== Build ==="
cmake --build ${BUILD_DIR}
echo

echo "=== Test ==="
cmake --build ${BUILD_DIR} --target test
echo

echo "=== Install ==="
cmake --build ${BUILD_DIR} --target install
echo

echo "=== All Done ==="

