## @file
#  Automate the process of building the various reset vector types
#
#  Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import glob
import os
import subprocess
import sys

def RunCommand(commandLine):
    #print ' '.join(commandLine)
    return subprocess.call(commandLine)

for filename in glob.glob(os.path.join('Bin', '*.raw')):
    os.remove(filename)

for arch in ('ia32', 'x64'):
    for debugType in (None, 'port80', 'serial'):
        output = os.path.join('Bin', 'ResetVector')
        output += '.' + arch
        if debugType is not None:
            output += '.' + debugType
        output += '.raw'
        commandLine = (
            'nasm',
            '-D', 'ARCH_%s' % arch.upper(),
            '-D', 'DEBUG_%s' % str(debugType).upper(),
            '-o', output,
            'Vtf0.nasmb',
            )
        print(f"Command : {' '.join(commandLine)}")
        ret = RunCommand(commandLine)
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

