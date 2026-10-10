#!/bin/sh
# Usage: link.sh <cxx> <binary> <stamp> <inputs...>
# Links <binary>, adding ".exe" when the compiler targets Windows, then writes the
# real path to <stamp>. Ninja tracks the stamp because it can't know the name upfront.
set -e

cxx=$1
bin=$2
stamp=$3
shift 3

case $($cxx -dumpmachine) in
    *windows*|*mingw*|*cygwin*|*msys*) bin=$bin.exe ;;
esac

$cxx -o "$bin" "$@"
echo "$bin" > "$stamp"
