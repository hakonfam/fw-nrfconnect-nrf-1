#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


import hashlib
import argparse
from intelhex import IntelHex


def parse_args():
    parser = argparse.ArgumentParser(
        description="Hash data from file(s).",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--infile", "-i", "--in", "-in", required=True, nargs="+",
                        help="Hash the contents of the specified file(s). If a *.hex file is given, the contents will "
                             "first be converted to binary. For all other file types, no conversion is done.")
    parser.add_argument("--output", "-o", "--out", required=True, nargs="+", type=argparse.FileType('w'),
                        help="Output file(s)")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    for infile, outfile in zip(args.infile, args.outfile):
        if infile.endswith('.hex'):
            ih = IntelHex(args.infile)
            if len(ih) - 1 != (ih.maxaddr() - ih.minaddr()):
                raise RuntimeError("Non-contiguous hex file not supported.")
            to_hash = ih.tobinstr()
        else:
            to_hash = open(args.infile, 'rb').read()
        outfile.write(hashlib.sha256(to_hash).digest())
