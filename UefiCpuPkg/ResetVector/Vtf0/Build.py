## @file
#  Automate the process of building the various reset vector types
#
#  Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import subprocess
import sys

PAGE_TABLE_2M  = 'PageTable2M'
PAGE_TABLE_1G  = 'PageTable1G'
FILE_FORMAT    = '.raw'
ALL_RAW_FORMAT = '*' + FILE_FORMAT
IA32           = 'IA32'
X64            = 'X64'

# Pre-Define a Macros for Page Table
PAGE_TABLES = {
    PAGE_TABLE_2M : "PAGE_TABLE_2M",
    PAGE_TABLE_1G : "PAGE_TABLE_1G"
}

def RunCommand(commandLine):
    return subprocess.call(commandLine)

# Check for all raw binaries and delete them
for root, dirs, files in os.walk('Bin'):
    for file in files:
        if file.endswith(FILE_FORMAT):
            os.remove(os.path.join(root, file))

for arch in ('ia32', 'x64'):
    for debugType in (None, 'port80', 'serial'):
        for pageTable in PAGE_TABLES.keys():
            ret = True
            if arch.lower() == X64.lower():
                directory = os.path.join('Bin', X64, pageTable)
            else:
                directory = os.path.join('Bin', IA32)

            # output raw binary name with arch type
            fileName = 'ResetVector' + '.' + arch

            if debugType is not None:
                fileName += '.' + debugType
            fileName += FILE_FORMAT

            output = os.path.join(directory, fileName)

            # if the directory not exists then create it
            if not os.path.isdir(directory):
                os.makedirs(directory)

            # Prepare the command to execute the nasmb
            commandLine = (
                'nasm',
                '-D', 'ARCH_%s' % arch.upper(),
                '-D', 'DEBUG_%s' % str(debugType).upper(),
                '-D', PAGE_TABLES[pageTable].upper(),
                '-o', output,
                'Vtf0.nasmb',
                )

            print(f"Command : {' '.join(commandLine)}")

            try:
                ret = RunCommand(commandLine)
            except FileNotFoundError:
                print("NASM not found")
            except:
                pass

            if ret != 0:
                print(f"something went wrong while executing {commandLine[-1]}")
                sys.exit()
            print('\tASM\t' + output)

            commandLine = (
                'python',
                'Tools/FixupForRawSection.py',
                output,
                )
            print('\tFIXUP\t' + output)
            ret = RunCommand(commandLine)
            if ret != 0: sys.exit(ret)

