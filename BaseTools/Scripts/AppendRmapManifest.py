## @file
# Append an RMAP region manifest trailer to a firmware image.
#
# The manifest is a simple list of FMAP region names the firmware update
# should program. It is consumed by the UefiPayloadPkg FMP device library.
# If the manifest is absent, firmware falls back to full-flash updates.
#
# Copyright (c) 2025, 3mdeb Sp. z o.o. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function

import argparse
import os
import struct
import sys
import subprocess
import tempfile

#
# Globals for help information
#
__prog__        = 'AppendRmapManifest'
__description__ = 'Append an RMAP manifest trailer listing FMAP regions to flash.'
__copyright__   = 'Copyright (c) 2025, 3mdeb Sp. z o.o. All rights reserved.'

# SIGNATURE_32('R','M','A','P')
RMAP_SIGNATURE = 0x50414D52
RMAP_VERSION   = 1
ENTRY_SIZE     = 16


def build_manifest(region_names):
    """Return the encoded manifest bytes for the given region names."""
    entries = []
    for name in region_names:
        try:
            encoded = name.encode('ascii')
        except UnicodeError:
            raise ValueError("Region name '{}' is not ASCII".format(name))
        if len(encoded) > ENTRY_SIZE:
            raise ValueError("Region name '{}' longer than {} bytes".format(name, ENTRY_SIZE))
        entries.append(struct.pack('<{}s'.format(ENTRY_SIZE), encoded))

    trailer = struct.pack('<IHH', RMAP_SIGNATURE, RMAP_VERSION, len(entries))
    return b''.join(entries) + trailer


def main():
    parser = argparse.ArgumentParser(description=__description__)
    parser.add_argument('input', help='Input firmware image (pre-FMP payload)')
    parser.add_argument('-o', '--output', help='Output path (default: overwrite input)')
    parser.add_argument(
        '-r',
        '--region',
        action='append',
        dest='regions',
        required=True,
        help='FMAP region name to flash (repeat for multiple regions)'
    )
    parser.add_argument('--fmp-guid', help='ImageTypeId GUID for GenFmpImage')
    parser.add_argument('--fw-version', type=int, default=1, help='Firmware version for GenFmpImage')
    parser.add_argument('--lsv', type=int, default=0, help='Lowest supported version for GenFmpImage')
    parser.add_argument('--capsule-guid', help='Capsule GUID (defaults to SystemFirmware GUID if omitted)')
    parser.add_argument('--cap-output', help='Optional .cap output; enables capsule build via GenFmpImage/GenCapsule')
    parser.add_argument('--genfmp', default='BaseTools/BinWrappers/PosixLike/GenFmpImage', help='Path to GenFmpImage wrapper')
    parser.add_argument('--gencapsule', default='BaseTools/BinWrappers/PosixLike/GenCapsule', help='Path to GenCapsule wrapper')

    args = parser.parse_args()

    out_path = args.output or args.input
    try:
        with open(args.input, 'rb') as infile:
            image = infile.read()
    except IOError as exc:
        print("error: failed to read '{}': {}".format(args.input, exc), file=sys.stderr)
        return 1

    try:
        manifest = build_manifest(args.regions)
    except ValueError as exc:
        print("error: {}".format(exc), file=sys.stderr)
        return 1

    try:
        with open(out_path, 'wb') as outfile:
            outfile.write(image)
            outfile.write(manifest)
    except IOError as exc:
        print("error: failed to write '{}': {}".format(out_path, exc), file=sys.stderr)
        return 1

    print("Appended {} region(s) manifest to {}".format(len(args.regions), os.path.abspath(out_path)))

    if not args.cap_output:
        return 0

    if not args.fmp_guid:
        print("error: --fmp-guid is required when building a capsule", file=sys.stderr)
        return 1

    capsule_guid_args = []
    if args.capsule_guid:
        capsule_guid_args = ['--guid', args.capsule_guid]
    else:
        # SystemFirmwareCapsule GUID
        capsule_guid_args = ['--guid', '6dcbd5ed-e82d-4c44-bda1-7194199ad92a']

    with tempfile.TemporaryDirectory() as tmpdir:
        fmp_path = os.path.join(tmpdir, 'image.fmp')
        genfmp_cmd = [
            args.genfmp,
            '-i', out_path,
            '-o', fmp_path,
            '-f', args.fmp_guid,
            '-v', str(args.fw_version),
            '-l', str(args.lsv),
        ]
        try:
            subprocess.check_call(genfmp_cmd)
        except subprocess.CalledProcessError as exc:
            print("error: GenFmpImage failed: {}".format(exc), file=sys.stderr)
            return 1

        gencap_cmd = [
            args.gencapsule,
            '--fmp', fmp_path,
            '--capflag', 'PersistAcrossReset',
        ] + capsule_guid_args + [
            '--update',
            '-o', args.cap_output,
        ]
        try:
            subprocess.check_call(gencap_cmd)
        except subprocess.CalledProcessError as exc:
            print("error: GenCapsule failed: {}".format(exc), file=sys.stderr)
            return 1

    print("Built capsule {}".format(os.path.abspath(args.cap_output)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
