## @file
#  Automate the process of building the various reset vector types
#
#  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
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

arch = 'ia32'
debugType = None
output = os.path.join('Bin', 'ResetVec')
output += '.' + arch
if debugType is not None:
    output += '.' + debugType
output += '.raw'
commandLine = (
    'nasm',
    '-D', 'ARCH_%s' % arch.upper(),
    '-D', 'DEBUG_%s' % str(debugType).upper(),
    '-o', output,
    'ResetVectorCode.asm',
    )
ret = RunCommand(commandLine)
print '\tASM\t' + output
if ret != 0: sys.exit(ret)

commandLine = (
    'python',
    'Tools/FixupForRawSection.py',
    output,
    )
print '\tFIXUP\t' + output
ret = RunCommand(commandLine)
if ret != 0: sys.exit(ret)

