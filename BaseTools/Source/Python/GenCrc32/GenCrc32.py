#
# @file
# Calculate Crc32 value and Verify Crc32 value for input data.
#
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import sys
import argparse
import zlib

__prog__ = 'GenCrc32'
__description__ = 'Tool has two modes of operation: Encode and Decode. \
                  Encode calculates CRC32 of a file and appends checksum \
                  to the beginning of file. Decode checks stored CRC32 \
                  and removes it from file.'
__version__ = '0.3'

MSG_INFO = 0
MSG_ERROR = 1
MSG_DEBUG = 2

STATUS_SUCCESS = 0
STATUS_ERROR = 2


def log(args, message_type, message):
    if (((args.verbose and
        (message_type == MSG_INFO or
         message_type == MSG_ERROR)) or
        (args.debug and
        (message_type == MSG_DEBUG or
         message_type == MSG_INFO or
         message_type == MSG_ERROR))) and
            not args.quiet):
        print("%s: %s" % (__prog__, message))


# replacement for int.from_bytes()
# for compatability with python 2
def uint32_from_bytes(data):
    barray = bytearray(data)
    return (
        (barray[3] << 24) |
        (barray[2] << 16) |
        (barray[1] << 8) |
        (barray[0] << 0))


# replacement for int.to_bytes()
# for compatability with python 2
def uint32_to_bytes(uint):
    return bytearray([
            (uint >> 0) & 0xff,
            (uint >> 8) & 0xff,
            (uint >> 16) & 0xff,
            (uint >> 24) & 0xff])


def parse_args():
    parser = argparse.ArgumentParser(prog=__prog__,
                                     description=__description__,
                                     conflict_handler='resolve')
    parser.add_argument('--version',
                        action='version',
                        version=__version__,
                        help='Show version number and exit')
    parser.add_argument('--debug',
                        type=int,
                        metavar='[0-9]',
                        choices=range(0, 10),
                        default=0,
                        help='Output DEBUG statements')
    logging = parser.add_mutually_exclusive_group(required=False)
    logging.add_argument('-v',
                         '--verbose',
                         action='store_true',
                         help='Print informational statements')
    logging.add_argument('-q',
                         '--quiet',
                         action='store_true',
                         help='Returns the exit code, \
                         error messages will be displayed')
    operation = parser.add_mutually_exclusive_group(required=True)
    operation.add_argument('-e',
                           '--encode',
                           action='store_true',
                           help='Calculate CRC32 value for the input file')
    operation.add_argument('-d',
                           '--decode',
                           action='store_true',
                           help='Verify CRC32 value for the input file')
    parser.add_argument('-o',
                        '--output',
                        dest='output_file',
                        type=str,
                        help='Output file name',
                        required=True)
    parser.add_argument(dest='input_file',
                        type=argparse.FileType('rb'),
                        help='Input file name')
    return parser.parse_args()


def main():
    args = parse_args()
    log(args, MSG_DEBUG, "Input file name: %s" % args.input_file.name)
    log(args, MSG_DEBUG, "Output file name: %s" % args.output_file)
    if args.encode:
        log(args, MSG_INFO, "Operation: Encode")
    if args.decode:
        log(args, MSG_INFO, "Operation: Decode")

    # Reading input file to buffer
    data = bytearray()
    header = bytearray()
    log(args, MSG_INFO, "Reading input file")
    try:
        if args.decode:
            header = args.input_file.read(4)
        data = args.input_file.read()
        args.input_file.close()
    except MemoryError as e:
        log(args, MSG_ERROR, ("Error reading input file, "
            "cannot allocate memory"))
        sys.exit(STATUS_ERROR)
    log(args, MSG_INFO, ("Input file length: %d bytes "
        "(%d bytes crc32 header and %d bytes data)") %
        ((len(header) + len(data)), len(header), len(data)))

    # Calculating CRC32
    log(args, MSG_INFO, "Calculating CRC32")
    data_crc32 = zlib.crc32(data) & 0xffffffff
    if args.decode:
        if len(header) >= 4:
            header_crc32 = uint32_from_bytes(header)
        else:
            log(args, MSG_ERROR, ("Input file size is smaller "
                "than 4 bytes, invalid file format"))
            sys.exit(STATUS_ERROR)
        log(args, MSG_INFO, "CRC32 in input file header is %s" %
            hex(header_crc32))
    log(args, MSG_INFO, "CRC32 of input file data is %s" %
        hex(data_crc32))

    # Perform selected operation by modifying header/data

    # Encoding
    if args.encode:
        log(args, MSG_DEBUG, "Encoding header")
        header = uint32_to_bytes(data_crc32)

    # Verification and decoding
    if args.decode:
        log(args, MSG_INFO, "Verifying checksum")
        header = bytearray()
        if data_crc32 == header_crc32:
            log(args, MSG_INFO, "Checksum verification succeeded")
        else:
            log(args, MSG_ERROR, ("Checksum verification failed, "
                "CRC32 mismatch"))
            sys.exit(STATUS_ERROR)

    # Write output file
    log(args, MSG_INFO, "Writing output file")
    try:
        with open(args.output_file, 'wb') as output_file:
            output_file.write(header)
            output_file.write(data)
    except OSError as e:
        log(args, MSG_ERROR, "Error writing output file: %s",
            format(e.errno, e.strerror))
        sys.exit(STATUS_ERROR)
    log(args, MSG_INFO, ("Output file length: %d bytes "
        "(%d bytes crc32 header and %d bytes data)") %
        ((len(header) + len(data)), len(header), len(data)))

    # Finish
    log(args, MSG_INFO, "Success")
    sys.exit(STATUS_SUCCESS)


if __name__ == "__main__":
    main()
