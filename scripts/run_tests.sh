#!/bin/sh
# Usage: scripts/run_tests.sh [i4 binary]
# Runs every tests/*.i4 twice, in memory (-m) and on disk, and compares stdout+stderr plus the
# exit code with tests/<name>.expected. The runs happen in a throwaway copy of tests/, so the
# programs here are never consumed.
# Optional per test: tests/<name>.in is fed to stdin, tests/<name>.args holds one program arg per
# line, tests/<name>.flags holds extra interpreter flags.
# Set UPDATE=1 to rewrite the .expected files from the current output instead of comparing.
cd "$(dirname "$0")/.." || exit 1
bin=${1:-$(cat build/i4.stamp 2>/dev/null)}
[ -x "$bin" ] || { echo "No i4 binary found; run ninja first or pass its path." >&2; exit 1; }
case $bin in /*) ;; *) bin=$PWD/$bin ;; esac

root=$PWD
work=$(mktemp -d) || exit 1
trap 'rm -rf "$work"' EXIT
cp -R tests "$work/tests" && cd "$work" || exit 1

# Usage: run <mode flag, or nothing for a run on disk> <program args...>
run() {
    mode=$1
    shift
    # $mode and $flags are split into words on purpose.
    "$bin" $mode $flags "$test" -- "$@" < "$input" 2>&1
    echo "[exit $?]"
}

passed=0 failed=0
for test in tests/*.i4; do
    name=${test%.i4}
    input=/dev/null flags=
    [ -f "$name.in" ] && input=$name.in
    [ -f "$name.flags" ] && flags=$(cat "$name.flags")
    set --
    [ -f "$name.args" ] && while IFS= read -r arg || [ -n "$arg" ]; do set -- "$@" "$arg"; done < "$name.args"

    if [ "${UPDATE:-0}" = 1 ]; then
        run -m "$@" > "$root/$name.expected"
        continue
    fi
    expected=$(cat "$name.expected" 2>/dev/null)
    # In memory first: the run on disk consumes the copy.
    for mode in -m ""; do
        actual=$(run "$mode" "$@")
        if [ "$actual" = "$expected" ]; then
            passed=$((passed + 1))
        else
            failed=$((failed + 1))
            echo "FAIL $test (${mode:-on disk})"
            printf '%s\n' "$actual" | diff "$name.expected" - | sed 's/^/    /'
        fi
    done
done
[ "${UPDATE:-0}" = 1 ] && { echo "Updated expected output."; exit 0; }
echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
