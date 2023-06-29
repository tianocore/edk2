## @file
# This file is used to define the UPL Header C Struct.
#
# Copyright (c) 2023-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from struct import *
from ctypes import *
from FirmwareStorageFormat.Common import *

EFI_COMMON_SECTION_HEADER_LEN = 4
EFI_COMMON_SECTION_HEADER2_LEN = 8

# ELF header.
class ELF_HEADER32(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_Identification',       ARRAY(c_char, 16)),   # /* File identification. */
        ('ELF_Type',                 c_uint16),            # Elf32_Half   /* File type. */
        ('ELF_Machine',              c_uint16),            # Elf32_Half   /* Machine architecture. */
        ('ELF_Version',              c_uint32),            # Elf32_Word   /* ELF format version. */
        ('ELF_Entry',                c_uint32),            # Elf32_Addr   /* Entry point. */
        ('ELF_PHOff',                c_uint32),            # Elf32_Off    /* Program header file offset. */
        ('ELF_SHOff',                c_uint32),            # Elf32_Off    /* Section header file offset. */
        ('ELF_Flags',                c_uint32),            # Elf32_Word   /* Architecture-specific flags. */
        ('ELF_EFSize',               c_uint16),            # Elf32_Half   /* Size of ELF header in bytes. */
        ('ELF_PHEntSize',            c_uint16),            # Elf32_Half   /* Size of program header entry. */
        ('ELF_PHNum',                c_uint16),            # Elf32_Half   /* Number of program header entries. */
        ('ELF_SHEntSize',            c_uint16),            # Elf32_Half   /* Size of section header entry. */
        ('ELF_SHNum',                c_uint16),            # Elf32_Half   /* Number of section header entries. */
        ('ELF_SNStr',                c_uint16),            # Elf32_Half   /* Section name strings section. */
    ]

class ELF_HEADER64(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_Identification',       ARRAY(c_char, 16)),   # /* File identification. */
        ('ELF_Type',                 c_uint16),            # Elf64_Half   /* File type. */
        ('ELF_Machine',              c_uint16),            # Elf64_Half   /* Machine architecture. */
        ('ELF_Version',              c_uint32),            # Elf64_Word   /* ELF format version. */
        ('ELF_Entry',                c_uint64),            # Elf64_Addr   /* Entry point. */
        ('ELF_PHOff',                c_uint64),            # Elf64_Off    /* Program header file offset. */
        ('ELF_SHOff',                c_uint64),            # Elf64_Off    /* Section header file offset. */
        ('ELF_Flags',                c_uint32),            # Elf64_Word   /* Architecture-specific flags. */
        ('ELF_EFSize',               c_uint16),            # Elf64_Half   /* Size of ELF header in bytes. */
        ('ELF_PHEntSize',            c_uint16),            # Elf64_Half   /* Size of program header entry. */
        ('ELF_PHNum',                c_uint16),            # Elf64_Half   /* Number of program header entries. */
        ('ELF_SHEntSize',            c_uint16),            # Elf64_Half   /* Size of section header entry. */
        ('ELF_SHNum',                c_uint16),            # Elf64_Half   /* Number of section header entries. */
        ('ELF_SNStr',                c_uint16),            # Elf64_Half   /* Section name strings section. */
    ]

# Section header.
class ELF_SECTION_HEADER32(Structure):
    _pack_ = 1
    _fields_ = [
        ('SH_Name',                  c_uint32),            # Elf32_Word   /* Section name (index into the section header string table). */
        ('SH_Type',                  c_uint32),            # Elf32_Word   /* Section type. */
        ('SH_Flags',                 c_uint32),            # Elf32_Word   /* Section flags. */
        ('SH_ADDR',                  c_uint32),            # Elf32_Addr   /* Address in memory image. */
        ('SH_Offset',                c_uint32),            # Elf32_Off    /* Offset in file. */
        ('SH_Size',                  c_uint32),            # Elf32_Word   /* Size in bytes. */
        ('SH_Link',                  c_uint32),            # Elf32_Word   /* Index of a related section. */
        ('SH_Info',                  c_uint32),            # Elf32_Word   /* Depends on section type. */
        ('SH_AddrAlign',             c_uint32),            # Elf32_Word   /* Alignment in bytes. */
        ('SH_EntSize',               c_uint32),            # Elf32_Word   /* Size of each entry in section. */
    ]

class ELF_SECTION_HEADER64(Structure):
    _pack_ = 1
    _fields_ = [
        ('SH_Name',                  c_uint32),            # Elf32_Word   /* Section name (index into the section header string table). */
        ('SH_Type',                  c_uint32),            # Elf32_Word   /* Section type. */
        ('SH_Flags',                 c_uint64),            # Elf32_XWord   /* Section flags. */
        ('SH_ADDR',                  c_uint64),            # Elf32_Addr   /* Address in memory image. */
        ('SH_Offset',                c_uint64),            # Elf32_Off    /* Offset in file. */
        ('SH_Size',                  c_uint64),            # Elf32_XWord   /* Size in bytes. */
        ('SH_Link',                  c_uint32),            # Elf32_Word   /* Index of a related section. */
        ('SH_Info',                  c_uint32),            # Elf32_Word   /* Depends on section type. */
        ('SH_AddrAlign',             c_uint64),            # Elf32_XWord   /* Alignment in bytes. */
        ('SH_EntSize',               c_uint64),            # Elf32_XWord   /* Size of each entry in section. */
    ]

# Program header.
class ELF_PROGRAM_HEADER32(Structure):
    _pack_ = 1
    _fields_ = [
        ('PH_Type',                  c_uint32),            # Elf32_Word   /* Entry type. */
        ('PH_Offset',                c_uint32),            # Elf32_Off    /* File offset of contents. */
        ('PH_VirAddr',               c_uint32),            # Elf32_Addr   /* Virtual address in memory image. */
        ('PH_PhyAddr',               c_uint32),            # Elf32_Addr   /* Physical address (not used). */
        ('PH_FileSize',              c_uint32),            # Elf32_Word   /* Size of contents in file. */
        ('PH_MemorySize',            c_uint32),            # Elf32_Word   /* Size of contents in memory. */
        ('PH_Flags',                 c_uint32),            # Elf32_Word   /* Access permission flags. */
        ('PH_Align',                 c_uint32),            # Elf32_Word   /* Alignment in memory and file. */
    ]

class ELF_PROGRAM_HEADER64(Structure):
    _pack_ = 1
    _fields_ = [
        ('PH_Type',                  c_uint32),            # Elf32_Word   /* Entry type. */
        ('PH_Flags',                 c_uint32),            # Elf32_Word   /* Access permission flags. */
        ('PH_Offset',                c_uint64),            # Elf32_Off    /* File offset of contents. */
        ('PH_VirAddr',               c_uint64),            # Elf32_Addr   /* Virtual address in memory image. */
        ('PH_PhyAddr',               c_uint64),            # Elf32_Addr   /* Physical address (not used). */
        ('PH_FileSize',              c_uint64),            # Elf32_XWord   /* Size of contents in file. */
        ('PH_MemorySize',            c_uint64),            # Elf32_XWord   /* Size of contents in memory. */
        ('PH_Align',                 c_uint64),            # Elf32_XWord   /* Alignment in memory and file. */
    ]

# Dynamic union.
class ELF_DYNAMIC_UNION(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_Dynamic_Val',          c_uint32),            # Elf32_Word   /* Integer value. */
        ('ELF_Dynamic_Ptr',          c_uint32),            # Elf32_Addr   /* Address value. */
    ]


# Dynamic structure. The ".dynamic" section contains an array of them.
class ELF_DYNAMIC_STRUCTURE(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_Dynamic_Tag',          c_int32),             # Elf32_Sword   /* Entry type. */
        ('ELF_Dynamic_Union',        ELF_DYNAMIC_UNION),   # Elf32_Off     /* Section type. */
    ]

## Relocation entries.

# /* Relocations that don't need an addend field. */
class ELF_RELOCATION(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_ReOffset',             c_uint32),            # Elf32_Addr   /* Location to be relocated. */
        ('ELF_ReInfo',               c_uint32),            # Elf32_Word   /* Relocation type and symbol index. */
    ]

# /* Relocations that need an addend field. */
class ELF_RELOCATION(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_ReOffset',             c_uint32),            # Elf32_Addr   /* Location to be relocated. */
        ('ELF_ReInfo',               c_uint32),            # Elf32_Word   /* Relocation type and symbol index. */
        ('ELF_ReAddend',             c_int32),             # Elf32_SWord  /* Addend. */
    ]

# Move Entry
class ELF_MOVE(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_MValue',               c_uint64),            # Elf32_Lword  /* symbol value */
        ('ELF_MInfo',                c_uint32),            # Elf32_Word   /* size + index */
        ('ELF_MPOffset',             c_int32),             # Elf32_Word   /* symbol offset */
        ('ELF_MRepeat',              c_uint16),            # Elf32_Half   /* repeat count */
        ('ELF_MStride',              c_uint16),            # Elf32_Half   /* stride info */
    ]

## Hardware/Software capabilities entry
class ELF_CAPA_UNION(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_Capa_Val',             c_uint32),            # Elf32_Word   /* Integer value. */
        ('ELF_Capa_Ptr',             c_uint32),            # Elf32_Addr   /* Address value. */
    ]

class ELF_CAPABILITY(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_Capa_Tag',             c_uint32),            # Elf32_Word   /* how to interpret value */
        ('ELF_Capa_Union',           ELF_CAPA_UNION),      # ELF_CAPA_UNION
    ]

# Symbol table entries.
class ELF_SYMBOL(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_ST_Name',              c_uint32),            # Elf32_Word   /* String table index of name. */
        ('ELF_ST_Value',             c_uint32),            # Elf32_Addr   /* Symbol value. */
        ('ELF_ST_Size',              c_uint32),            # Elf32_Word   /* Size of associated object. */
        ('ELF_ST_Info',              c_char),              # /* Type and binding information. */
        ('ELF_ST_Other',             c_char),              # /* Reserved (not used). */
        ('ELF_ST_Shndx',             c_uint16),            # Elf32_Half   /* Section index of symbol. */
    ]

# Structures used by Sun & GNU symbol versioning.
class ELF_VERDEF(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_VD_Version',           c_uint16),            # Elf32_Half
        ('ELF_VD_Flags',             c_uint16),            # Elf32_Half
        ('ELF_VD_Ndx',               c_uint16),            # Elf32_Half
        ('ELF_VD_Cnt',               c_uint16),            # Elf32_Half
        ('ELF_VD_Hash',              c_uint32),            # Elf32_Word
        ('ELF_VD_Aux',               c_uint32),            # Elf32_Word
        ('ELF_VD_Next',              c_uint32),            # Elf32_Word
    ]

class ELF_VERDAUX(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_VDA_Name',             c_uint32),            # Elf32_Word
        ('ELF_VDA_Next',             c_uint32),            # Elf32_Word
    ]

class ELF_VERNEED(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_VN_Version',           c_uint16),            # Elf32_Half
        ('ELF_VN_Cnt',               c_uint16),            # Elf32_Half
        ('ELF_VN_File',              c_uint32),            # Elf32_Word
        ('ELF_VN_Aux',               c_uint32),            # Elf32_Word
        ('ELF_VN_Next',              c_uint32),            # Elf32_Word
    ]

class ELF_VERNAUX(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_VNA_Hash',             c_uint32),            # Elf32_Word
        ('ELF_VNA_Flags',            c_uint16),            # Elf32_Half
        ('ELF_VNA_Other',            c_uint16),            # Elf32_Half
        ('ELF_VNA_Name',             c_uint32),            # Elf32_Word
        ('ELF_VNA_Next',             c_uint32),            # Elf32_Word
    ]

class ELF_SYMINFO(Structure):
    _pack_ = 1
    _fields_ = [
        ('ELF_SI_BoundTo',           c_uint16),            # Elf32_Half   /* direct bindings - symbol bound to */
        ('ELF_SI_Flags',             c_uint16),            # Elf32_Half   /* per symbol flags */
    ]

class UNIVERSAL_PAYLOAD_INFO(Structure):
    _pack_ = 1
    _fields_ = [
        ('Identifier',               c_uint32),            # ?PLDH? Identifier for the unverial payload info. 0x504c4448
        ('HeaderLength',             c_uint32),            # Length of the structure in bytes.
        ('SpecRevision',             c_uint16),            # Indicates compliance with a revision of this specification in the BCD format. 7 : 0 - Minor Version / 15 : 8 - Major Version For revision v0.75 the value will be 0x0075.
        ('Reserved',                 c_uint16),            # Reserved for future use.
        ('Revision',                 c_uint32),            # Revision of the Payload binary. Major.Minor .Revision.Build . The ImageRevision can be decoded as follows: 7 : 0  - Build Number / 15 :8  - Revision / 23 :16 - Minor Version / 31 :24 - Major Version
        ('Attribute',                c_uint32),            # Length of the structure in bytes.
        ('Capability',               c_uint32),            # Length of the structure in bytes.
        ('ProducerId',               ARRAY(c_uint8, 16)),  # Length of the structure in bytes.
        ('ImageId',                  ARRAY(c_uint8, 16)),  # Length of the structure in bytes.
    ]
