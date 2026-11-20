#!/usr/bin/env bash
# srchash.sh [rev]  - hash of the firmware-relevant source tree at <rev> (default HEAD).
# Docs, tests, workflows and tools are excluded on purpose: changing them does not change firmware.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
. "$here/baseline.env"
rev="${1:-HEAD}"
git ls-tree -r --full-tree "$rev" -- $SRC_PATHS | sha1sum | cut -c1-12
