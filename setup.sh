#!/bin/bash
# One-time setup: fills in this repo's actual absolute path everywhere
# worlds/thermalRandy.sdf needs one (gz-sim plugin/save paths must be real
# filesystem paths). Safe to re-run; it only ever replaces the placeholder,
# so running it twice from the same location is a no-op.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sed -i "s|__REPO_ROOT__|${REPO_ROOT}|g" "${REPO_ROOT}/worlds/thermalRandy.sdf"

mkdir -p "${REPO_ROOT}/output/rgb" "${REPO_ROOT}/output/thermal" "${REPO_ROOT}/output/radiometric" "${REPO_ROOT}/output/depth"

echo "Configured worlds/thermalRandy.sdf for: ${REPO_ROOT}"
echo "Output will be written to: ${REPO_ROOT}/output/{rgb,thermal,radiometric,depth}"
echo "Next: build the plugin (see README), then run: gz sim worlds/thermalRandy.sdf"
