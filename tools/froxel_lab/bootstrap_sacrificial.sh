#!/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TRINITY_FROXEL_BUILD_DIR:-${ROOT}/.cmake-build-arm64-osx-debug}"
MANIFEST="${BUILD_DIR}/samples/eve_scene_probe/Reports/VolumetricResources.json"

[[ "$(uname -s)" == "Darwin" ]] || { echo "macOS is required" >&2; exit 1; }
for tool in cmake ninja python3 git xcrun xcodebuild; do
	command -v "${tool}" >/dev/null || { echo "Missing required tool: ${tool}" >&2; exit 1; }
done

free_kb="$(df -Pk "${ROOT}" | awk 'NR == 2 { print $4 }')"
(( free_kb >= 75 * 1024 * 1024 )) || { echo "At least 75 GiB free is required" >&2; exit 1; }

cd "${ROOT}"
export PATH_TO_VCPKG_ROOT="${ROOT}/vendor/github.com/microsoft/vcpkg"
export X_VCPKG_REGISTRIES_CACHE="${ROOT}/regcache"
export VCPKG_BINARY_SOURCES=clear
mkdir -p "${X_VCPKG_REGISTRIES_CACHE}"
python3 tools/froxel_lab/froxel_lab.py inventory \
	--output "${HOME}/Library/Logs/TrinityFroxelLab/bootstrap-inventory.json"

CMAKE_GENERATOR="Ninja Multi-Config" cmake --preset arm64-osx-debug \
	-DBUILD_METAL=ON \
	-DBUILD_SHADER_COMPILER=ON \
	-DBUILD_TESTING=OFF \
	-DWITH_GRANNY=OFF \
	-DBUILD_EVE_SCENE_PROBE=ON \
	-DBUILD_FROXEL_GPU_LAB=ON

cmake --build "${BUILD_DIR}" --config Debug \
	--target TrinityALFroxelProbe_metal TrinityALEveSceneProbeFroxelLab_metal

froxel_probe="${BUILD_DIR}/tools/froxel_lab/Debug/TrinityALFroxelProbe_metal"
[[ -x "${froxel_probe}" ]] || froxel_probe="${froxel_probe}_debug"
[[ -x "${froxel_probe}" ]] || { echo "Missing froxel probe executable" >&2; exit 1; }
"${froxel_probe}" --self-test

[[ -f "${MANIFEST}" ]] || { echo "Missing volumetric manifest: ${MANIFEST}" >&2; exit 1; }
python3 -m json.tool "${MANIFEST}" >/dev/null
PYTHONPATH="${ROOT}/shadercompiler/python" \
	python3 -m unittest shadercompiler.test.test_effectinfo
python3 tools/froxel_lab/test_froxel_lab.py
python3 tools/froxel_lab/froxel_lab.py audit \
	--effect-root "${BUILD_DIR}/samples/eve_scene_probe/Assets/graphics/effect.metal/managed/space/specialfx/volumetric/fog" \
	--report "${HOME}/Library/Logs/TrinityFroxelLab/bootstrap-air-audit.json"

echo "Sacrificial host bootstrap complete. No client kernel was executed."
