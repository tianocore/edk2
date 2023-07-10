## @file
# OBJCOPY parser, it's used to replace FV
#
# Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import argparse
from   ctypes import *
import struct

class ElfSectionHeader64:
    def __init__(self, sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize):
        self.sh_name = sh_name
        self.sh_type = sh_type
        self.sh_flags = sh_flags
        self.sh_addr = sh_addr
        self.sh_offset = sh_offset
        self.sh_size = sh_size
        self.sh_link = sh_link
        self.sh_info = sh_info
        self.sh_addralign = sh_addralign
        self.sh_entsize = sh_entsize

    def pack(self):
        return struct.pack('<IIQQQQIIQQ', self.sh_name, self.sh_type, self.sh_flags, self.sh_addr, self.sh_offset, self.sh_size, self.sh_link, self.sh_info, self.sh_addralign, self.sh_entsize)

    @classmethod
    def unpack(cls, data):
        unpacked_data = struct.unpack('<IIQQQQIIQQ', data)
        return cls(*unpacked_data)

class ElfHeader64:
    def __init__(self, data):
        # Parse the ELF identification bytes
        self.e_ident = struct.unpack('16s', data[:16])[0]
        self.e_type = struct.unpack('H', data[16:18])[0]
        self.e_machine = struct.unpack('H', data[18:20])[0]
        self.e_version = struct.unpack('I', data[20:24])[0]
        self.e_entry = struct.unpack('Q', data[24:32])[0]
        self.e_phoff = struct.unpack('Q', data[32:40])[0]
        self.e_shoff = struct.unpack('Q', data[40:48])[0]
        self.e_flags = struct.unpack('I', data[48:52])[0]
        self.e_ehsize = struct.unpack('H', data[52:54])[0]
        self.e_phentsize = struct.unpack('H', data[54:56])[0]
        self.e_phnum = struct.unpack('H', data[56:58])[0]
        self.e_shentsize = struct.unpack('H', data[58:60])[0]
        self.e_shnum = struct.unpack('H', data[60:62])[0]
        self.e_shstrndx = struct.unpack('H', data[62:64])[0]

    def pack(self):
        # Pack the ELF header data into a binary string
        data = b''
        data += struct.pack('16s', self.e_ident)
        data += struct.pack('H', self.e_type)
        data += struct.pack('H', self.e_machine)
        data += struct.pack('I', self.e_version)
        data += struct.pack('Q', self.e_entry)
        data += struct.pack('Q', self.e_phoff)
        data += struct.pack('Q', self.e_shoff)
        data += struct.pack('I', self.e_flags)
        data += struct.pack('H', self.e_ehsize)
        data += struct.pack('H', self.e_phentsize)
        data += struct.pack('H', self.e_phnum)
        data += struct.pack('H', self.e_shentsize)
        data += struct.pack('H', self.e_shnum)
        data += struct.pack('H', self.e_shstrndx)
        return data

class Elf64_Phdr:
    def __init__(self, data):
        self.p_type = struct.unpack("<L", data[0:4])[0]
        self.p_flags = struct.unpack("<L", data[4:8])[0]
        self.p_offset = struct.unpack("<Q", data[8:16])[0]
        self.p_vaddr = struct.unpack("<Q", data[16:24])[0]
        self.p_paddr = struct.unpack("<Q", data[24:32])[0]
        self.p_filesz = struct.unpack("<Q", data[32:40])[0]
        self.p_memsz = struct.unpack("<Q", data[40:48])[0]
        self.p_align = struct.unpack("<Q", data[48:56])[0]

    def pack(self):
        # Pack the Program header table into a binary string
        data = b''
        data += struct.pack('<L', self.p_type)
        data += struct.pack('<L', self.p_flags)
        data += struct.pack('<Q', self.p_offset)
        data += struct.pack('<Q', self.p_vaddr)
        data += struct.pack('<Q', self.p_paddr)
        data += struct.pack('<Q', self.p_filesz)
        data += struct.pack('<Q', self.p_memsz)
        data += struct.pack('<Q', self.p_align)
        return data

class ElfSectionHeader32:
    def __init__(self, sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link, sh_info, sh_addralign, sh_entsize):
        self.sh_name = sh_name
        self.sh_type = sh_type
        self.sh_flags = sh_flags
        self.sh_addr = sh_addr
        self.sh_offset = sh_offset
        self.sh_size = sh_size
        self.sh_link = sh_link
        self.sh_info = sh_info
        self.sh_addralign = sh_addralign
        self.sh_entsize = sh_entsize

    def pack(self):
        return struct.pack('<IIIIIIIIII', self.sh_name, self.sh_type, self.sh_flags, self.sh_addr, self.sh_offset, self.sh_size, self.sh_link, self.sh_info, self.sh_addralign, self.sh_entsize)

    @classmethod
    def unpack(cls, data):
        unpacked_data = struct.unpack('<IIIIIIIIII', data)
        return cls(*unpacked_data)

class ElfHeader32:
    def __init__(self, data):
        # Parse the ELF identification bytes
        self.e_ident = struct.unpack('16s', data[:16])[0]
        self.e_type = struct.unpack('H', data[16:18])[0]
        self.e_machine = struct.unpack('H', data[18:20])[0]
        self.e_version = struct.unpack('I', data[20:24])[0]
        self.e_entry = struct.unpack('I', data[24:28])[0]
        self.e_phoff = struct.unpack('I', data[28:32])[0]
        self.e_shoff = struct.unpack('I', data[32:36])[0]
        self.e_flags = struct.unpack('I', data[36:40])[0]
        self.e_ehsize = struct.unpack('H', data[40:42])[0]
        self.e_phentsize = struct.unpack('H', data[42:44])[0]
        self.e_phnum = struct.unpack('H', data[44:46])[0]
        self.e_shentsize = struct.unpack('H', data[46:48])[0]
        self.e_shnum = struct.unpack('H', data[48:50])[0]
        self.e_shstrndx = struct.unpack('H', data[50:52])[0]

    def pack(self):
        # Pack the ELF header data into a binary string
        data = b''
        data += struct.pack('16s', self.e_ident)
        data += struct.pack('H', self.e_type)
        data += struct.pack('H', self.e_machine)
        data += struct.pack('I', self.e_version)
        data += struct.pack('I', self.e_entry)
        data += struct.pack('I', self.e_phoff)
        data += struct.pack('I', self.e_shoff)
        data += struct.pack('I', self.e_flags)
        data += struct.pack('H', self.e_ehsize)
        data += struct.pack('H', self.e_phentsize)
        data += struct.pack('H', self.e_phnum)
        data += struct.pack('H', self.e_shentsize)
        data += struct.pack('H', self.e_shnum)
        data += struct.pack('H', self.e_shstrndx)
        return data

class Elf32_Phdr:
    def __init__(self, data):
        self.p_type = struct.unpack("<L", data[0:4])[0]
        self.p_offset = struct.unpack("<L", data[4:8])[0]
        self.p_vaddr = struct.unpack("<L", data[8:12])[0]
        self.p_paddr = struct.unpack("<L", data[12:16])[0]
        self.p_filesz = struct.unpack("<L", data[16:20])[0]
        self.p_memsz = struct.unpack("<L", data[20:24])[0]
        self.p_flags = struct.unpack("<L", data[24:28])[0]
        self.p_align = struct.unpack("<L", data[28:32])[0]

    def pack(self):
        # Pack the Program header table into a binary string
        data = b''
        data += struct.pack('<L', self.p_type)
        data += struct.pack('<L', self.p_offset)
        data += struct.pack('<L', self.p_vaddr)
        data += struct.pack('<L', self.p_paddr)
        data += struct.pack('<L', self.p_filesz)
        data += struct.pack('<L', self.p_memsz)
        data += struct.pack('<L', self.p_flags)
        data += struct.pack('<L', self.p_align)
        return data

def SectionAlignment(NewUPLEntry, AlignmentIndex):
    # Section entry Alignment
    # Alignment is transfer to integer if AlignmentIndex is string.
    if isinstance(AlignmentIndex, str):
        int_num = int(AlignmentIndex, 16)
        int_num = 10 * (int_num//16) + int_num % 16
    else:
        int_num = AlignmentIndex
    if (int_num != 0 or int_num != 1):
        if ((len(NewUPLEntry) % int_num) != 0):
            AlignNumber = int_num - (len(NewUPLEntry) % int_num)
            if (AlignNumber != 0):
                for x in range(AlignNumber):
                    NewUPLEntry = NewUPLEntry + bytearray(b'\0')
    return NewUPLEntry

def SectionEntryFill(SectionEntry, Alignment, Value, Offset):
    # Alignment
    n = 0
    if (len (Value) < Alignment):
        Value = Value.zfill(Alignment)
    for x in range(0, (Alignment//2)):
        Index = '0x' + Value[n] + Value[n + 1]
        SectionEntry[Offset - x] = int(Index,16)
        n += 2
    return SectionEntry

def ElfHeaderParser(UPLEntry):
    # Read EI_CLASS, it stores information that elf with 32-bit or 64-bit architectures.
    EI_CLASS = UPLEntry[4]
    # If Elf is 64-bit objects.
    if (EI_CLASS == 2):
        # Elf header is stored at 0x0-0x40 in 64-bits objects
        ElfHeaderData = UPLEntry[:64]
    # If Elf is 32-bit objects.
    else:
        # Elf header is stored at 0x0-0x34 in 32-bits objects
        ElfHeaderData = UPLEntry[:53]
    # If Elf is 64-bit objects.
    if (EI_CLASS == 2):
        elf_header = ElfHeader64(ElfHeaderData)
        ElfHeaderOffset = elf_header.e_shoff
        SectionHeaderEntryNumber = elf_header.e_shnum
        StringIndexNumber = elf_header.e_shstrndx
        SectionHeaderEntrySize = elf_header.e_shentsize
        StringIndexEntryOffset = ElfHeaderOffset + (StringIndexNumber * SectionHeaderEntrySize)
        unpacked_header = ElfSectionHeader64.unpack(UPLEntry[StringIndexEntryOffset: (StringIndexEntryOffset + SectionHeaderEntrySize)])
        StringIndexSize = unpacked_header.sh_size
        StringIndexOffset = unpacked_header.sh_offset
    # If elf is 32-bit objects.
    else:
        elf_header = ElfHeader32(ElfHeaderData)
        ElfHeaderOffset = elf_header.e_shoff
        SectionHeaderEntryNumber = elf_header.e_shnum
        StringIndexNumber = elf_header.e_shstrndx
        SectionHeaderEntrySize = elf_header.e_shentsize
        StringIndexEntryOffset = ElfHeaderOffset + (StringIndexNumber * SectionHeaderEntrySize)
        unpacked_header = ElfSectionHeader32.unpack(UPLEntry[StringIndexEntryOffset: (StringIndexEntryOffset + SectionHeaderEntrySize)])
        StringIndexSize = unpacked_header.sh_size
        StringIndexOffset = unpacked_header.sh_offset
    return ElfHeaderOffset, SectionHeaderEntryNumber, StringIndexNumber, StringIndexEntryOffset, StringIndexSize, SectionHeaderEntrySize, StringIndexOffset, EI_CLASS

def FindSection(UPLEntry, SectionName):
    ElfHeaderOffset, SectionHeaderEntryNumber, StringIndexNumber, _, StringIndexSize, SectionHeaderEntrySize, StringIndexOffset, EI_CLASS = ElfHeaderParser(UPLEntry)
    # StringIndex is String Index section
    StringIndex = UPLEntry[StringIndexOffset:StringIndexOffset+StringIndexSize]
    # Section header isn't exist if SectionNameOffset = -1.
    StringIndex = StringIndex.decode('utf-8', errors='ignore')
    SectionNameOffset = StringIndex.find(SectionName)
    return SectionNameOffset, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, StringIndexOffset, StringIndexNumber, EI_CLASS

def AddNewSectionEntry64(LastUPLEntrylen, StringIndexValue, SectionSize, Alignment):
    # If elf is 64-bit objects.
    NewSectionEntry = ElfSectionHeader64 (StringIndexValue, 1, 0, 0, LastUPLEntrylen, SectionSize, 0, 0, Alignment, 0)
    sh_bytes = NewSectionEntry.pack()
    return sh_bytes

def AddNewSectionEntry32(LastUPLEntrylen, StringIndexValue, SectionSize, Alignment):
    # If elf is 32-bit objects.
    NewSectionEntry = ElfSectionHeader32 (StringIndexValue, 1, 0, 0, LastUPLEntrylen, SectionSize, 0, 0, Alignment, 0)
    sh_bytes = NewSectionEntry.pack()
    return sh_bytes

def AddSectionHeader64(SHentry, NewUPLEntrylen, SectionHeaderEntrySize, Index, RemoveNameOffset, SectionName, StringIndexNumber):
    SHentry = bytearray(SHentry)
    unpacked_header = ElfSectionHeader64.unpack(SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
    # Section header of section 0 shows 0. It don't modify any offset.
    if (Index != 0):
        # read section offset.
        unpacked_header.sh_offset = NewUPLEntrylen
        # Modify offset of name in section entry
        # if RemoveNameOffset != 0 that is remove function.
        if (RemoveNameOffset != 0):
            if (unpacked_header.sh_name > RemoveNameOffset):
                unpacked_header.sh_name -= len (SectionName)
            # Modify size of name string section entry in section entry.
            if (Index == StringIndexNumber):
                unpacked_header.sh_size -= len (SectionName)
        # added section
        else :
            if (Index == StringIndexNumber):
                unpacked_header.sh_size += len (SectionName)
    NewSHentry = ElfSectionHeader64 (
        unpacked_header.sh_name,
        unpacked_header.sh_type,
        unpacked_header.sh_flags,
        unpacked_header.sh_addr,
        unpacked_header.sh_offset,
        unpacked_header.sh_size,
        unpacked_header.sh_link,
        unpacked_header.sh_info,
        unpacked_header.sh_addralign,
        unpacked_header.sh_entsize).pack()
    return NewSHentry

def AddSectionHeader32(SHentry, NewUPLEntrylen, SectionHeaderEntrySize, Index, RemoveNameOffset, SectionName, StringIndexNumber):
    SHentry = bytearray(SHentry)
    unpacked_header = ElfSectionHeader32.unpack(SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
    if (Index != 0):
        NewSHentry = SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)]
        unpacked_header.sh_offset = NewUPLEntrylen
        # Modify offset of name in section entry
        # if RemoveNameOffset != 0 that is remove function.
        if (RemoveNameOffset != 0):
            if (unpacked_header.sh_name > RemoveNameOffset):
                unpacked_header.sh_name -= len (SectionName)
            # Modify size of name string section entry in section entry.
            if (Index == StringIndexNumber):
                unpacked_header.sh_size -= len (SectionName)
        # added section
        else :
            if (Index == StringIndexNumber):
                unpacked_header.sh_size += len (SectionName)
    NewSHentry = ElfSectionHeader32 (
        unpacked_header.sh_name,
        unpacked_header.sh_type,
        unpacked_header.sh_flags,
        unpacked_header.sh_addr,
        unpacked_header.sh_offset,
        unpacked_header.sh_size,
        unpacked_header.sh_link,
        unpacked_header.sh_info,
        unpacked_header.sh_addralign,
        unpacked_header.sh_entsize).pack()
    return NewSHentry

def ModifyPHSegmentOffset64(NewUPLEntry, ElfHeaderOffset, PHSegmentName):
    # Modify offset and address of program header tables.
    elf_header = ElfHeader64(NewUPLEntry[:64])
    SHentry = NewUPLEntry[ElfHeaderOffset:]
    # Elf program header tables start from 0x40 in 64-bits objects
    PHentry = NewUPLEntry[64: 64 + (elf_header.e_phnum * elf_header.e_phentsize)]
    PHdrs = []
    SHdrs = []
    for i in range(elf_header.e_shnum):
        SHData = SHentry[(i * elf_header.e_shentsize): (i * elf_header.e_shentsize) + elf_header.e_shentsize]
        unpacked_SectionHeader = ElfSectionHeader64.unpack(SHData)
        SHdrs.append(unpacked_SectionHeader)
    for i in range(elf_header.e_phnum):
        PHData = PHentry[(i * elf_header.e_phentsize): (i * elf_header.e_phentsize) + elf_header.e_phentsize]
        unpacked_ProgramHeader = Elf64_Phdr(PHData)
        PHdrs.append(unpacked_ProgramHeader)
    if (PHSegmentName == '.text'):
        PHdrs[0].p_offset = SHdrs[1].sh_offset
        PHdrs[0].p_paddr = SHdrs[1].sh_addr
        PHdrs[4].p_offset = SHdrs[1].sh_offset
        PHdrs[4].p_paddr = SHdrs[1].sh_addr
    elif (PHSegmentName == '.dynamic'):
        PHdrs[1].p_offset = SHdrs[2].sh_offset
        PHdrs[1].p_paddr = SHdrs[2].sh_addr
        PHdrs[3].p_offset = SHdrs[2].sh_offset
        PHdrs[3].p_paddr = SHdrs[2].sh_addr
    elif (PHSegmentName == '.data'):
        PHdrs[2].p_offset = SHdrs[3].sh_offset
        PHdrs[2].p_paddr = SHdrs[3].sh_addr
    packed_PHData = b''
    for phdr in PHdrs:
        packed_PHData += phdr.pack()
    NewUPLEntry = bytearray(NewUPLEntry)
    NewUPLEntry[64: 64 + (elf_header.e_phnum * elf_header.e_phentsize)] = packed_PHData
    return NewUPLEntry

def ModifyPHSegmentOffset32(NewUPLEntry, ElfHeaderOffset, PHSegmentName):
    # Modify offset and address of program header tables.
    # Elf header is stored at 0x0-0x34 in 32-bits objects
    elf_header = ElfHeader32(NewUPLEntry[:52])
    SHentry = NewUPLEntry[ElfHeaderOffset:]
    # Elf program header tables start from 0x34 in 32-bits objects
    PHentry = NewUPLEntry[52: 52 + (elf_header.e_phnum * elf_header.e_phentsize)]
    PHdrs = []
    SHdrs = []
    for i in range(elf_header.e_shnum):
        SHData = SHentry[(i * elf_header.e_shentsize): (i * elf_header.e_shentsize) + elf_header.e_shentsize]
        unpacked_SectionHeader = ElfSectionHeader32.unpack(SHData)
        SHdrs.append(unpacked_SectionHeader)
    for i in range(elf_header.e_phnum):
        PHData = PHentry[(i * elf_header.e_phentsize): (i * elf_header.e_phentsize) + elf_header.e_phentsize]
        unpacked_ProgramHeader = Elf32_Phdr(PHData)
        PHdrs.append(unpacked_ProgramHeader)
    if (PHSegmentName == '.text'):
        PHdrs[0].p_offset = SHdrs[1].sh_offset
        PHdrs[0].p_paddr = SHdrs[1].sh_addr
        PHdrs[0].p_vaddr = SHdrs[1].sh_addr
        PHdrs[2].p_offset = SHdrs[1].sh_offset
        PHdrs[2].p_paddr = SHdrs[1].sh_addr
        PHdrs[0].p_vaddr = SHdrs[1].sh_addr
    elif (PHSegmentName == '.data'):
        PHdrs[1].p_offset = SHdrs[2].sh_offset
        PHdrs[1].p_paddr = SHdrs[2].sh_addr
        PHdrs[1].p_vaddr = SHdrs[2].sh_addr
    packed_PHData = b''
    for phdr in PHdrs:
        packed_PHData += phdr.pack()
    NewUPLEntry = bytearray(NewUPLEntry)
    NewUPLEntry[52: 52 + (elf_header.e_phnum * elf_header.e_phentsize)] = packed_PHData
    return NewUPLEntry

def RemoveSection64(UniversalPayloadEntry, RemoveSectionName):
    # If elf is 64-bit objects.
    # Get offsets as follows:
    #                      1. Section name which will remove in section name string.
    #                      2. Section which will remove.
    #                      3. Section header which will remove.
    with open(UniversalPayloadEntry,'rb') as f:
        UPLEntry = f.read()
        RemoveSectionNameOffset, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, _ = FindSection(UPLEntry, RemoveSectionName)
        if (RemoveSectionNameOffset == -1):
            raise argparse.ArgumentTypeError ('Section: {} not found.'.format (RemoveSectionNameOffset))
        # Read section header entry
        SHentry = UPLEntry[ElfHeaderOffset:]
        # find deleted fv section offset.
        # Elf header is stored at 0x0-0x40 in 64-bits objects
        elf_header = ElfHeader64(UPLEntry[:64])
        Counter = 0
        RemoveIndex = 0
        RemoveNameOffset = 0
        for Index in range(0, elf_header.e_shnum):
            # Read Index of section header.
            unpacked_SectionHeader = ElfSectionHeader64.unpack(SHentry[(Index * elf_header.e_shentsize):((Index * elf_header.e_shentsize) + elf_header.e_shentsize)])
            # Find offset of section name which is removed.
            if (unpacked_SectionHeader.sh_name == RemoveSectionNameOffset):
                RemoveIndex = Counter
                Counter += 1
            else:
                Counter += 1
        # Elf header is recombined.
        # Elf header and program header table in front of first section are reserved.
        # Elf header size is 0x40 with 64-bit object.
        ElfHeaderSize = 64
        ElfHandPH = ElfHeaderSize + (elf_header.e_phnum * elf_header.e_phentsize)
        NewUPLEntry = UPLEntry[:ElfHandPH]
        # Keep Section header and program header table, RemoveSection64() only recombined section and section header.
        NewUPLEntry = bytearray(NewUPLEntry)
        # Sections is recombined.
        # 1. name of deleted section is removed in name string section.
        # 2. deleted section is removed in dll file.
        # 3. re-align sections before and after deleted section.
        NewUPLEntrylen = []
        for Index in range(0, (SectionHeaderEntryNumber)):
            unpacked_SectionHeader = ElfSectionHeader64.unpack(SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
            NewUPLEntrylen.append(len(NewUPLEntry))
            if (Index == 0):
                # Address alignment, section will align with alignment of next section.
                AlignmentIndex = 8
                if (SectionHeaderEntryNumber > 2):
                   unpacked_NextSectionHeader = ElfSectionHeader64.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
            # Section in front of removed section
            elif (Index + 1 == RemoveIndex):
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Read section address alignment
                # If section that will be removed in .dll is not first and last one .
                # Address alignment, section will align with alignment of section after deleted section.
                # Check next and the section after next are not end of section.
                if ((Index + 2) < (SectionHeaderEntryNumber - 1)):
                    unpacked_Next2SectionHeader = ElfSectionHeader64.unpack(SHentry[((Index + 2) * SectionHeaderEntrySize):(((Index + 2) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                    NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_Next2SectionHeader.sh_addralign)
                else:
                    # It is align 8 bytes if next section or the section after next is last one.
                    AlignmentIndex = 8
                    NewUPLEntry = SectionAlignment(NewUPLEntry, AlignmentIndex)
            # section is Deleted section
            elif (Index  == RemoveIndex):
                # Don't add removed section to elf.
                # Find offset of section name.
                RemoveNameOffset = unpacked_SectionHeader.sh_name
            # section is name string section.
            elif (Index  == StringIndexNumber):
                # StringIndex is String Index section
                StringIndex = UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Remove name of removed section in name string section.
                # Section header isn't exist if RemoveSectionNameOffset equal to -1.
                StringIndex = bytearray(StringIndex)
                RemoveSectionName = bytearray(RemoveSectionName, encoding='utf-8')
                RemoveSectionName = RemoveSectionName + bytes('\0', encoding='utf-8')
                StringIndex = StringIndex.replace(RemoveSectionName,b'')
                NewUPLEntry += StringIndex
            # other sections.
            else:
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Address alignment, section will align with alignment of next section.
                if (Index < (SectionHeaderEntryNumber - 1)):
                    NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
                else:
                    # If section is last one.
                    AlignmentIndex = 8
                    NewUPLEntry = SectionAlignment(NewUPLEntry, AlignmentIndex)
        SectionHeaderOffset = len(NewUPLEntry)
        # Add section header
        for Number in range(0, (SectionHeaderEntryNumber)):
            if (Number  != RemoveIndex):
                NewSHentry = AddSectionHeader64(SHentry, NewUPLEntrylen[Number], SectionHeaderEntrySize, Number, RemoveNameOffset, RemoveSectionName, StringIndexNumber)
                NewUPLEntry += NewSHentry
        # Modify number of sections and offset of section header in Elf header.
        elf_header.e_shoff = SectionHeaderOffset
        elf_header.e_shnum -= 1
        NewUPLEntry = elf_header.pack() + NewUPLEntry[64:]
        # write to Elf.
        with open(UniversalPayloadEntry,'wb') as f:
            f.write(NewUPLEntry)

def RemoveSection32(UniversalPayloadEntry, RemoveSectionName):
    # If elf is 32-bit objects.
    # Get offsets as follows:
    #                      1. Section name which will remove in section name string.
    #                      2. Section which will remove.
    #                      3. Section header which will remove.
    with open(UniversalPayloadEntry,'rb') as f:
        UPLEntry = f.read()
        RemoveSectionNameOffset, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, EI_CLASS = FindSection(UPLEntry, RemoveSectionName)
        if (RemoveSectionNameOffset == -1):
            raise argparse.ArgumentTypeError ('Section: {} not found.'.format (RemoveSectionNameOffset))
        # Read section header entry
        SHentry = UPLEntry[ElfHeaderOffset:]
        # find deleted fv section offset.
        # Elf header is stored at 0x0-0x34 in 32-bits objects
        elf_header = ElfHeader32(UPLEntry[:52])
        Counter = 0
        RemoveIndex = 0
        RemoveNameOffset = 0
        for Index in range(0, elf_header.e_shnum):
            # Read Index of section header.
            unpacked_SectionHeader = ElfSectionHeader32.unpack(SHentry[(Index * elf_header.e_shentsize):((Index * elf_header.e_shentsize) + elf_header.e_shentsize)])
            # Find offset of section name which is removed.
            if (unpacked_SectionHeader.sh_name == RemoveSectionNameOffset):
                RemoveIndex = Counter
                Counter += 1
            else:
                Counter += 1
        # Elf header is recombined.
        # Elf header and program header table in front of first section are reserved.
        # Elf header size is 0x34 with 32-bit object.
        ElfHeaderSize = 52
        ElfHandPH = ElfHeaderSize + (elf_header.e_phnum * elf_header.e_phentsize)
        NewUPLEntry = UPLEntry[:ElfHandPH]
        # Keep Section header and program header table, RemoveSection32() only recombined section and section header.
        NewUPLEntry = bytearray(NewUPLEntry)
        # Sections is recombined.
        # 1. name of deleted section is removed in name string section.
        # 2. deleted section is removed in dll file.
        # 3. re-align sections before and after deleted section.
        NewUPLEntrylen = []
        for Index in range(0, (SectionHeaderEntryNumber)):
            unpacked_SectionHeader = ElfSectionHeader32.unpack(SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
            NewUPLEntrylen.append(len(NewUPLEntry))
            if (Index == 0):
                # Address alignment, section will align with alignment of next section.
                AlignmentIndex = 8
                if (SectionHeaderEntryNumber > 2):
                   unpacked_NextSectionHeader = ElfSectionHeader32.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
            # Section in front of removed section
            elif (Index + 1 == RemoveIndex):
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Read section address alignment
                # If section that will be removed in .dll is not first and last one .
                # Address alignment, section will align with alignment of section after deleted section.
                # Check next and the section after next are not end of section.
                if ((Index + 2) < (SectionHeaderEntryNumber - 1)):
                    unpacked_Next2SectionHeader = ElfSectionHeader32.unpack(SHentry[((Index + 2) * SectionHeaderEntrySize):(((Index + 2) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                    NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_Next2SectionHeader.sh_addralign)
                else:
                    # It is align 8 bytes if next section or the section after next is last one.
                    AlignmentIndex = 8
                    NewUPLEntry = SectionAlignment(NewUPLEntry, AlignmentIndex)
            # section is Deleted section
            elif (Index  == RemoveIndex):
                # Don't add removed section to elf.
                # Find offset of section name.
                RemoveNameOffset = unpacked_SectionHeader.sh_name
            # section is name string section.
            elif (Index  == StringIndexNumber):
                # StringIndex is String Index section
                StringIndex = UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Remove name of removed section in name string section.
                # Section header isn't exist if RemoveSectionNameOffset equal to -1.
                StringIndex = bytearray(StringIndex)
                RemoveSectionName = bytearray(RemoveSectionName, encoding='utf-8')
                RemoveSectionName = RemoveSectionName + bytes('\0', encoding='utf-8')
                StringIndex = StringIndex.replace(RemoveSectionName,b'')
                NewUPLEntry += StringIndex
            # other sections.
            else:
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Address alignment, section will align with alignment of next section.
                if (Index < (SectionHeaderEntryNumber - 1)):
                    NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
                else:
                    # If section is last one.
                    AlignmentIndex = 8
                    NewUPLEntry = SectionAlignment(NewUPLEntry, AlignmentIndex)
        SectionHeaderOffset = len(NewUPLEntry)
        # Add section header
        for Number in range(0, (SectionHeaderEntryNumber)):
            if (Number  != RemoveIndex):
                NewSHentry = AddSectionHeader32(SHentry, NewUPLEntrylen[Number], SectionHeaderEntrySize, Number, RemoveNameOffset, RemoveSectionName, StringIndexNumber)
                NewUPLEntry += NewSHentry
        # Modify number of sections and offset of section header in Elf header.
        elf_header.e_shoff = SectionHeaderOffset
        elf_header.e_shnum -= 1
        NewUPLEntry = elf_header.pack() + NewUPLEntry[52:]
        # write to Elf.
        with open(UniversalPayloadEntry,'wb') as f:
            f.write(NewUPLEntry)

def AddSection64(UniversalPayloadEntry, AddSectionName, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, StringIndexNumber, FileBinary, Alignment):
    with open(UniversalPayloadEntry,'rb+') as f:
        UPLEntry = f.read()
        fFileBinary = open(FileBinary, 'rb')
        Binary_File = fFileBinary.read()
        ElfHeaderOffset, SectionHeaderEntryNumber, StringIndexNumber, _, _, SectionHeaderEntrySize, _, _ = ElfHeaderParser(UPLEntry)
        # Read section header entry
        SHentry = UPLEntry[ElfHeaderOffset:]
        # Elf header is recombined.
        # Elf header and program header table in front of first section are reserved.
        # Elf header is stored at 0x0-0x40 in 64-bits objects
        elf_header = ElfHeader64(UPLEntry[:64])
        # Elf header size is 0x40 with 64-bit object.
        ElfHeaderSize = 64
        ElfHandPH = ElfHeaderSize + (elf_header.e_phnum * elf_header.e_phentsize)
        NewUPLEntry = UPLEntry[:ElfHandPH]
        # Keep Section header and program header table, AddSection64() only recombined section and section header.
        NewUPLEntry = bytearray(NewUPLEntry)
        # Sections is recombined.
        # 1. name of added section is added in name string section.
        # 2. added section is added in dll file.
        # 3. re-align sections before and after added section.
        NewUPLEntrylen = []
        StringIndexValue = 0
        for Index in range(0, SectionHeaderEntryNumber):
            NewUPLEntrylen.append(len(NewUPLEntry))
            unpacked_SectionHeader = ElfSectionHeader64.unpack(SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
            # Sections is recombined.
            if (Index == 0):
                # Address alignment, section will align with alignment of next section.
                AlignmentIndex = 8
                if (SectionHeaderEntryNumber > 2):
                    unpacked_NextSectionHeader = ElfSectionHeader64.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
            # Section is last one.
            elif (Index == (SectionHeaderEntryNumber - 1)):
                # Add new section at the end.
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                NewUPLEntry = SectionAlignment(NewUPLEntry, Alignment)
                LastUPLEntrylen = len(NewUPLEntry)
                NewUPLEntry += Binary_File
                # Address alignment, section will align with alignment of next section.
                AlignmentIndex = 8
                NewUPLEntry = SectionAlignment(NewUPLEntry, AlignmentIndex)
            # section is name string section.
            elif (Index  == StringIndexNumber):
                # StringIndex is String Index section
                StringIndex = UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Read name of added Section after StringIndex is transform into string.
                StringIndex = bytearray(StringIndex)
                StringIndexValue = len(StringIndex)
                AddSectionName = bytearray(AddSectionName, encoding='utf-8') + bytes('\0', encoding='utf-8')
                StringIndex += AddSectionName
                NewUPLEntry += StringIndex
            # section after name string section but not last one.
            elif ((Index > StringIndexNumber) and (Index < (SectionHeaderEntryNumber - 1))):
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Address alignment, section will align with alignment of next section.
                unpacked_NextSectionHeader = ElfSectionHeader64.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
            # Section before name string section.
            else:
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                    # Address alignment, section will align with alignment of next section.
                if (Index < (SectionHeaderEntryNumber - 1)):
                    unpacked_NextSectionHeader = ElfSectionHeader64.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                    NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
        SectionHeaderOffset = len(NewUPLEntry)
        RemoveNameOffset = 0
        # Add section header
        for Number in range(0, (SectionHeaderEntryNumber)):
            NewSHentry = AddSectionHeader64(SHentry, NewUPLEntrylen[Number], SectionHeaderEntrySize, Number, RemoveNameOffset, AddSectionName, StringIndexNumber)
            NewUPLEntry += NewSHentry
        NewUPLEntry += bytearray(AddNewSectionEntry64(LastUPLEntrylen, StringIndexValue, len(Binary_File), Alignment))
        # Modify number of sections and offset of section header in Elf header.
        # Modify offset in in Elf header.
        elf_header.e_shoff = SectionHeaderOffset
        elf_header.e_shnum += 1
        elf_header = elf_header.pack()
        UPLEntryBin = elf_header + NewUPLEntry[64:]
        # Modify offsets and address of program header table in elf.
        PHSegmentName = '.text'
        _, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, _ = FindSection(UPLEntryBin, PHSegmentName)
        UPLEntryBin = ModifyPHSegmentOffset64(UPLEntryBin, ElfHeaderOffset, PHSegmentName)
        # Modify offsets and address of program header table in elf.
        PHSegmentName = '.dynamic'
        _, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, _ = FindSection(UPLEntryBin, PHSegmentName)
        UPLEntryBin = ModifyPHSegmentOffset64(UPLEntryBin, ElfHeaderOffset, PHSegmentName)
        # Modify offsets and address of program header table in elf.
        PHSegmentName = '.data'
        _, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, _ = FindSection(UPLEntryBin, PHSegmentName)
        UPLEntryBin = ModifyPHSegmentOffset64(UPLEntryBin, ElfHeaderOffset, PHSegmentName)
    fFileBinary.close()
    return UPLEntryBin

def AddSection32(UniversalPayloadEntry, AddSectionName, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, StringIndexNumber, FileBinary, Alignment):
    with open(UniversalPayloadEntry,'rb+') as f:
        # Read Elf and binary which will be write to elf.
        UPLEntry = f.read()
        fFileBinary = open(FileBinary, 'rb')
        Binary_File = fFileBinary.read()
        ElfHeaderOffset, SectionHeaderEntryNumber, StringIndexNumber, _, _, SectionHeaderEntrySize, _, _ = ElfHeaderParser(UPLEntry)
        # Read section header entry
        SHentry = UPLEntry[ElfHeaderOffset:]
        # Elf header is recombined.
        # Elf header and program header table in front of first section are reserved.
        # Elf header is stored at 0x0-0x34 in 32-bits objects
        elf_header = ElfHeader32(UPLEntry[:52])
        # Elf header size is 0x34 with 32-bit object.
        ElfHeaderSize = 52
        ElfHandPH = ElfHeaderSize + (elf_header.e_phnum * elf_header.e_phentsize)
        NewUPLEntry = UPLEntry[:ElfHandPH]
        # Keep Section header and program header table, AddSection32() only recombined section and section header.
        NewUPLEntry = bytearray(NewUPLEntry)
        # Sections is recombined.
        # 1. name of added section is added in name string section.
        # 2. added section is added in dll file.
        # 3. re-align sections before and after added section.
        NewUPLEntrylen = []
        StringIndexValue = 0
        for Index in range(0, SectionHeaderEntryNumber):
            NewUPLEntrylen.append(len(NewUPLEntry))
            unpacked_SectionHeader = ElfSectionHeader32.unpack(SHentry[(Index * SectionHeaderEntrySize):((Index * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
            # Sections is recombined.
            if (Index == 0):
                # Address alignment, section will align with alignment of next section.
                AlignmentIndex = 8
                if (SectionHeaderEntryNumber > 2):
                    unpacked_NextSectionHeader = ElfSectionHeader32.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
            # Section is last one.
            elif (Index == (SectionHeaderEntryNumber - 1)):
                # Add new section at the end.
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                NewUPLEntry = SectionAlignment(NewUPLEntry, Alignment)
                LastUPLEntrylen = len(NewUPLEntry)
                NewUPLEntry += Binary_File
                # Address alignment, section will align with alignment of next section.
                AlignmentIndex = 8
                NewUPLEntry = SectionAlignment(NewUPLEntry, AlignmentIndex)
            # section is name string section.
            elif (Index  == StringIndexNumber):
                # StringIndex is String Index section
                StringIndex = UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Read name of added Section after StringIndex is transform into string.
                StringIndex = bytearray(StringIndex)
                StringIndexValue = len(StringIndex)
                AddSectionName = bytearray(AddSectionName, encoding='utf-8') + bytes('\0', encoding='utf-8')
                StringIndex += AddSectionName
                NewUPLEntry += StringIndex
            # section after name string section but not last one.
            elif ((Index > StringIndexNumber) and (Index < (SectionHeaderEntryNumber - 1))):
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                # Address alignment, section will align with alignment of next section.
                unpacked_NextSectionHeader = ElfSectionHeader32.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
            # Section before name string section.
            else:
                NewUPLEntry += UPLEntry[unpacked_SectionHeader.sh_offset:(unpacked_SectionHeader.sh_offset + unpacked_SectionHeader.sh_size)]
                    # Address alignment, section will align with alignment of next section.
                if (Index < (SectionHeaderEntryNumber - 1)):
                    unpacked_NextSectionHeader = ElfSectionHeader32.unpack(SHentry[((Index + 1) * SectionHeaderEntrySize):(((Index + 1) * SectionHeaderEntrySize) + SectionHeaderEntrySize)])
                    NewUPLEntry = SectionAlignment(NewUPLEntry, unpacked_NextSectionHeader.sh_addralign)
        SectionHeaderOffset = len(NewUPLEntry)
        RemoveNameOffset = 0
        # Add section header
        for Number in range(0, (SectionHeaderEntryNumber)):
            NewSHentry = AddSectionHeader32(SHentry, NewUPLEntrylen[Number], SectionHeaderEntrySize, Number, RemoveNameOffset, AddSectionName, StringIndexNumber)
            NewUPLEntry += NewSHentry
        NewUPLEntry += bytearray(AddNewSectionEntry32(LastUPLEntrylen, StringIndexValue, len(Binary_File), Alignment))
        # Modify number of sections and offset of section header in Elf header.
        # Modify offset in in Elf header.
        elf_header.e_shoff = SectionHeaderOffset
        elf_header.e_shnum += 1
        PHTableSize = elf_header.e_phentsize
        elf_header = elf_header.pack()
        UPLEntryBin = elf_header + NewUPLEntry[52:]
        # Modify offsets and address of program header table in elf.
        PHSegmentName = '.text'
        _, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, _ = FindSection(UPLEntryBin, PHSegmentName)
        UPLEntryBin = ModifyPHSegmentOffset32(UPLEntryBin, ElfHeaderOffset, PHSegmentName)
        # Modify offsets and address of program header table in elf. Its are stored at 0x08-0x0F and 0x10-0x17
        PHSegmentName = '.data'
        _, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, _ = FindSection(UPLEntryBin, PHSegmentName)
        UPLEntryBin = ModifyPHSegmentOffset32(UPLEntryBin, ElfHeaderOffset, PHSegmentName)
    fFileBinary.close()
    return UPLEntryBin

def ReplaceFv (UniversalPayloadEntry, FileBinary, AddSectionName, Alignment = 16):
    with open(UniversalPayloadEntry,'rb+') as f:
        UPLEntry = f.read()
        SectionNameOffset, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, _, StringIndexNumber, EI_CLASS = FindSection(UPLEntry, AddSectionName)
    # If elf is 64-bit objects.
    if (EI_CLASS == 2):
        # Remove section if it exists.
        if (SectionNameOffset != -1):
            RemoveSection64(UniversalPayloadEntry, AddSectionName)
        # Add section.
        NewUPLEntry = AddSection64(UniversalPayloadEntry, AddSectionName, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, StringIndexNumber, FileBinary, Alignment)
    # If elf is 32-bit objects.
    else:
        # Remove section if it exists.
        if (SectionNameOffset != -1):
            RemoveSection32(UniversalPayloadEntry, AddSectionName)
        # Add section.
        NewUPLEntry = AddSection32(UniversalPayloadEntry, AddSectionName, ElfHeaderOffset, SectionHeaderEntrySize, SectionHeaderEntryNumber, StringIndexNumber, FileBinary, Alignment)
    with open(UniversalPayloadEntry,'wb') as f:
        f.write(NewUPLEntry)
    return 0
