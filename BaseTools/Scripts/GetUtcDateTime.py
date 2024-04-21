## @file
#  Get current UTC date and time information and output as ascii code.
#
#  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

VersionNumber = '0.1'
import sys
import datetime
import argparse

def Main():
    PARSER = argparse.ArgumentParser(
        description='Retrieves UTC date and time information (output ordering: year, date, time) - Version ' + VersionNumber)
    PARSER.add_argument('--year',
                        action='store_true',
                        help='Return UTC year of now. [Example output (2019): 39313032]')
    PARSER.add_argument('--date',
                        action='store_true',
                        help='Return UTC date MMDD of now. [Example output (7th August): 37303830]')
    PARSER.add_argument('--time',
                        action='store_true',
                        help='Return 24-hour-format UTC time HHMM of now. [Example output (14:25): 35323431]')

    ARGS = PARSER.parse_args()
    if len(sys.argv) == 1:
        print ("ERROR: At least one argument is required!\n")
        PARSER.print_help()

    today = datetime.datetime.now(datetime.timezone.utc)
    if ARGS.year:
        ReversedNumber = str(today.year)[::-1]
        print (''.join(hex(ord(HexString))[2:] for HexString in ReversedNumber))
    if ARGS.date:
        ReversedNumber = str(today.strftime("%m%d"))[::-1]
        print (''.join(hex(ord(HexString))[2:] for HexString in ReversedNumber))
    if ARGS.time:
        ReversedNumber = str(today.strftime("%H%M"))[::-1]
        print (''.join(hex(ord(HexString))[2:] for HexString in ReversedNumber))

if __name__ == '__main__':
    Main()
