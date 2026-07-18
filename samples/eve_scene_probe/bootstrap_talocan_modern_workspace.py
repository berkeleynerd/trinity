#!/usr/bin/env python3
"""Seed the out-of-repo Talocan modernization workspace.

Decodes the staged tde1 originals with TrinityDdsToRgba and writes PNG
masters the modern-staging CMake lane encodes back to DDS via
TrinityRgbaToDds. Two modes:

  tint (default) — an unmistakable magenta shift on the albedo, every
    other map passing through: the determinism/A-B probe the rig was
    built on.
  v1 — a procedural modernization pass driven by the albedo's panel-seam
    luminance field: a teal glow synthesized into the _g slot (the client
    ships a flat 827-byte placeholder there, shared with _p3; note the
    wreck material currently ignores the slot — see the doc), a bounded
    gradient perturbation of the normal map's R/G channels (delta-based,
    so it is agnostic to the map's channel basis; the wreck _n is not the
    V5 128-centered convention), and a mean-pivoted contrast curve on the
    grayscale roughness. Albedo, dirt, material, and paint mask pass
    through unchanged.

An optional upscale stage (--upscale pixelmator) super-resolves the
masters to --upscale-size (default 2048) before the transforms run:
albedo, dirt, normal, and roughness through Pixelmator Pro's local ML
Super Resolution (pixelmator_super_resolution.sh), the material and
paint-mask ID maps through exact point sampling, and the glow
synthesized directly at the target size. The stage shells to
ImageMagick (magick) for PNG readback and point resampling; both
Pixelmator Pro and ImageMagick are checked up front with clear
failures. The normal map is NOT renormalized after upscale: the wreck
_n is not the 128-centered V5 basis, so no unit-length invariant
exists to restore.

Everything written here is an EVE-client derivative: the workspace must
stay outside git, and this script refuses paths inside a repository
checkout. Stdlib only (the host python has no PIL/numpy).
"""

import argparse
import os
import shutil
import struct
import subprocess
import sys
import zlib

SUFFIXES = ["a", "d", "g", "m", "n", "p3", "r"]
STEM = "tde1_t1_wreck"


def write_png(path, width, height, rgba):
    def chunk(tag, payload):
        raw = tag + payload
        return struct.pack(">I", len(payload)) + raw + struct.pack(">I", zlib.crc32(raw))

    scanlines = bytearray()
    stride = width * 4
    for y in range(height):
        scanlines.append(0)
        scanlines += rgba[y * stride : (y + 1) * stride]
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    with open(path, "wb") as handle:
        handle.write(b"\x89PNG\r\n\x1a\n")
        handle.write(chunk(b"IHDR", header))
        handle.write(chunk(b"IDAT", zlib.compress(bytes(scanlines), 6)))
        handle.write(chunk(b"IEND", b""))


def require_upscale_tools():
    if shutil.which("magick") is None:
        sys.exit("--upscale pixelmator needs ImageMagick (magick) on PATH; "
                 "brew install imagemagick, or run without --upscale")


def png_to_rgba(path):
    """PNG -> (width, height, RGBA8 bytes) via ImageMagick, tolerant of
    whatever channel layout Pixelmator exported."""
    size = subprocess.run(["magick", "identify", "-format", "%w %h", path],
                          check=True, capture_output=True, text=True).stdout
    width, height = (int(v) for v in size.split())
    raw = subprocess.run(["magick", path, "-depth", "8", "RGBA:-"],
                         check=True, capture_output=True).stdout
    expected = width * height * 4
    if len(raw) != expected:
        sys.exit(f"magick RGBA readback size mismatch for {path}: "
                 f"{len(raw)} != {expected}")
    return width, height, raw


def point_upscale(path_in, path_out, size):
    """Exact point-sampled upscale for ID/mask maps."""
    subprocess.run(["magick", path_in, "-sample", f"{size}x{size}", path_out],
                   check=True)


def tint_albedo(rgba):
    tinted = bytearray(rgba)
    for i in range(0, len(tinted), 4):
        tinted[i] = min(255, tinted[i] + 90)
        tinted[i + 1] = (tinted[i + 1] * 7) // 10
        tinted[i + 2] = min(255, tinted[i + 2] + 90)
    return bytes(tinted)


def parse_hue(text):
    parts = text.split(",")
    if len(parts) != 3:
        raise argparse.ArgumentTypeError("expected R,G,B (three integers 0-255)")
    channels = []
    for part in parts:
        value = int(part)
        if not 0 <= value <= 255:
            raise argparse.ArgumentTypeError("hue channels must be 0-255")
        channels.append(value)
    return tuple(channels)


def luminance(rgba):
    lum = bytearray(len(rgba) // 4)
    for i in range(len(lum)):
        j = i * 4
        lum[i] = (77 * rgba[j] + 150 * rgba[j + 1] + 29 * rgba[j + 2]) >> 8
    return lum


def detail_normal(rgba, lum, width, height, strength):
    """Bounded gradient deltas on the normal R/G channels.

    The wreck _n is not the Astero-era 128-centered basis (R/G means sit
    near 17, B/A pinned 255), so absolute re-authoring would gamble on the
    convention. Signed deltas added to whatever the authored values are
    tilt the surface along the seam gradient under any affine encoding;
    B and A are never touched.
    """
    out = bytearray(rgba)
    limit = max(1, int(60 * strength))
    for y in range(height):
        row = y * width
        nextrow = row + width if y + 1 < height else row
        for x in range(width):
            i = row + x
            here = lum[i]
            dx = (lum[i + 1] if x + 1 < width else here) - here
            dy = lum[nextrow + x] - here
            if dx == 0 and dy == 0:
                continue
            dr = int(dx * strength) // 2
            dg = int(dy * strength) // 2
            dr = -limit if dr < -limit else limit if dr > limit else dr
            dg = -limit if dg < -limit else limit if dg > limit else dg
            o = i * 4
            r = out[o] + dr
            g = out[o + 1] + dg
            out[o] = 0 if r < 0 else 255 if r > 255 else r
            out[o + 1] = 0 if g < 0 else 255 if g > 255 else g
    return bytes(out)


def curve_roughness(rgba, contrast):
    """Mean-pivoted contrast on the grayscale roughness (R/G/B together)."""
    total = 0
    count = len(rgba) // 4
    for i in range(0, len(rgba), 4):
        total += rgba[i]
    pivot = total // count
    table = bytes(
        min(255, max(0, int(pivot + (v - pivot) * contrast)))
        for v in range(256)
    )
    out = bytearray(rgba)
    for i in range(0, len(out), 4):
        out[i] = table[out[i]]
        out[i + 1] = table[out[i + 1]]
        out[i + 2] = table[out[i + 2]]
    return bytes(out)


def synth_glow(lum, width, height, hue_rgb, intensity, threshold):
    """Emissive teal seams from albedo luminance edges.

    Panel grooves read as dark lines in the albedo, so a forward-difference
    gradient peaks along them and is ~flat on clean plating. The paint mask
    (_p3) is the flat client placeholder here and cannot gate the seams; the
    material map (_m) carries real structure and is a possible future gate.
    This pass rides the albedo alone. Bloom in the pipeline spreads the
    single-texel edges, so no explicit blur is applied.
    """
    hue_r, hue_g, hue_b = hue_rgb
    out = bytearray(width * height * 4)
    for y in range(height):
        row = y * width
        nextrow = row + width if y + 1 < height else row
        for x in range(width):
            i = row + x
            here = lum[i]
            right = lum[i + 1] if x + 1 < width else here
            down = lum[nextrow + x]
            edge = abs(right - here) + abs(down - here) - threshold
            o = i * 4
            out[o + 3] = 255
            if edge <= 0:
                continue
            level = int(edge * intensity)
            if level > 255:
                level = 255
            out[o] = (level * hue_r) // 255
            out[o + 1] = (level * hue_g) // 255
            out[o + 2] = (level * hue_b) // 255
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dds-to-rgba", required=True, help="Path to the TrinityDdsToRgba binary")
    parser.add_argument("--originals-dir", required=True,
                        help="Staged originals, e.g. <build>/samples/eve_scene_probe/Assets/dx9/model/ship/talocan/destroyer/tde1")
    parser.add_argument("--workspace", default=os.path.expanduser("~/TalocanModernization/tde1_t1_wreck"),
                        help="Out-of-repo workspace to seed (default: ~/TalocanModernization/tde1_t1_wreck)")
    parser.add_argument("--mode", choices=["tint", "v1"], default="tint",
                        help="tint: magenta albedo spike (determinism/A-B probe, default). "
                             "v1: procedural pass — synthesize a teal seam glow into _g.")
    parser.add_argument("--glow-hue", type=parse_hue, default=(0, 180, 200),
                        help="v1 glow color as R,G,B (default 0,180,200 teal)")
    parser.add_argument("--glow-intensity", type=float, default=2.0,
                        help="v1 glow edge gain (default 2.0; ~1 whispers, 4+ is pronounced)")
    parser.add_argument("--glow-threshold", type=int, default=14,
                        help="v1 glow edge knee; flat plating below this stays black "
                             "(default 14; lower picks up albedo micro-detail as speckle)")
    parser.add_argument("--detail-normal-strength", type=float, default=1.0,
                        help="v1 normal-detail gain; 0 disables the pass (default 1.0, "
                             "deltas bounded to +-60 per channel)")
    parser.add_argument("--roughness-contrast", type=float, default=1.25,
                        help="v1 roughness contrast about the map mean; 1.0 disables "
                             "the pass (default 1.25)")
    parser.add_argument("--upscale", choices=["off", "pixelmator"], default="off",
                        help="off: keep source resolution (default). pixelmator: "
                             "super-resolve masters to --upscale-size via local ML "
                             "(albedo/dirt/normal/roughness; masks nearest; glow "
                             "synthesized at target size)")
    parser.add_argument("--upscale-size", type=int, default=2048,
                        help="upscale target edge in texels (default 2048)")
    args = parser.parse_args()
    if args.upscale == "pixelmator":
        require_upscale_tools()

    workspace = os.path.abspath(args.workspace)
    probe = workspace
    while True:
        if os.path.isdir(os.path.join(probe, ".git")):
            sys.exit(f"refusing workspace inside a git checkout: {probe}")
        parent = os.path.dirname(probe)
        if parent == probe:
            break
        probe = parent
    os.makedirs(workspace, exist_ok=True)

    albedo_lum = None
    albedo_dims = None
    for suffix in SUFFIXES:
        source = os.path.join(args.originals_dir, f"{STEM}_{suffix}.dds")
        rgba_path = os.path.join(workspace, f"{STEM}_{suffix}.rgba")
        meta_path = os.path.join(workspace, f"{STEM}_{suffix}.txt")
        subprocess.run([args.dds_to_rgba, "--input", source,
                        "--output", rgba_path, "--metadata", meta_path], check=True)
        with open(meta_path) as handle:
            width, height = (int(v) for v in handle.read().split())
        with open(rgba_path, "rb") as handle:
            rgba = handle.read()

        note = ""
        if args.upscale == "pixelmator" and width < args.upscale_size and suffix != "g":
            if args.upscale_size % width != 0:
                sys.exit(f"upscale size {args.upscale_size} is not an integer "
                         f"multiple of {width} ({suffix})")
            sr_in = os.path.join(workspace, f"{STEM}_{suffix}_sr_in.png")
            sr_out = os.path.join(workspace, f"{STEM}_{suffix}_sr_out.png")
            write_png(sr_in, width, height, rgba)
            if suffix in ("m", "p3"):
                point_upscale(sr_in, sr_out, args.upscale_size)
                note = ", point-sampled"
            else:
                helper = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      "pixelmator_super_resolution.sh")
                subprocess.run([helper, sr_in, sr_out, str(args.upscale_size)],
                               check=True)
                note = ", ml-sr"
            width, height, rgba = png_to_rgba(sr_out)
            os.remove(sr_in)
            os.remove(sr_out)
            # The glow map skips upscale entirely: v1 synthesizes it at the
            # albedo's (already upscaled) size, and tint mode passes the
            # placeholder through at source size deliberately.
        if args.mode == "tint":
            if suffix == "a":
                rgba = tint_albedo(rgba)
                note = ", tinted"
        else:  # v1 — glow + detail normals + roughness curve
            if suffix == "a":
                albedo_lum = luminance(rgba)
                albedo_dims = (width, height)
            elif suffix in ("g", "n"):
                if albedo_lum is None:
                    sys.exit(f"v1 {suffix} pass needs the albedo decoded first")
                if suffix == "g":
                    # Synthesized at the albedo's size: with the upscale stage
                    # active that is the target size, and the placeholder's own
                    # decoded dimensions are irrelevant.
                    width, height = albedo_dims
                    rgba = synth_glow(albedo_lum, width, height,
                                      args.glow_hue, args.glow_intensity, args.glow_threshold)
                    note = ", glow"
                elif args.detail_normal_strength > 0.0:
                    if albedo_dims != (width, height):
                        sys.exit(f"v1 n pass needs albedo at {width}x{height}; "
                                 f"got albedo {albedo_dims} — cannot derive seams")
                    rgba = detail_normal(rgba, albedo_lum, width, height,
                                         args.detail_normal_strength)
                    note = note + ", detail-normal" if note else ", detail-normal"
            elif suffix == "r" and args.roughness_contrast != 1.0:
                rgba = curve_roughness(rgba, args.roughness_contrast)
                note = note + ", roughness-curve" if note else ", roughness-curve"

        png_path = os.path.join(workspace, f"{STEM}_{suffix}.png")
        write_png(png_path, width, height, rgba)
        os.remove(rgba_path)
        os.remove(meta_path)
        print(f"seeded {png_path} ({width}x{height}{note})")

    print(f"workspace ready ({args.mode}); reconfigure cmake to pick up the masters")


if __name__ == "__main__":
    main()
