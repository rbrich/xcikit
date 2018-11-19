#!/bin/bash
cd "$(dirname "$0")"

conan create incbin rbrich/stable

# Cleanup
rm -rf ./incbin/test_package/build
