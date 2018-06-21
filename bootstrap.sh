#!/usr/bin/env bash
#
# Regenerate dependencies
#

PROJECT_ROOT=$(dirname "$0")

echo "=== glad ==="
pip install --user glad
~/.local/bin/glad --profile="core" --api="gl=3.3" --generator="c" --spec="gl" \
    --no-loader --local-files --omit-khrplatform --extensions="GL_KHR_debug" \
    --out-path ${PROJECT_ROOT}/ext/glad/
