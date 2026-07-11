#!/usr/bin/env python3

import argparse
import json
import sys


def load(path):
    with open(path, "r", encoding="utf-8") as stream:
        return json.load(stream)


def main():
    parser = argparse.ArgumentParser(description="Compare RC-10 exposure-off and client tone reports")
    parser.add_argument("--off", required=True)
    parser.add_argument("--client", required=True)
    args = parser.parse_args()

    off = load(args.off)
    client = load(args.client)
    width, height = client["dimensions"]
    pixels = width * height
    checks = {
        "off_report_valid": bool(off["validationPassed"]),
        "client_report_valid": bool(client["validationPassed"]),
        "same_pre_tonemap_hash": off["preTonemapHash"] == client["preTonemapHash"],
        "distinct_post_tonemap_hash": off["postTonemapHash"] != client["postTonemapHash"],
        "strictly_fewer_white_pixels": client["fullyWhitePixels"] < off["fullyWhitePixels"],
        "white_pixels_at_most_0_1_percent": client["fullyWhitePixels"] <= pixels * 0.001,
        "any_saturated_at_most_3_percent": client["anySaturatedChannelPixels"] <= pixels * 0.03,
        "black_pixels_at_most_0_1_percent": client["fullyBlackPixels"] <= pixels * 0.001,
        "p01_retains_75_percent": client["luminance"]["p01"] >= off["luminance"]["p01"] * 0.75,
    }
    passed = all(checks.values())
    print(json.dumps({"validationPassed": passed, "checks": checks}, indent=2, sort_keys=True))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
