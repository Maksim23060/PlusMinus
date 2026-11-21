#!/bin/sh
# Double-click in macOS Finder to launch the source checkout.
cd "$(dirname "$0")" || exit 1
exec sh ./Start-PlusMinus.sh "$@"
