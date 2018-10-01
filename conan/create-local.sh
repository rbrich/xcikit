#!/bin/bash
cd "$(dirname "$0")"

conan create PEGTL xci/local
conan create incbin xci/local

# Cleanup
rm -rf ./incbin/test_package/build
