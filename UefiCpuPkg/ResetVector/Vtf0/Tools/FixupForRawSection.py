## @file
#  Apply fixup to VTF binary image for FFS Raw section
#
#  Copyright (c) 2008 - 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import sys

filename = sys.argv[1]

d = open(sys.argv[1], 'rb').read()
c = ((len(d) + 4 + 7) & ~7) - 4
if c > len(d):
    c -= len(d)
    f = open(sys.argv[1], 'wb')
    f.write(b'\x90' * c)
    f.write(d)
    f.close()
