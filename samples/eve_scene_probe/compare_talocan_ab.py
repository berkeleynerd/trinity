#!/usr/bin/env python3
"""Compare Talocan A/B captures without PIL/numpy.

Converts both PNGs to BMP with sips (host macOS), parses the BMPs directly,
and emits a JSON verdict. Two modes:

  aa  determinism proof: the pair must be (near) identical
  ab  texture-set proof: the pair must differ inside a plausible band —
      more than codec noise, less than a scene-wide change
"""

import argparse
import json
import os
import struct
import subprocess
import sys
import tempfile


def png_to_pixels(png_path, work_dir):
    bmp_path = os.path.join(work_dir, os.path.basename(png_path) + ".bmp")
    subprocess.run(["sips", "-s", "format", "bmp", png_path, "--out", bmp_path],
                   check=True, capture_output=True)
    with open(bmp_path, "rb") as handle:
        data = handle.read()
    if data[:2] != b"BM":
        raise ValueError(f"not a BMP: {bmp_path}")
    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    header_size = struct.unpack_from("<I", data, 14)[0]
    if header_size < 40:
        raise ValueError(f"unsupported BMP header size {header_size}")
    width, height = struct.unpack_from("<ii", data, 18)
    planes, bit_count = struct.unpack_from("<HH", data, 26)
    compression = struct.unpack_from("<I", data, 30)[0]
    if planes != 1 or bit_count not in (24, 32) or compression not in (0, 3):
        raise ValueError(f"unsupported BMP layout: planes={planes} bits={bit_count} compression={compression}")

    def mask_to_byte_index(mask):
        for index in range(4):
            if mask == 0xFF << (index * 8):
                return index
        raise ValueError(f"non-byte-aligned BMP channel mask 0x{mask:08x}")

    if compression == 3:
        # BI_BITFIELDS: masks sit right after BITMAPINFOHEADER, which is also
        # where the V4/V5 headers place them.
        red_mask, green_mask, blue_mask = struct.unpack_from("<III", data, 54)
        channel_indices = tuple(mask_to_byte_index(m) for m in (red_mask, green_mask, blue_mask))
    else:
        channel_indices = (2, 1, 0)

    bottom_up = height > 0
    height = abs(height)
    bytes_per_pixel = bit_count // 8
    stride = (width * bytes_per_pixel + 3) & ~3
    rows = []
    for y in range(height):
        source_y = (height - 1 - y) if bottom_up else y
        row_start = pixel_offset + source_y * stride
        rows.append(data[row_start : row_start + width * bytes_per_pixel])
    return width, height, bytes_per_pixel, channel_indices, rows


def main():
    parser = argparse.ArgumentParser(description="Compare two probe captures (A/A or A/B)")
    parser.add_argument("--a", required=True)
    parser.add_argument("--b", required=True)
    parser.add_argument("--mode", required=True, choices=["aa", "ab"])
    parser.add_argument("--noise-threshold", type=int, default=2,
                        help="Per-channel delta a pixel must exceed to count as different")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as work_dir:
        width_a, height_a, bpp_a, channels_a, rows_a = png_to_pixels(args.a, work_dir)
        width_b, height_b, bpp_b, channels_b, rows_b = png_to_pixels(args.b, work_dir)

    same_dimensions = (width_a, height_a) == (width_b, height_b)
    diff_pixels = 0
    delta_sum = 0
    channel_samples = 0
    bbox = None
    if same_dimensions and bpp_a == bpp_b and channels_a == channels_b:
        for y in range(height_a):
            row_a = rows_a[y]
            row_b = rows_b[y]
            if row_a == row_b:
                channel_samples += width_a * 3
                continue
            for x in range(width_a):
                base = x * bpp_a
                pixel_delta = 0
                for channel in channels_a:
                    delta = abs(row_a[base + channel] - row_b[base + channel])
                    delta_sum += delta
                    pixel_delta = max(pixel_delta, delta)
                channel_samples += 3
                if pixel_delta > args.noise_threshold:
                    diff_pixels += 1
                    if bbox is None:
                        bbox = [x, y, x, y]
                    else:
                        bbox[0] = min(bbox[0], x)
                        bbox[1] = min(bbox[1], y)
                        bbox[2] = max(bbox[2], x)
                        bbox[3] = max(bbox[3], y)

    total_pixels = width_a * height_a if same_dimensions else 0
    diff_pixel_fraction = (diff_pixels / total_pixels) if total_pixels else 1.0
    mean_abs_delta = (delta_sum / channel_samples) if channel_samples else 255.0

    if args.mode == "aa":
        checks = {
            "same_dimensions": same_dimensions,
            "diff_fraction_at_most_0_1_percent": diff_pixel_fraction <= 0.001,
            "mean_abs_delta_at_most_0_05": mean_abs_delta <= 0.05,
        }
    else:
        checks = {
            "same_dimensions": same_dimensions,
            "diff_fraction_at_least_0_1_percent": diff_pixel_fraction >= 0.001,
            "diff_fraction_at_most_60_percent": diff_pixel_fraction <= 0.60,
            "difference_is_localized": bbox is not None,
        }

    passed = all(checks.values())
    print(json.dumps({
        "validationPassed": passed,
        "mode": args.mode,
        "checks": checks,
        "metrics": {
            "dimensions": [width_a, height_a],
            "diff_pixel_fraction": round(diff_pixel_fraction, 6),
            "mean_abs_delta": round(mean_abs_delta, 4),
            "diff_bbox": bbox,
        },
    }, indent=2, sort_keys=True))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
