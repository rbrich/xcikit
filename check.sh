#!/usr/bin/env bash

set -e
cd "$(dirname "$0")"

echo "=== ShellCheck ==="
command -v shellcheck >/dev/null \
&& find . -type f -name '*.sh' -exec shellcheck '{}' ';' \
|| echo "[skipped] shellcheck not found"

echo "=== Done ==="
