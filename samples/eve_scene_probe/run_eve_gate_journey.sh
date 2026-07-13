#!/bin/zsh

set -euo pipefail

script_dir="${0:A:h}"
bundle_binary="$script_dir/TrinityEveGateJourney.bin"

# When copied into the app bundle, this script is the bundle executable.
if [[ -x "$bundle_binary" ]]; then
	log_root="$HOME/Library/Logs/TrinityEveGateJourney"
	run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
	run_dir="$log_root/$run_id"
	mkdir -p "$run_dir"
	ln -sfn "$run_dir" "$log_root/latest"

	demo_args=(
		--quality-rung hdr-finish
		--material-mode eve-v5
		--scene-fixture new-eden
		--lighting-view combined
		--sh-source new-eden-celestials
		--local-lights authored
		--local-shadows authored
		--attachments authored
		--decals authored
		--engines authored
		--reflection-source dynamic
		--reflection-correction client
		--shadows high
		--ao high
		--ao-method cortao
		--composition system
		--planet-layers all
		--sun-effects all
		--dynamic-exposure client
		--bloom client
		--film-grain client
		--distortion authored
		--taa high
		--motion static
		--ballpark warp
		--warp-target evegate
		--ballpark-frame chase
		--celestial-ballpark natural
		--eve-gate authored
		--warp-tunnel authored
		--celestial-anchor stargate
		--volumetrics all
		--volumetric-quality high
		--enable-froxels
		--ballpark-log "$run_dir/ballpark.csv"
	)

	extra_args=()
	for arg in "$@"; do
		[[ "$arg" == -psn_* ]] || extra_args+=( "$arg" )
	done

	exec "$bundle_binary" "${demo_args[@]}" "${extra_args[@]}" >"$run_dir/runtime.log" 2>&1
fi

usage() {
	cat <<'EOF'
Usage: run_eve_gate_journey.sh [--binary PATH] [--install-only]

Installs and launches the canonical fullscreen EVE Gate round-trip demo as a
standalone macOS application. Set TRINITY_EVE_SCENE_PROBE to override binary
discovery or TRINITY_EVE_GATE_APP to override the installed app path.
EOF
}

binary_override="${TRINITY_EVE_SCENE_PROBE:-}"
install_only=false
while (( $# > 0 )); do
	case "$1" in
		--binary)
			(( $# >= 2 )) || { usage >&2; exit 2; }
			binary_override="$2"
			shift 2
			;;
		--install-only)
			install_only=true
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			usage >&2
			exit 2
			;;
	esac
done

if [[ "$(uname -s)" != Darwin ]]; then
	print -u2 "The canonical EVE Gate journey app requires macOS."
	exit 1
fi

repo_root="${script_dir:h:h}"
probe_name="TrinityALEveSceneProbe_metal_debug"
candidates=(
	"$binary_override"
	"$repo_root/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/Debug/$probe_name"
	"$repo_root/.cmake-build-arm64-osx-debug/samples/eve_scene_probe/$probe_name"
	"${repo_root:h}/promised-land-warp-demo/.cmake-build-arm64-osx-debug/pl10/trinity-build/samples/eve_scene_probe/Debug/$probe_name"
	"${repo_root:h}/promised-land/.cmake-build-arm64-osx-debug/pl10/trinity-build/samples/eve_scene_probe/Debug/$probe_name"
)

probe_binary=""
for candidate in "${candidates[@]}"; do
	if [[ -n "$candidate" && -x "$candidate" ]]; then
		probe_binary="${candidate:A}"
		break
	fi
done

if [[ -z "$probe_binary" ]]; then
	print -u2 "Could not find $probe_name. Set TRINITY_EVE_SCENE_PROBE or pass --binary PATH."
	exit 1
fi
probe_assets="${probe_binary:h}/Assets"
if [[ ! -d "$probe_assets" ]]; then
	print -u2 "The staged probe Assets directory is missing: $probe_assets"
	exit 1
fi

app_path="${TRINITY_EVE_GATE_APP:-$HOME/Applications/Trinity EVE Gate Journey.app}"
contents="$app_path/Contents"
macos="$contents/MacOS"
launcher="$macos/TrinityEveGateJourney"

if [[ "$app_path" != *.app ]]; then
	print -u2 "Refusing to replace a bundle path that does not end in .app: $app_path"
	exit 1
fi
if [[ -e "$app_path" && ! -f "$contents/Info.plist" ]]; then
	print -u2 "Refusing to replace an existing directory that is not an app bundle: $app_path"
	exit 1
fi
if [[ -f "$contents/Info.plist" ]]; then
	existing_id="$(plutil -extract CFBundleIdentifier raw "$contents/Info.plist" 2>/dev/null || true)"
	if [[ "$existing_id" != com.berkeleynerd.trinity.evegatejourney ]]; then
		print -u2 "Refusing to replace app bundle with identifier '$existing_id': $app_path"
		exit 1
	fi
fi

rm -rf "$app_path"
mkdir -p "$macos"
cp "$0" "$launcher"
chmod +x "$launcher"
ln -s "$probe_binary" "$macos/TrinityEveGateJourney.bin"
ln -s "$probe_assets" "$macos/Assets"

plutil -create xml1 "$contents/Info.plist"
plutil -insert CFBundleDevelopmentRegion -string en "$contents/Info.plist"
plutil -insert CFBundleDisplayName -string "Trinity EVE Gate Journey" "$contents/Info.plist"
plutil -insert CFBundleExecutable -string TrinityEveGateJourney "$contents/Info.plist"
plutil -insert CFBundleIdentifier -string com.berkeleynerd.trinity.evegatejourney "$contents/Info.plist"
plutil -insert CFBundleInfoDictionaryVersion -string 6.0 "$contents/Info.plist"
plutil -insert CFBundleName -string "Trinity EVE Gate Journey" "$contents/Info.plist"
plutil -insert CFBundlePackageType -string APPL "$contents/Info.plist"
plutil -insert CFBundleShortVersionString -string 1.0 "$contents/Info.plist"
plutil -insert NSHighResolutionCapable -bool true "$contents/Info.plist"

print "Installed $app_path"
print "Probe: $probe_binary"

if [[ "$install_only" == false ]]; then
	/usr/bin/open -n "$app_path"
fi
