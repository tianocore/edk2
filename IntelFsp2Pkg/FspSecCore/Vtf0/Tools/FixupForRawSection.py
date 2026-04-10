## @file
#  Apply fixup to VTF binary image for FFS Raw section
#
#  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import sys

if len(sys.argv) < 2:
    print('Usage: %s <raw_file>' % sys.argv[0], file=sys.stderr)
    sys.exit(1)

filename = sys.argv[1]

if 'ia32' in filename.lower():
    d = open(filename, 'rb').read()
    c = ((len(d) + 4 + 7) & ~7) - 4
    if c > len(d):
        c -= len(d)
        f = open(filename, 'wb')
        f.write(b'\x90' * c)
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
        return (b'\x90' * c) + s

    def PageDirectoryEntries4GbOf2MbPages(baseAddress):

        s = b''
        for i in range(0x800):
            i = (
                    baseAddress + (i << 21) +
                    PAGE_2M_MBO +
                    PAGE_CACHE_DISABLE +
                    PAGE_ACCESSED +
                    PAGE_DIRTY +
                    PAGE_READ_WRITE +
                    PAGE_PRESENT
                )
            s += pack('<Q', i)
        return s

    def PageDirectoryPointerTable4GbOf2MbPages(pdeBase):
        s = b''
        for i in range(0x200):
            i = (
                    pdeBase +
                    (min(i, 3) << 12) +
                    PAGE_CACHE_DISABLE +
                    PAGE_ACCESSED +
                    PAGE_READ_WRITE +
                    PAGE_PRESENT
                )
            s += pack('<Q', i)
        return s

    def PageMapLevel4Table4GbOf2MbPages(pdptBase):
        s = b''
        for i in range(0x200):
            i = (
                    pdptBase +
                    (min(i, 0) << 12) +
                    PAGE_CACHE_DISABLE +
                    PAGE_ACCESSED +
                    PAGE_READ_WRITE +
                    PAGE_PRESENT
                )
            s += pack('<Q', i)
        return s

    def First4GbPageEntries(topAddress):
        PDE = PageDirectoryEntries4GbOf2MbPages(0)
        pml4tBase = topAddress - 0x1000
        pdptBase = pml4tBase - 0x1000
        pdeBase = pdptBase - len(PDE)
        PDPT = PageDirectoryPointerTable4GbOf2MbPages(pdeBase)
        PML4T = PageMapLevel4Table4GbOf2MbPages(pdptBase)
        return PDE + PDPT + PML4T

    def AlignAndAddPageTables():
        d = open(filename, 'rb').read()
        code = NopAlign4k(d)
        topAddress = 0x100000000 - len(code)
        d = (b'\x90' * 4) + First4GbPageEntries(topAddress) + code
        f = open(filename, 'wb')
        f.write(d)
        f.close()

    AlignAndAddPageTables()

