#!/bin/sh
# Usage: scripts/run_tests.sh [i4 binary]
# Runs every tests/*.i4 in memory mode (-m), so test files are never consumed, and compares
# stdout+stderr plus the exit code with tests/<name>.expected.
# Optional per test: tests/<name>.in is fed to stdin, tests/<name>.args holds one program arg per line.
# Set UPDATE=1 to rewrite the .expected files from the current output instead of comparing.
cd "$(dirname "$0")/.." || exit 1
bin=${1:-$(cat build/i4.stamp 2>/dev/null)}
[ -x "$bin" ] || { echo "No i4 binary found; run ninja first or pass its path." >&2; exit 1; }

passed=0 failed=0
for test in tests/*.i4; do
    name=${test%.i4}
    input=/dev/null
    [ -f "$name.in" ] && input=$name.in
    set --
    [ -f "$name.args" ] && while IFS= read -r arg || [ -n "$arg" ]; do set -- "$@" "$arg"; done < "$name.args"
    actual=$("$bin" -m "$test" -- "$@" < "$input" 2>&1; echo "[exit $?]")

    if [ "${UPDATE:-0}" = 1 ]; then
        printf '%s\n' "$actual" > "$name.expected"
    elif [ "$actual" = "$(cat "$name.expected" 2>/dev/null)" ]; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
        echo "FAIL $test"
        printf '%s\n' "$actual" | diff "$name.expected" - | sed 's/^/    /'
    fi
done
[ "${UPDATE:-0}" = 1 ] && { echo "Updated expected output."; exit 0; }
echo "$passed passed, $failed failed"
[ "$failed" -eq 0 ]
