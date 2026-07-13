#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EXPERIMENT="${1:?usage: run_experiment.sh EXPERIMENT [PROFILE]}"
PROFILE="${2:-}"
RISK="$(python3 -c 'import json,sys; c=json.load(open(sys.argv[1])); print(next(x["risk"] for x in c["experiments"] if x["id"] == sys.argv[2]))' "${ROOT}/tools/froxel_lab/experiments.json" "${EXPERIMENT}")"
if [[ -z "${PROFILE}" ]]; then
	PROFILE="$(python3 -c 'import json,sys; c=json.load(open(sys.argv[1])); print(next(x["profiles"][0] for x in c["experiments"] if x["id"] == sys.argv[2]))' "${ROOT}/tools/froxel_lab/experiments.json" "${EXPERIMENT}")"
fi
RUN_ID="${EXPERIMENT}-${PROFILE//-/}-$(date -u +%Y%m%dT%H%M%SZ)"

cd "${ROOT}"
if [[ "${RISK}" == "secondary-client" || "${RISK}" == "incident-equivalent" ]]; then
	sudo -v
	python3 tools/froxel_lab/froxel_lab.py enroll --role sacrificial
	export TRINITY_FROXEL_GPU_LAB=I_ACKNOWLEDGE_GPU_RESET_RISK
fi

prepare=(python3 tools/froxel_lab/froxel_lab.py prepare \
	--experiment "${EXPERIMENT}" --profile "${PROFILE}" --run-id "${RUN_ID}")
"${prepare[@]}"

python3 tools/froxel_lab/froxel_lab.py run --run-id "${RUN_ID}"

echo "Run ${RUN_ID} finished. Collect it before arming another experiment."
