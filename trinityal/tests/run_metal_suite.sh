#!/bin/sh
# PL-05: build and run the TrinityAL GPU conformance suite on the Metal
# backend, collecting the ctest verdict and the per-case XML report into
# <build>/trinityal/tests/reports/ (evidence stays in the ignored build
# tree, per repo convention).
#
# Usage: run_metal_suite.sh [build-dir]
set -eu

here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
build="${1:-$repo/.cmake-build-arm64-osx-debug}"

if ! xcrun --find metal > /dev/null 2>&1; then
	echo "The Metal shader toolchain is unavailable (xcrun --find metal failed)." >&2
	echo "Install it via Xcode (Metal toolchain component) and retry." >&2
	exit 1
fi
if [ ! -f "$build/CMakeCache.txt" ]; then
	echo "No configured build at $build (configure with -DBUILD_TESTING=ON first)." >&2
	exit 1
fi

cmake --build "$build" --target TrinityALTest_metal

reports="$build/trinityal/tests/reports"
mkdir -p "$reports"

status=0
ctest --test-dir "$build" -R TrinityALTest_metal --output-on-failure || status=$?

summary="$reports/TrinityALTest_metal_summary.txt"
{
	echo "TrinityAL Metal conformance run: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
	echo "ctest exit status: $status"
	echo "toolchain: $(xcrun metal --version 2>/dev/null | head -1)"
	echo "report: $reports/TrinityALTest_metal.xml"
	if [ -f "$reports/TrinityALTest_metal.xml" ]; then
		python3 - "$reports/TrinityALTest_metal.xml" <<'PYEOF'
import sys, xml.etree.ElementTree as ET
root = ET.parse(sys.argv[1]).getroot()
print(f"cases: {root.get('tests')} failures: {root.get('failures')} "
      f"disabled: {root.get('disabled')} skipped: {root.get('skipped', '0')} "
      f"errors: {root.get('errors')} time: {root.get('time')}s")
for suite in root:
    print(f"  {suite.get('name')}: {suite.get('tests')} cases, "
          f"{suite.get('failures')} failures, {suite.get('skipped', '0')} skipped")
PYEOF
	fi
} | tee "$summary"

exit "$status"
