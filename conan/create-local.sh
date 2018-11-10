#!/bin/bash
cd "$(dirname "$0")"

conan create incbin xci/local
conan create abseil xci/local

# Cleanup
rm -rf ./incbin/test_package/build
rm -rf ./abseil/test_package/build
