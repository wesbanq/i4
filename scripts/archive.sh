#!/bin/sh
# Usage: archive.sh <ar> <archive> <objects...>
# Uses <ar> if it's installed, otherwise the system's ar (macOS ships ar, not llvm-ar).
set -e

ar=$1
out=$2
shift 2

command -v "$ar" > /dev/null 2>&1 || ar=ar
"$ar" rcs "$out" "$@"
