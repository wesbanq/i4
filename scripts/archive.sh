#!/bin/sh
# Usage: archive.sh <ar> <archive> <objects...>
# Uses <ar> if it's installed, otherwise the system's ar (macOS ships ar, not llvm-ar).
ar=$1
shift
command -v "$ar" > /dev/null 2>&1 || ar=ar
exec "$ar" rcs "$@"
