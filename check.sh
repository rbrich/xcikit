#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

echo "=== ShellCheck ==="
if command -v shellcheck >/dev/null; then
    find . -type f -name '*.sh' -print0 | xargs -0 shellcheck
else
    echo "[skipped] shellcheck not found"
fi

echo "=== Done ==="
