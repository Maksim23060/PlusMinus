#!/bin/sh
# One-click source launcher for Linux and macOS.
set -u
project_dir=$(CDPATH= cd "$(dirname "$0")" && pwd -P) || exit 1
cd "$project_dir" || exit 1

if command -v python3 >/dev/null 2>&1; then
    python3 "$project_dir/launch.py" "$@"
    status=$?
elif command -v python >/dev/null 2>&1; then
    python "$project_dir/launch.py" "$@"
    status=$?
else
    printf '%s\n' 'PlusMinus requires Python 3.9+ to launch from source.' \
        'For installation-free startup, use a prebuilt PlusMinusStudio release.' >&2
    status=127
fi

if [ "$status" -ne 0 ] && [ -t 0 ]; then
    printf '\nPlusMinus could not start (exit code %s). Press Enter to close. ' "$status"
    read -r _answer || true
fi
exit "$status"
