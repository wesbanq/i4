#!/bin/sh
# Usage: gen_version.sh <template> <output>
# Replaces @@VERSION@@ in <template> with `git describe` (or "unknown" without git).
# Runs on every build; <output> is only touched when the version changed, so nothing
# is recompiled otherwise.
version=$(git describe --always --dirty 2>/dev/null)
mkdir -p "$(dirname "$2")" && sed "s|@@VERSION@@|${version:-unknown}|g" "$1" > "$2.tmp" || exit 1
if cmp -s "$2.tmp" "$2"; then rm -f "$2.tmp"; else mv -f "$2.tmp" "$2"; fi
