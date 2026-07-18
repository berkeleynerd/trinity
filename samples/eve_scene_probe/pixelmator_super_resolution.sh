#!/bin/zsh
# ML super resolution via Pixelmator Pro's AppleScript interface.
#
# Usage: pixelmator_super_resolution.sh INPUT.png OUTPUT.png SIZE
#
# Fully local: the image never leaves the machine. Requires Pixelmator Pro
# (either bundle name); fails with a clear message when it is absent. Part
# of the Talocan modernization lane's optional upscale stage — see
# docs/talocan-modernization.md.

set -euo pipefail

if (( $# != 3 )); then
	print -u2 "usage: $0 INPUT.png OUTPUT.png SIZE"
	exit 2
fi

input="$1"
output="$2"
size="$3"

if [[ ! -f "$input" ]]; then
	print -u2 "input does not exist: $input"
	exit 1
fi

app_name=""
for candidate in "Pixelmator Pro Creator Studio" "Pixelmator Pro"; do
	if [[ -d "/Applications/$candidate.app" ]]; then
		app_name="$candidate"
		break
	fi
done
if [[ -z "$app_name" ]]; then
	print -u2 "Pixelmator Pro is not installed; the upscale stage needs its ML Super Resolution."
	print -u2 "Install Pixelmator Pro or run the bootstrap without --upscale."
	exit 1
fi

# The application name must be a literal at AppleScript compile time —
# app-specific terminology (resize image, ml super resolution) cannot
# compile against a variable name — so the detected name is interpolated
# into the script by the shell before osascript sees it.
osascript - "$input" "$output" "$size" <<EOF
on run argv
	set inPath to POSIX file (item 1 of argv)
	set outPath to POSIX file (item 2 of argv)
	set targetSize to (item 3 of argv) as integer
	tell application "$app_name"
		set doc to open inPath
		resize image doc width targetSize height targetSize algorithm ml super resolution
		export doc to outPath as PNG
		close doc saving no
	end tell
end run
EOF

if [[ ! -f "$output" ]]; then
	print -u2 "Pixelmator export produced no output: $output"
	exit 1
fi
