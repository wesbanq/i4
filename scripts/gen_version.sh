#!/bin/sh
# Usage: gen_version.sh <template> <output>
# Replaces @@VERSION@@ in <template> with `git describe` (or "unknown" without git).
version=$(git describe --always --dirty 2>/dev/null)
mkdir -p "$(dirname "$2")" && sed "s|@@VERSION@@|${version:-unknown}|g" "$1" > "$2"
