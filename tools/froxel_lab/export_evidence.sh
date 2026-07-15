#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RUN_ID="${1:-latest}"
DESTINATION="${2:?usage: export_evidence.sh [RUN_ID] DESTINATION}"

cd "${ROOT}"
python3 tools/froxel_lab/froxel_lab.py export \
	--run "${RUN_ID}" \
	--destination "${DESTINATION}"
