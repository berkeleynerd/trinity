#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RUN_ID="${1:-latest}"

cd "${ROOT}"
sudo -v
python3 tools/froxel_lab/froxel_lab.py collect --run "${RUN_ID}"
python3 tools/froxel_lab/froxel_lab.py bundle --run "${RUN_ID}"

echo "Collection complete. Review the focused bundle before sharing it."
echo "The full sysdiagnose and symbols bundles are unredacted and sensitive."
