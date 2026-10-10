#!/bin/sh
# Usage: gen_version.sh <template> <output>
# Replaces @@VERSION@@ in <template> with `git describe` (or "unknown" without git).
set -e

version=$(git describe --always --dirty 2>/dev/null) || version=
[ -n "$version" ] || version=unknown

mkdir -p "$(dirname "$2")"
sed "s|@@VERSION@@|$version|g" "$1" > "$2"
