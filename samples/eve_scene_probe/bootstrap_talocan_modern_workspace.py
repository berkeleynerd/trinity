#!/usr/bin/env python3
"""Seed the out-of-repo Talocan modernization workspace with a tint spike.

Decodes the staged tde1 originals with TrinityDdsToRgba, applies an
unmistakable magenta shift to the albedo (all other maps pass through
unchanged), and writes PNG masters the modern-staging CMake lane encodes
back to DDS via TrinityRgbaToDds. Everything written here is an EVE-client
derivative: the workspace must stay outside git, and this script refuses
paths inside a repository checkout.

Stdlib only (the host python has no PIL/numpy).
"""

import argparse
import os
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


def tint_albedo(rgba):
    tinted = bytearray(rgba)
    for i in range(0, len(tinted), 4):
        tinted[i] = min(255, tinted[i] + 90)
        tinted[i + 1] = (tinted[i + 1] * 7) // 10
        tinted[i + 2] = min(255, tinted[i + 2] + 90)
    return bytes(tinted)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dds-to-rgba", required=True, help="Path to the TrinityDdsToRgba binary")
    parser.add_argument("--originals-dir", required=True,
                        help="Staged originals, e.g. <build>/samples/eve_scene_probe/Assets/dx9/model/ship/talocan/destroyer/tde1")
    parser.add_argument("--workspace", default=os.path.expanduser("~/TalocanModernization/tde1_t1_wreck"),
                        help="Out-of-repo workspace to seed (default: ~/TalocanModernization/tde1_t1_wreck)")
    args = parser.parse_args()

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
        if suffix == "a":
            rgba = tint_albedo(rgba)
        png_path = os.path.join(workspace, f"{STEM}_{suffix}.png")
        write_png(png_path, width, height, rgba)
        os.remove(rgba_path)
        os.remove(meta_path)
        print(f"seeded {png_path} ({width}x{height}{', tinted' if suffix == 'a' else ''})")

    print("workspace ready; reconfigure cmake to pick up the masters")


if __name__ == "__main__":
    main()
