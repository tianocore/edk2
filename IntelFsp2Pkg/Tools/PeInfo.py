## @ PeTeinfo.py
#
# Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import argparse
from ctypes import *
from functools import reduce

class c_uint24(Structure):
    """Little-Endian 24-bit Unsigned Integer"""
    _pack_   = 1
    _fields_ = [('Data', (c_uint8 * 3))]

    def __init__(self, val=0):
        self.set_value(val)

    def __str__(self, indent=0):
        return '0x%.6x' % self.value

    def __int__(self):
        return self.get_value()

    def set_value(self, val):
        self.Data[0:3] = Val2Bytes(val, 3)

    def get_value(self):
        return Bytes2Val(self.Data[0:3])

    value = property(get_value, set_value)

class EFI_FIRMWARE_VOLUME_HEADER(Structure):
    _fields_ = [
        ('ZeroVector',           ARRAY(c_uint8, 16)),
        ('FileSystemGuid',       ARRAY(c_uint8, 16)),
        ('FvLength',             c_uint64),
        ('Signature',            ARRAY(c_char, 4)),
        ('Attributes',           c_uint32),
        ('HeaderLength',         c_uint16),
        ('Checksum',             c_uint16),
        ('ExtHeaderOffset',      c_uint16),
        ('Reserved',             c_uint8),
        ('Revision',             c_uint8)
        ]

class EFI_FIRMWARE_VOLUME_EXT_HEADER(Structure):
    _fields_ = [
        ('FvName',               ARRAY(c_uint8, 16)),
        ('ExtHeaderSize',        c_uint32)
        ]

class EFI_FFS_INTEGRITY_CHECK(Structure):
    _fields_ = [
        ('Header',               c_uint8),
        ('File',                 c_uint8)
        ]

class EFI_FFS_FILE_HEADER(Structure):
    _fields_ = [
        ('Name',                 ARRAY(c_uint8, 16)),
        ('IntegrityCheck',       EFI_FFS_INTEGRITY_CHECK),
        ('Type',                 c_uint8),
        ('Attributes',           c_uint8),
        ('Size',                 c_uint24),
        ('State',                c_uint8)
        ]

class EFI_COMMON_SECTION_HEADER(Structure):
    _fields_ = [
        ('Size',                 c_uint24),
        ('Type',                 c_uint8)
        ]

class FSP_COMMON_HEADER(Structure):
     _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32)
        ]

class FSP_INFORMATION_HEADER(Structure):
     _fields_ = [
        ('Signature',                       ARRAY(c_char, 4)),
        ('HeaderLength',                    c_uint32),
        ('Reserved1',                       c_uint16),
        ('SpecVersion',                     c_uint8),
        ('HeaderRevision',                  c_uint8),
        ('ImageRevision',                   c_uint32),
        ('ImageId',                         ARRAY(c_char, 8)),
        ('ImageSize',                       c_uint32),
        ('ImageBase',                       c_uint32),
        ('ImageAttribute',                  c_uint16),
        ('ComponentAttribute',              c_uint16),
        ('CfgRegionOffset',                 c_uint32),
        ('CfgRegionSize',                   c_uint32),
        ('Reserved2',                       c_uint32),
        ('TempRamInitEntryOffset',          c_uint32),
        ('Reserved3',                       c_uint32),
        ('NotifyPhaseEntryOffset',          c_uint32),
        ('FspMemoryInitEntryOffset',        c_uint32),
        ('TempRamExitEntryOffset',          c_uint32),
        ('FspSiliconInitEntryOffset',       c_uint32),
        ('FspMultiPhaseSiInitEntryOffset',  c_uint32),
        ('ExtendedImageRevision',           c_uint16),
        ('Reserved4',                       c_uint16),
        ('FspMultiPhaseMemInitEntryOffset', c_uint32),
        ('FspSmmInitEntryOffset',           c_uint32)
    ]

class FSP_PATCH_TABLE(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint16),
        ('HeaderRevision',       c_uint8),
        ('Reserved',             c_uint8),
        ('PatchEntryNum',        c_uint32)
        ]

class EFI_IMAGE_DATA_DIRECTORY(Structure):
    _fields_ = [
        ('VirtualAddress',       c_uint32),
        ('Size',                 c_uint32)
        ]

class EFI_TE_IMAGE_HEADER(Structure):
    _fields_ = [
        ('Signature',            ARRAY(c_char, 2)),
        ('Machine',              c_uint16),
        ('NumberOfSections',     c_uint8),
        ('Subsystem',            c_uint8),
        ('StrippedSize',         c_uint16),
        ('AddressOfEntryPoint',  c_uint32),
        ('BaseOfCode',           c_uint32),
        ('ImageBase',            c_uint64),
        ('DataDirectoryBaseReloc',  EFI_IMAGE_DATA_DIRECTORY),
        ('DataDirectoryDebug',      EFI_IMAGE_DATA_DIRECTORY)
        ]

class EFI_IMAGE_DOS_HEADER(Structure):
    _fields_ = [
        ('e_magic',              c_uint16),
        ('e_cblp',               c_uint16),
        ('e_cp',                 c_uint16),
        ('e_crlc',               c_uint16),
        ('e_cparhdr',            c_uint16),
        ('e_minalloc',           c_uint16),
        ('e_maxalloc',           c_uint16),
        ('e_ss',                 c_uint16),
        ('e_sp',                 c_uint16),
        ('e_csum',               c_uint16),
        ('e_ip',                 c_uint16),
        ('e_cs',                 c_uint16),
        ('e_lfarlc',             c_uint16),
        ('e_ovno',               c_uint16),
        ('e_res',                ARRAY(c_uint16, 4)),
        ('e_oemid',              c_uint16),
        ('e_oeminfo',            c_uint16),
        ('e_res2',               ARRAY(c_uint16, 10)),
        ('e_lfanew',             c_uint16)
        ]

class EFI_IMAGE_FILE_HEADER(Structure):
    _fields_ = [
        ('Machine',               c_uint16),
        ('NumberOfSections',      c_uint16),
        ('TimeDateStamp',         c_uint32),
        ('PointerToSymbolTable',  c_uint32),
        ('NumberOfSymbols',       c_uint32),
        ('SizeOfOptionalHeader',  c_uint16),
        ('Characteristics',       c_uint16)
        ]

class PE_RELOC_BLOCK_HEADER(Structure):
    _fields_ = [
        ('PageRVA',              c_uint32),
        ('BlockSize',            c_uint32)
        ]

class EFI_IMAGE_OPTIONAL_HEADER32(Structure):
    _fields_ = [
        ('Magic',                         c_uint16),
        ('MajorLinkerVersion',            c_uint8),
        ('MinorLinkerVersion',            c_uint8),
        ('SizeOfCode',                    c_uint32),
        ('SizeOfInitializedData',         c_uint32),
        ('SizeOfUninitializedData',       c_uint32),
        ('AddressOfEntryPoint',           c_uint32),
        ('BaseOfCode',                    c_uint32),
        ('BaseOfData',                    c_uint32),
        ('ImageBase',                     c_uint32),
        ('SectionAlignment',              c_uint32),
        ('FileAlignment',                 c_uint32),
        ('MajorOperatingSystemVersion',   c_uint16),
        ('MinorOperatingSystemVersion',   c_uint16),
        ('MajorImageVersion',             c_uint16),
        ('MinorImageVersion',             c_uint16),
        ('MajorSubsystemVersion',         c_uint16),
        ('MinorSubsystemVersion',         c_uint16),
        ('Win32VersionValue',             c_uint32),
        ('SizeOfImage',                   c_uint32),
        ('SizeOfHeaders',                 c_uint32),
        ('CheckSum'     ,                 c_uint32),
        ('Subsystem',                     c_uint16),
        ('DllCharacteristics',            c_uint16),
        ('SizeOfStackReserve',            c_uint32),
        ('SizeOfStackCommit' ,            c_uint32),
        ('SizeOfHeapReserve',             c_uint32),
        ('SizeOfHeapCommit' ,             c_uint32),
        ('LoaderFlags'     ,              c_uint32),
        ('NumberOfRvaAndSizes',           c_uint32),
        ('DataDirectory',                 ARRAY(EFI_IMAGE_DATA_DIRECTORY, 16))
        ]

class EFI_IMAGE_OPTIONAL_HEADER32_PLUS(Structure):
    _fields_ = [
        ('Magic',                         c_uint16),
        ('MajorLinkerVersion',            c_uint8),
        ('MinorLinkerVersion',            c_uint8),
        ('SizeOfCode',                    c_uint32),
        ('SizeOfInitializedData',         c_uint32),
        ('SizeOfUninitializedData',       c_uint32),
        ('AddressOfEntryPoint',           c_uint32),
        ('BaseOfCode',                    c_uint32),
        ('ImageBase',                     c_uint64),
        ('SectionAlignment',              c_uint32),
        ('FileAlignment',                 c_uint32),
        ('MajorOperatingSystemVersion',   c_uint16),
        ('MinorOperatingSystemVersion',   c_uint16),
        ('MajorImageVersion',             c_uint16),
        ('MinorImageVersion',             c_uint16),
        ('MajorSubsystemVersion',         c_uint16),
        ('MinorSubsystemVersion',         c_uint16),
        ('Win32VersionValue',             c_uint32),
        ('SizeOfImage',                   c_uint32),
        ('SizeOfHeaders',                 c_uint32),
        ('CheckSum'     ,                 c_uint32),
        ('Subsystem',                     c_uint16),
        ('DllCharacteristics',            c_uint16),
        ('SizeOfStackReserve',            c_uint64),
        ('SizeOfStackCommit' ,            c_uint64),
        ('SizeOfHeapReserve',             c_uint64),
        ('SizeOfHeapCommit' ,             c_uint64),
        ('LoaderFlags'     ,              c_uint32),
        ('NumberOfRvaAndSizes',           c_uint32),
        ('DataDirectory',                 ARRAY(EFI_IMAGE_DATA_DIRECTORY, 16))
        ]

class EFI_IMAGE_OPTIONAL_HEADER(Union):
    _fields_ = [
        ('PeOptHdr',             EFI_IMAGE_OPTIONAL_HEADER32),
        ('PePlusOptHdr',         EFI_IMAGE_OPTIONAL_HEADER32_PLUS)
        ]

class EFI_IMAGE_NT_HEADERS32(Structure):
    _fields_ = [
        ('Signature',            c_uint32),
        ('FileHeader',           EFI_IMAGE_FILE_HEADER),
        ('OptionalHeader',       EFI_IMAGE_OPTIONAL_HEADER)
        ]


class EFI_IMAGE_DIRECTORY_ENTRY:
    EXPORT                     = 0
    IMPORT                     = 1
    RESOURCE                   = 2
    EXCEPTION                  = 3
    SECURITY                   = 4
    BASERELOC                  = 5
    DEBUG                      = 6
    COPYRIGHT                  = 7
    GLOBALPTR                  = 8
    TLS                        = 9
    LOAD_CONFIG                = 10

class EFI_FV_FILETYPE:
    ALL                        = 0x00
    RAW                        = 0x01
    FREEFORM                   = 0x02
    SECURITY_CORE              = 0x03
    PEI_CORE                   = 0x04
    DXE_CORE                   = 0x05
    PEIM                       = 0x06
    DRIVER                     = 0x07
    COMBINED_PEIM_DRIVER       = 0x08
    APPLICATION                = 0x09
    SMM                        = 0x0a
    FIRMWARE_VOLUME_IMAGE      = 0x0b
    COMBINED_SMM_DXE           = 0x0c
    SMM_CORE                   = 0x0d
    OEM_MIN                    = 0xc0
    OEM_MAX                    = 0xdf
    DEBUG_MIN                  = 0xe0
    DEBUG_MAX                  = 0xef
    FFS_MIN                    = 0xf0
    FFS_MAX                    = 0xff
    FFS_PAD                    = 0xf0

class EFI_SECTION_TYPE:
    """Enumeration of all valid firmware file section types."""
    ALL                        = 0x00
    COMPRESSION                = 0x01
    GUID_DEFINED               = 0x02
    DISPOSABLE                 = 0x03
    PE32                       = 0x10
    PIC                        = 0x11
    TE                         = 0x12
    DXE_DEPEX                  = 0x13
    VERSION                    = 0x14
    USER_INTERFACE             = 0x15
    COMPATIBILITY16            = 0x16
    FIRMWARE_VOLUME_IMAGE      = 0x17
    FREEFORM_SUBTYPE_GUID      = 0x18
    RAW                        = 0x19
    PEI_DEPEX                  = 0x1b
    SMM_DEPEX                  = 0x1c

def AlignPtr (offset, alignment = 8):
    return (offset + alignment - 1) & ~(alignment - 1)

def Bytes2Val (bytes):
    return reduce(lambda x,y: (x<<8)|y,  bytes[::-1] )

def Val2Bytes (value, blen):
    return [(value>>(i*8) & 0xff) for i in range(blen)]

class PeTeImage:
    def __init__(self, offset, data):
        self.Offset    = offset
        tehdr          = EFI_TE_IMAGE_HEADER.from_buffer (data, 0)
        if   tehdr.Signature == b'VZ': # TE image
            self.TeHdr   = tehdr
        elif tehdr.Signature == b'MZ': # PE image
            self.TeHdr   = None
            self.DosHdr  = EFI_IMAGE_DOS_HEADER.from_buffer (data, 0)
            self.PeHdr   = EFI_IMAGE_NT_HEADERS32.from_buffer (data, self.DosHdr.e_lfanew)
            if self.PeHdr.Signature != 0x4550:
                raise Exception("ERROR: Invalid PE32 header !")
            if self.PeHdr.OptionalHeader.PeOptHdr.Magic == 0x10b: # PE32 image
                if self.PeHdr.FileHeader.SizeOfOptionalHeader < EFI_IMAGE_OPTIONAL_HEADER32.DataDirectory.offset:
                    raise Exception("ERROR: Unsupported PE32 image !")
                if self.PeHdr.OptionalHeader.PeOptHdr.NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY.BASERELOC:
                    raise Exception("ERROR: No relocation information available !")
            elif self.PeHdr.OptionalHeader.PeOptHdr.Magic == 0x20b: # PE32+ image
                if self.PeHdr.FileHeader.SizeOfOptionalHeader < EFI_IMAGE_OPTIONAL_HEADER32_PLUS.DataDirectory.offset:
                    raise Exception("ERROR: Unsupported PE32+ image !")
                if self.PeHdr.OptionalHeader.PePlusOptHdr.NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY.BASERELOC:
                    raise Exception("ERROR: No relocation information available !")
            else:
                raise Exception("ERROR: Invalid PE32 optional header !")
        self.Offset    = offset
        self.Data      = data
        self.RelocList = []

    def IsTeImage(self):
        return  self.TeHdr is not None

    def ParseReloc(self):
        if self.IsTeImage():
            rsize   = self.TeHdr.DataDirectoryBaseReloc.Size
            roffset = sizeof(self.TeHdr) - self.TeHdr.StrippedSize + self.TeHdr.DataDirectoryBaseReloc.VirtualAddress
        else:
            # Assuming PE32 image type (self.PeHdr.OptionalHeader.PeOptHdr.Magic == 0x10b)
            rsize   = self.PeHdr.OptionalHeader.PeOptHdr.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY.BASERELOC].Size
            roffset = self.PeHdr.OptionalHeader.PeOptHdr.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY.BASERELOC].VirtualAddress
            if self.PeHdr.OptionalHeader.PePlusOptHdr.Magic == 0x20b: # PE32+ image
                rsize   = self.PeHdr.OptionalHeader.PePlusOptHdr.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY.BASERELOC].Size
                roffset = self.PeHdr.OptionalHeader.PePlusOptHdr.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY.BASERELOC].VirtualAddress

        alignment = 4
        offset = roffset
        while offset < roffset + rsize:
            offset = AlignPtr(offset, 4)
            blkhdr = PE_RELOC_BLOCK_HEADER.from_buffer(self.Data, offset)
            offset += sizeof(blkhdr)
            # Read relocation type,offset pairs
            rlen  = blkhdr.BlockSize - sizeof(PE_RELOC_BLOCK_HEADER)
            rnum  = int (rlen/sizeof(c_uint16))
            rdata = (c_uint16 * rnum).from_buffer(self.Data, offset)
            for each in rdata:
                roff  = each & 0xfff
                rtype = each >> 12
                if rtype == 0: # IMAGE_REL_BASED_ABSOLUTE:
                    continue
                if ((rtype != 3) and (rtype != 10)): # IMAGE_REL_BASED_HIGHLOW and IMAGE_REL_BASED_DIR64
                    raise Exception("ERROR: Unsupported relocation type %d!" % rtype)
                # Calculate the offset of the relocation
                aoff  = blkhdr.PageRVA + roff
                if self.IsTeImage():
                    aoff += sizeof(self.TeHdr) - self.TeHdr.StrippedSize
                self.RelocList.append((rtype, aoff))
            offset += sizeof(rdata)

    def Rebase(self, delta, fdbin):
        count = 0
        if delta == 0:
            return count

        for (rtype, roff) in self.RelocList:
            if rtype == 3: # IMAGE_REL_BASED_HIGHLOW
                offset = roff + self.Offset
                value  = Bytes2Val(fdbin[offset:offset+sizeof(c_uint32)])
                value += delta
                fdbin[offset:offset+sizeof(c_uint32)] = Val2Bytes(value, sizeof(c_uint32))
                count += 1
            elif rtype == 10: # IMAGE_REL_BASED_DIR64
                offset = roff + self.Offset
                value  = Bytes2Val(fdbin[offset:offset+sizeof(c_uint64)])
                value += delta
                fdbin[offset:offset+sizeof(c_uint64)] = Val2Bytes(value, sizeof(c_uint64))
                count += 1
            else:
                raise Exception('ERROR: Unknown relocation type %d !' % rtype)

        if self.IsTeImage():
            offset  = self.Offset + EFI_TE_IMAGE_HEADER.ImageBase.offset
            size    = EFI_TE_IMAGE_HEADER.ImageBase.size
        else:
            offset  = self.Offset + self.DosHdr.e_lfanew
            offset += EFI_IMAGE_NT_HEADERS32.OptionalHeader.offset
            if self.PeHdr.OptionalHeader.PePlusOptHdr.Magic == 0x20b: # PE32+ image
                offset += EFI_IMAGE_OPTIONAL_HEADER32_PLUS.ImageBase.offset
                size    = EFI_IMAGE_OPTIONAL_HEADER32_PLUS.ImageBase.size
            else:
                offset += EFI_IMAGE_OPTIONAL_HEADER32.ImageBase.offset
                size    = EFI_IMAGE_OPTIONAL_HEADER32.ImageBase.size

        value  = Bytes2Val(fdbin[offset:offset+size]) + delta
        fdbin[offset:offset+size] = Val2Bytes(value, size)

        return count

    def GetRelocCount(self):
        return len(self.RelocList)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('peimage', help='Path to the PE image file')
    args = parser.parse_args()

    reloc_count = GetRelocCountOfPeTE(args.peimage)
    print(f'Relocation entry count: {reloc_count}')

def GetRelocCountOfPeTE(peImagePath):
    with open(peImagePath, 'rb') as f:
        data = f.read()

    pe_image = PeTeImage(0, bytearray(data))
    pe_image.ParseReloc()
    return pe_image.GetRelocCount()

if __name__ == '__main__':
    main()
