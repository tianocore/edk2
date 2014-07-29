## @file
#  Apply fixup to VTF binary image for FFS Raw section
#
#  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
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

if filename.lower().find('ia32') >= 0:
    d = open(sys.argv[1], 'rb').read()
    c = ((len(d) + 4 + 7) & ~7) - 4
    if c > len(d):
        c -= len(d)
        f = open(sys.argv[1], 'wb')
        f.write('\x90' * c)
        f.write(d)
        f.close()
else:
    from struct import pack

    PAGE_PRESENT             =     0x01
    PAGE_READ_WRITE          =     0x02
    PAGE_USER_SUPERVISOR     =     0x04
    PAGE_WRITE_THROUGH       =     0x08
    PAGE_CACHE_DISABLE       =    0x010
    PAGE_ACCESSED            =    0x020
    PAGE_DIRTY               =    0x040
    PAGE_PAT                 =    0x080
    PAGE_GLOBAL              =   0x0100
    PAGE_2M_MBO              =    0x080
    PAGE_2M_PAT              =  0x01000

    def NopAlign4k(s):
        c = ((len(s) + 0xfff) & ~0xfff) - len(s)
        return ('\x90' * c) + s

    def PageDirectoryEntries4GbOf2MbPages(baseAddress):

        s = ''
        for i in range(0x800):
            i = (
                    baseAddress + long(i << 21) +
                    PAGE_2M_MBO +
                    PAGE_CACHE_DISABLE +
                    PAGE_ACCESSED +
                    PAGE_DIRTY +
                    PAGE_READ_WRITE +
                    PAGE_PRESENT
                )
            s += pack('Q', i)
        return s

    def PageDirectoryPointerTable4GbOf2MbPages(pdeBase):
        s = ''
        for i in range(0x200):
            i = (
                    pdeBase +
                    (min(i, 3) << 12) +
                    PAGE_CACHE_DISABLE +
                    PAGE_ACCESSED +
                    PAGE_READ_WRITE +
                    PAGE_PRESENT
                )
            s += pack('Q', i)
        return s

    def PageMapLevel4Table4GbOf2MbPages(pdptBase):
        s = ''
        for i in range(0x200):
            i = (
                    pdptBase +
                    (min(i, 0) << 12) +
                    PAGE_CACHE_DISABLE +
                    PAGE_ACCESSED +
                    PAGE_READ_WRITE +
                    PAGE_PRESENT
                )
            s += pack('Q', i)
        return s

    def First4GbPageEntries(topAddress):
        PDE = PageDirectoryEntries4GbOf2MbPages(0L)
        pml4tBase = topAddress - 0x1000
        pdptBase = pml4tBase - 0x1000
        pdeBase = pdptBase - len(PDE)
        PDPT = PageDirectoryPointerTable4GbOf2MbPages(pdeBase)
        PML4T = PageMapLevel4Table4GbOf2MbPages(pdptBase)
        return PDE + PDPT + PML4T

    def AlignAndAddPageTables():
        d = open(sys.argv[1], 'rb').read()
        code = NopAlign4k(d)
        topAddress = 0x100000000 - len(code)
        d = ('\x90' * 4) + First4GbPageEntries(topAddress) + code
        f = open(sys.argv[1], 'wb')
        f.write(d)
        f.close()

    AlignAndAddPageTables()

