## @file
#  Apply fixup to VTF binary image for FFS Raw section
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import sys

filename = sys.argv[1]

d = open(sys.argv[1], 'rb').read()
c = ((len(d) + 4 + 7) & ~7) - 4
if c > len(d):
    c -= len(d)
    f = open(sys.argv[1], 'wb')
    f.write('\x90' * c)
    f.write(d)
    f.close()
