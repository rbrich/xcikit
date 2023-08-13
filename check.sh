#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

echo "=== ShellCheck ==="
if command -v shellcheck >/dev/null; then
    shellcheck ./*.sh
else
    echo "[skipped] shellcheck not found"
fi

echo "=== Done ==="
