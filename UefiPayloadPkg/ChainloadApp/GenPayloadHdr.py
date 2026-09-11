#!/usr/bin/env python3
## @file
# Generate C header with embedded payload binary
#
# Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import sys
import os

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <input.fd> <output.h>", file=sys.stderr)
        return 1

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found", file=sys.stderr)
        return 1

    with open(input_file, 'rb') as f:
        data = f.read()

    with open(output_file, 'w') as f:
        f.write("// Auto-generated - do not edit\n")
        f.write(f"// Source: {os.path.basename(input_file)}\n\n")
        #
        # FindFvInPayload() casts addresses inside this array to
        # EFI_FIRMWARE_VOLUME_HEADER * and reads a UINT64 field from
        # them, so the array needs a stated alignment rather than
        # whatever a compiler happens to give a large object.
        #
        f.write("#if defined (_MSC_VER)\n")
        f.write("#define CHAINLOAD_PAYLOAD_ALIGN  __declspec (align (8))\n")
        f.write("#else\n")
        f.write("#define CHAINLOAD_PAYLOAD_ALIGN  __attribute__ ((aligned (8)))\n")
        f.write("#endif\n\n")
        f.write("CHAINLOAD_PAYLOAD_ALIGN STATIC CONST UINT8 mPayloadData[] = {\n")

        for i, byte in enumerate(data):
            if i % 16 == 0:
                f.write("  ")
            f.write(f"0x{byte:02X},")
            if i % 16 == 15:
                f.write("\n")
            else:
                f.write(" ")

        if len(data) % 16 != 0:
            f.write("\n")
        f.write("};\n\n")
        f.write("STATIC CONST UINTN mPayloadSize = sizeof(mPayloadData);\n")

    print(f"Generated {output_file} ({len(data)} bytes)")
    return 0

if __name__ == '__main__':
    sys.exit(main())
