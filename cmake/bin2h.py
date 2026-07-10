#!/usr/bin/env python3
# Copyright © 2026 CCP ehf.

import argparse


def convert_file(input_path, output_path):
    with open(input_path, "rb") as input_file:
        data = input_file.read()

    with open(output_path, "w", encoding="ascii") as output_file:
        for index, byte in enumerate(data):
            if index:
                output_file.write(", ")
            if index and index % 16 == 0:
                output_file.write("\n")
            output_file.write(f"0x{byte:02x}")


def main():
    parser = argparse.ArgumentParser(description="Convert a binary file to a C++ include byte list.")
    parser.add_argument("input", help="Path to the input binary file.")
    parser.add_argument("output", help="Path to the output header fragment.")
    args = parser.parse_args()
    convert_file(args.input, args.output)


if __name__ == "__main__":
    main()
