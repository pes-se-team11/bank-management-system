#!/usr/bin/env bash
set -euo pipefail

pattern='\b(gets|strcpy|strcat|sprintf|vsprintf|scanf|fscanf|sscanf|vscanf|vfscanf|vsscanf)[[:space:]]*\('

if matches=$(grep -rnE \
    --include='*.cpp' \
    --include='*.h' \
    --include='*.hpp' \
    "$pattern" src/); then

    printf '%s\n' "$matches"
    echo "Disallowed function found. Use std::string and checked parsing." >&2
    exit 1
else
    status=$?

    if [ "$status" -ne 1 ]; then
        echo "Source scan failed." >&2
        exit "$status"
    fi
fi

echo "No disallowed functions found."
