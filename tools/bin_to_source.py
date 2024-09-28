#!/usr/bin/env python3

import os
import sys


def generate(dst_dir, data, base_name):
    hh_file = open(f"{dst_dir}/{base_name}.hh", "w")
    hh_file.write(
        f"""#pragma once
#include <cstdint>
#include <array>

extern const std::array<uint8_t, {len(data)}> {base_name}_data;
"""
    )

    cc_file = open(f"{dst_dir}/{base_name}.cc", "w")
    cc_file.write(
        f"""#include "{base_name}.hh"

const std::array<uint8_t, {len(data)}> {base_name}_data = {{
    """
    )

    for i in range(0, len(data), 20):
        line = bytearray(data[i : i + 20])

        for val in line:
            cc_file.write("0x%02x," % (val))
        cc_file.write("\n    ")

    cc_file.write("\n};\n")


if __name__ == "__main__":  #
    if len(sys.argv) < 4 or (len(sys.argv) - 2) % 2 != 0:
        print(
            "Usage: bin-to-source.py <outdir> <file> <base-name> [<file2> <base-name2>]"
        )
        sys.exit(1)

    out_dir = sys.argv[1]
    try:
        os.mkdir(out_dir)
    except FileExistsError:
        pass

    for i in range(2, len(sys.argv), 2):
        file = sys.argv[i]
        base_name = sys.argv[i + 1]

        f = open(file, "rb")
        data = f.read()
        f.close()

        generate(out_dir, data, base_name)
