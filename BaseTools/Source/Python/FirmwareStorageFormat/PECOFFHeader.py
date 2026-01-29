## @file
# This file is used to define the PECOFF Header C Struct.
#
# Copyright (c) 2023-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from struct import *
from ctypes import *
from FirmwareStorageFormat.Common import *

## Signature
EFI_IMAGE_DOS_SIGNATURE     = 0x5A4D      # MZ
EFI_IMAGE_OS2_SIGNATURE     = 0x454E      # NE
EFI_IMAGE_OS2_SIGNATURE_LE  = 0x454C      # LE
EFI_IMAGE_NT_SIGNATURE      = 0x00004550  # PE00
EFI_IMAGE_EDOS_SIGNATURE    = 0x44454550  # PEED
EFI_IMAGE_TE_SIGNATURE      = 0x5A56      # VZ


## PE32+ Subsystem type for EFI images
## Windows Subsystem
EFI_IMAGE_SUBSYSTEM_UNKNOWN                  = 0   # An unknown subsystem
EFI_IMAGE_SUBSYSTEM_NATIVE                   = 1   # Device drivers and native Windows processes
EFI_IMAGE_SUBSYSTEM_WINDOWS_GUI              = 2   # The Windows graphical user interface (GUI) subsystem
EFI_IMAGE_SUBSYSTEM_WINDOWS_CUI              = 3   # The Windows character subsystem
EFI_IMAGE_SUBSYSTEM_OS2_CUI                  = 5   # The OS/2 character subsystem
EFI_IMAGE_SUBSYSTEM_POSIX_CUI                = 7   # The Posix character subsystem
EFI_IMAGE_SUBSYSTEM_NATIVE_WINDOWS           = 8   # Native Win9x driver
EFI_IMAGE_SUBSYSTEM_WINDOWS_CE_GUI           = 9   # Windows CE
EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION          = 10  # An Extensible Firmware Interface (EFI) application
EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER  = 11  # An EFI driver with boot services
EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER       = 12  # An EFI driver with run-time services
EFI_IMAGE_SUBSYSTEM_EFI_ROM                  = 13  # An EFI ROM image
EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER       = 13
EFI_IMAGE_SUBSYSTEM_XBOX                     = 14  # XBOX
EFI_IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION = 16  # Windows boot application.

## Machine Types

EFI_IMAGE_FILE_MACHINE_UNKNOWN     = 0x0     # The content of this field is assumed to be applicable to any machine type
EFI_IMAGE_FILE_MACHINE_AM33        = 0x01d3  # Matsushita AM33
EFI_IMAGE_FILE_MACHINE_I386        = 0x014c  # Intel 386 or later processors and compatible processors
EFI_IMAGE_FILE_MACHINE_EBC         = 0x0EBC  # EFI byte code
EFI_IMAGE_FILE_MACHINE_X64         = 0x8664  # IMAGE_FILE_MACHINE_AMD64, x64
EFI_IMAGE_FILE_MACHINE_ARM         = 0x01c0  # Thumb only, ARM little endian
EFI_IMAGE_FILE_MACHINE_ARMT        = 0x01c2  # 32bit Mixed ARM and Thumb/Thumb 2  Little Endian -- IMAGE_FILE_MACHINE_THUMB
EFI_IMAGE_FILE_MACHINE_ARM64       = 0xAA64  # 64bit ARM Architecture, Little Endian
EFI_IMAGE_FILE_MACHINE_RISCV32     = 0x5032  # RISC-V 32-bit address space
EFI_IMAGE_FILE_MACHINE_RISCV64     = 0x5064  # 64bit RISC-V ISA / RISC-V 64-bit address space
EFI_IMAGE_FILE_MACHINE_RISCV128    = 0x5128  # RISC-V 128-bit address space
EFI_IMAGE_FILE_MACHINE_ARMNT       = 0x01c4  # ARM Thumb-2 little endian
EFI_IMAGE_FILE_MACHINE_IA64        = 0x200   # Intel Itanium processor family
EFI_IMAGE_FILE_MACHINE_LOONGARCH32 = 0x6232 # LoongArch 32-bit processor family
EFI_IMAGE_FILE_MACHINE_LOONGARCH64 = 0x6264 # LoongArch 64-bit processor family
EFI_IMAGE_FILE_MACHINE_M32R        = 0x9041  # Mitsubishi M32R little endian
EFI_IMAGE_FILE_MACHINE_MIPS16      = 0x266   # MIPS16
EFI_IMAGE_FILE_MACHINE_MIPSFPU     = 0x366   # MIPS with FPU
EFI_IMAGE_FILE_MACHINE_MIPSFPU16   = 0x466  # MIPS16 with FPU
EFI_IMAGE_FILE_MACHINE_POWERPC     = 0x1f0   # Power PC little endian
EFI_IMAGE_FILE_MACHINE_POWERPCFP   = 0x1f1  # Power PC with floating point support
EFI_IMAGE_FILE_MACHINE_R4000       = 0x166   # MIPS little endian
EFI_IMAGE_FILE_MACHINE_SH3         = 0x1a2   # Hitachi SH3
EFI_IMAGE_FILE_MACHINE_SH3DSP      = 0x1a3   # Hitachi SH3 DSP
EFI_IMAGE_FILE_MACHINE_SH4         = 0x1a6   # Hitachi SH4
EFI_IMAGE_FILE_MACHINE_SH5         = 0x1a8   # Hitachi SH5
EFI_IMAGE_FILE_MACHINE_WCEMIPSV2   = 0x169  # MIPS little-endian WCE v2


class EFI_IMAGE_DOS_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('e_magic',                 c_uint16),           # Magic number
        ('e_cblp',                  c_uint16),           # Bytes on last page of file
        ('e_cp',                    c_uint16),           # Pages in file
        ('e_crlc',                  c_uint16),           # Relocations
        ('e_cparhdr',               c_uint16),           # Size of header in paragraphs
        ('e_minalloc',              c_uint16),           # Minimum extra paragraphs needed
        ('e_maxalloc',              c_uint16),           # Maximum extra paragraphs needed
        ('e_ss',                    c_uint16),           # Initial (relative) SS value
        ('e_sp',                    c_uint16),           # Initial SP value
        ('e_csum',                  c_uint16),           # Checksum
        ('e_ip',                    c_uint16),           # Initial IP value
        ('e_cs',                    c_uint16),           # Initial (relative) CS value
        ('e_lfarlc',                c_uint16),           # File address of relocation table    520e   0x70
        ('e_ovno',                  c_uint16),           # Overlay number
        ('e_res',                   ARRAY(c_uint16, 4)), # Reserved words
        ('e_oemid',                 c_uint16),           # OEM identifier (for e_oeminfo)
        ('e_oeminfo',               c_uint16),           # OEM information; e_oemid specific
        ('e_res2',                  ARRAY(c_uint16, 10)),   # Reserved words
        ('e_lfanew',                c_uint32),           # File address of new exe header    02a0
    ]


class EFI_IMAGE_FILE_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Machine',                  c_uint16),            # The number that identifies the type of target machine.
        ('NumberOfSections',         c_uint16),            # The number of sections. This indicates the size of the section table, which immediately follows the headers.
        ('TimeDateStamp',            c_uint32),            # The low 32 bits of the number of seconds since 00:00 January 1, 1970 (a C run-time time_t value), which indicates when the file was created.
        ('PointerToSymbolTable',     c_uint32),            # The file offset of the COFF symbol table, or zero if no COFF symbol table is present. This value should be zero for an image because COFF debugging information is deprecated.
        ('NumberOfSymbols',          c_uint32),            # The number of entries in the symbol table. This data can be used to locate the string table, which immediately follows the symbol table. This value should be zero for an image because COFF debugging information is deprecated.
        ('SizeOfOptionalHeader',     c_uint16),            # The size of the optional header, which is required for executable files but not for object files. This value should be zero for an object file.
        ('Characteristics',          c_uint16),            # The flags that indicate the attributes of the file.
    ]

EFI_IMAGE_SIZEOF_FILE_HEADER       = 20

## Characteristics
EFI_IMAGE_FILE_RELOCS_STRIPPED          = 0x0001 # Image only, Windows CE, and Microsoft Windows NT and later.
                                             # This indicates that the file does not contain base relocations and must therefore be loaded at its preferred base address.
                                             # If the base address is not available, the loader reports an error. The default behavior of the linker is to strip base relocations from executable (EXE) files.
EFI_IMAGE_FILE_EXECUTABLE_IMAGE         = 0x0002 # Image only. This indicates that the image file is valid and can be run. If this flag is not set, it indicates a linker error.
EFI_IMAGE_FILE_LINE_NUMS_STRIPPED       = 0x0004 # COFF line numbers have been removed. This flag is deprecated and should be zero.
EFI_IMAGE_FILE_LOCAL_SYMS_STRIPPED      = 0x0008 # COFF symbol table entries for local symbols have been removed. This flag is deprecated and should be zero.
EFI_IMAGE_FILE_AGGRESSIVE_WS_TRIM       = 0x0010 # Obsolete. Aggressively trim working set. This flag is deprecated for Windows 2000 and later and must be zero.
EFI_IMAGE_FILE_LARGE_ADDRESS_AWARE      = 0x0020 # Application can handle > 2-GB addresses.
# 0x0040 flag is reserved for future use
EFI_IMAGE_FILE_BYTES_REVERSED_LO        = 0x0080 # Little endian: the least significant bit (LSB) precedes the most significant bit (MSB) in memory. This flag is deprecated and should be zero.
EFI_IMAGE_FILE_32BIT_MACHINE            = 0x0100 # Machine is based on a 32-bit-word architecture.
EFI_IMAGE_FILE_DEBUG_STRIPPED           = 0x0200 # Debugging information is removed from the image file.
EFI_IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP  = 0x0400 # If the image is on removable media, fully load it and copy it to the swap file.
EFI_IMAGE_FILE_NET_RUN_FROM_SWAP        = 0x0800 # If the image is on network media, fully load it and copy it to the swap file.
EFI_IMAGE_FILE_SYSTEM                   = 0x1000 # The image file is a system file, not a user program.
EFI_IMAGE_FILE_DLL                      = 0x2000 # The image file is a dynamic-link library (DLL). Such files are considered executable files for almost all purposes, although they cannot be directly run.
EFI_IMAGE_FILE_UP_SYSTEM_ONLY           = 0x4000 # The file should be run only on a uniprocessor machine.
EFI_IMAGE_FILE_BYTES_REVERSED_HI        = 0x8000 # Big endian: the MSB precedes the LSB in memory. This flag is deprecated and should be zero.


# EFI_IMAGE_DATA_DIRECTORY.
class EFI_IMAGE_DATA_DIRECTORY(Structure):
    _pack_ = 1
    _fields_ = [
        ('VirtualAddress',           c_uint32),
        ('Size',                     c_uint32),
    ]

EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES = 16


# image ROM Option Header
class EFI_IMAGE_ROM_OPTION_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Magic',                   c_uint16),           # The unsigned integer that identifies the state of the image file. The most common number is 0x10B, which identifies it as a normal executable file. 0x107 identifies it as a ROM image, and 0x20B identifies it as a PE32+ executable.
        ('MajorLinkerVersion',      c_uint8),            # The linker major version number.
        ('MinorLinkerVersion',      c_uint8),            # The linker minor version number.
        ('SizeOfCode',              c_uint32),           # The size of the code (text) section, or the sum of all code sections if there are multiple sections.
        ('SizeOfInitializedData',   c_uint32),           # The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
        ('SizeOfUninitializedData', c_uint32),           # The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
        ('AddressOfEntryPoint',     c_uint32),           # The address of the entry point relative to the image base when the executable file is loaded into memory. For program images, this is the starting address.
                                                         # For device drivers, this is the address of the initialization function. An entry point is optional for DLLs.
                                                         # When no entry point is present, this field must be zero.
        ('BaseOfCode',              c_uint32),           # The address that is relative to the image base of the beginning-of-code section when it is loaded into memory.
        ('BaseOfData',              c_uint32),           # The address that is relative to the image base of the beginning-of-data section when it is loaded into memory.
        ('BaseOfBss',               c_uint32),           # The address that is relative to the image base of the beginning-of-code section when it is loaded into memory.
        ('GprMask',                 c_uint32),           # The address that is relative to the image base of the beginning-of-data section when it is loaded into memory.
        ('CprMask',                 ARRAY(c_uint32, 4)),           # The address that is relative to the image base of the beginning-of-code section when it is loaded into memory.
        ('GpValue',                 c_uint32),           # The address that is relative to the image base of the beginning-of-data section when it is loaded into memory.
    ]

EFI_IMAGE_ROM_OPTIONAL_HDR_MAGIC      = 0x107
EFI_IMAGE_SIZEOF_ROM_OPTIONAL_HEADER  = 0x38


class EFI_IMAGE_ROM_HEADERS(Structure):
    _pack_ = 1
    _fields_ = [
        ('FileHeader',              EFI_IMAGE_FILE_HEADER),           # The unsigned integer that identifies the state of the image file. The most common number is 0x10B, which identifies it as a normal executable file. 0x107 identifies it as a ROM image, and 0x20B identifies it as a PE32+ executable.
        ('OptionalHeader',            EFI_IMAGE_ROM_OPTION_HEADER),            # The linker major version number.
    ]



EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b

class EFI_IMAGE_OPTIONAL_HEADER32(Structure):
    _pack_ = 1
    _fields_ = [
        # Standard fields
        ('Magic',                   c_uint16),           # The unsigned integer that identifies the state of the image file. The most common number is 0x10B, which identifies it as a normal executable file. 0x107 identifies it as a ROM image, and 0x20B identifies it as a PE32+ executable.
        ('MajorLinkerVersion',      c_uint8),            # The linker major version number.
        ('MinorLinkerVersion',      c_uint8),            # The linker minor version number.
        ('SizeOfCode',              c_uint32),           # The size of the code (text) section, or the sum of all code sections if there are multiple sections.
        ('SizeOfInitializedData',   c_uint32),           # The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
        ('SizeOfUninitializedData', c_uint32),           # The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
        ('AddressOfEntryPoint',     c_uint32),           # The address of the entry point relative to the image base when the executable file is loaded into memory. For program images, this is the starting address.
                                                         # For device drivers, this is the address of the initialization function. An entry point is optional for DLLs.
                                                         # When no entry point is present, this field must be zero.
        ('BaseOfCode',              c_uint32),           # The address that is relative to the image base of the beginning-of-code section when it is loaded into memory.
        ('BaseOfData',              c_uint32),           # The address that is relative to the image base of the beginning-of-data section when it is loaded into memory.
        # NT additional fields
        ('ImageBase',               c_uint32),          # The preferred address of the first byte of image when loaded into memory; must be a multiple of 64 K.
                                                        # The default for DLLs is 0x10000000. The default for Windows CE EXEs is 0x00010000.
                                                        # The default for Windows NT, Windows 2000, Windows XP, Windows 95, Windows 98, and Windows Me is 0x00400000.
        ('SectionAlignment',        c_uint32),          # The alignment (in bytes) of sections when they are loaded into memory.
                                                        # It must be greater than or equal to FileAlignment. The default is the page size for the architecture.
        ('FileAlignment',           c_uint32),          # The alignment factor (in bytes) that is used to align the raw data of sections in the image file.
                                                        # The value should be a power of 2 between 512 and 64 K, inclusive. The default is 512.
                                                        # If the SectionAlignment is less than the architecture's page size, then FileAlignment must match SectionAlignment.
        ('MajorOperatingSystemVersion',  c_uint16),     # The major version number of the required operating system.
        ('MinorOperatingSystemVersion',  c_uint16),     # The minor version number of the required operating system.
        ('MajorImageVersion',       c_uint16),          # The major version number of the image.
        ('MinorImageVersion',       c_uint16),          # The minor version number of the image.
        ('MajorSubsystemVersion',   c_uint16),          # The major version number of the subsystem.
        ('MinorSubsystemVersion',   c_uint16),          # The minor version number of the subsystem.
        ('Win32VersionValue',       c_uint32),          # Reserved, must be zero.
        ('SizeOfImage',             c_uint32),          # The size (in bytes) of the image, including all headers, as the image is loaded in memory. It must be a multiple of SectionAlignment.
        ('SizeOfHeaders',           c_uint32),          # The combined size of an MS-DOS stub, PE header, and section headers rounded up to a multiple of FileAlignment.
        ('CheckSum',                c_uint32),          # The image file checksum. The algorithm for computing the checksum is incorporated into IMAGHELP.DLL.
                                                        # The following are checked for validation at load time: all drivers, any DLL loaded at boot time, and any DLL that is loaded into a critical Windows process.
        ('Subsystem',               c_uint16),          # The subsystem that is required to run this image.
        ('DllCharacteristics',      c_uint16),
        ('SizeOfStackReserve',      c_uint32),          # The size of the stack to reserve. Only SizeOfStackCommit is committed; the rest is made available one page at a time until the reserve size is reached.
        ('SizeOfStackCommit',       c_uint32),          # The size of the stack to commit.
        ('SizeOfHeapReserve',       c_uint32),          # The size of the local heap space to reserve. Only SizeOfHeapCommit is committed; the rest is made available one page at a time until the reserve size is reached.
        ('SizeOfHeapCommit',        c_uint32),          # The size of the local heap space to commit.
        ('LoaderFlags',             c_uint32),          # Reserved, must be zero.
        ('NumberOfRvaAndSizes',     c_uint32),          # The number of data-directory entries in the remainder of the optional header. Each describes a location and size.
        ('DataDirectory',           ARRAY(EFI_IMAGE_DATA_DIRECTORY, EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES)),
    ]


EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20b

class EFI_IMAGE_OPTIONAL_HEADER64(Structure):
    _pack_ = 1
    _fields_ = [
        # Standard fields
        ('Magic',                   c_uint16),           # The unsigned integer that identifies the state of the image file. The most common number is 0x10B, which identifies it as a normal executable file. 0x107 identifies it as a ROM image, and 0x20B identifies it as a PE32+ executable.
        ('MajorLinkerVersion',      c_uint8),            # The linker major version number.
        ('MinorLinkerVersion',      c_uint8),            # The linker minor version number.
        ('SizeOfCode',              c_uint32),           # The size of the code (text) section, or the sum of all code sections if there are multiple sections.
        ('SizeOfInitializedData',   c_uint32),           # The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
        ('SizeOfUninitializedData', c_uint32),           # The size of the uninitialized data section (BSS), or the sum of all such sections if there are multiple BSS sections.
        ('AddressOfEntryPoint',     c_uint32),           # The address of the entry point relative to the image base when the executable file is loaded into memory. For program images, this is the starting address.
                                                         # For device drivers, this is the address of the initialization function. An entry point is optional for DLLs.
                                                         # When no entry point is present, this field must be zero.
        ('BaseOfCode',              c_uint32),           # The address that is relative to the image base of the beginning-of-code section when it is loaded into memory.
        # NT additional fields
        ('ImageBase',               c_uint64),          # The preferred address of the first byte of image when loaded into memory; must be a multiple of 64 K.
                                                        # The default for DLLs is 0x10000000. The default for Windows CE EXEs is 0x00010000.
                                                        # The default for Windows NT, Windows 2000, Windows XP, Windows 95, Windows 98, and Windows Me is 0x00400000.
        ('SectionAlignment',        c_uint32),          # The alignment (in bytes) of sections when they are loaded into memory.
                                                        # It must be greater than or equal to FileAlignment. The default is the page size for the architecture.
        ('FileAlignment',           c_uint32),          # The alignment factor (in bytes) that is used to align the raw data of sections in the image file.
                                                        # The value should be a power of 2 between 512 and 64 K, inclusive. The default is 512.
                                                        # If the SectionAlignment is less than the architecture's page size, then FileAlignment must match SectionAlignment.
        ('MajorOperatingSystemVersion',  c_uint16),     # The major version number of the required operating system.
        ('MinorOperatingSystemVersion',  c_uint16),     # The minor version number of the required operating system.
        ('MajorImageVersion',       c_uint16),          # The major version number of the image.
        ('MinorImageVersion',       c_uint16),          # The minor version number of the image.
        ('MajorSubsystemVersion',   c_uint16),          # The major version number of the subsystem.
        ('MinorSubsystemVersion',   c_uint16),          # The minor version number of the subsystem.
        ('Win32VersionValue',       c_uint32),          # Reserved, must be zero.
        ('SizeOfImage',             c_uint32),          # The size (in bytes) of the image, including all headers, as the image is loaded in memory. It must be a multiple of SectionAlignment.
        ('SizeOfHeaders',           c_uint32),          # The combined size of an MS-DOS stub, PE header, and section headers rounded up to a multiple of FileAlignment.
        ('CheckSum',                c_uint32),          # The image file checksum. The algorithm for computing the checksum is incorporated into IMAGHELP.DLL.
                                                        # The following are checked for validation at load time: all drivers, any DLL loaded at boot time, and any DLL that is loaded into a critical Windows process.
        ('Subsystem',               c_uint16),          # The subsystem that is required to run this image.
        ('DllCharacteristics',      c_uint16),
        ('SizeOfStackReserve',      c_uint64),          # The size of the stack to reserve. Only SizeOfStackCommit is committed; the rest is made available one page at a time until the reserve size is reached.
        ('SizeOfStackCommit',       c_uint64),          # The size of the stack to commit.
        ('SizeOfHeapReserve',       c_uint64),          # The size of the local heap space to reserve. Only SizeOfHeapCommit is committed; the rest is made available one page at a time until the reserve size is reached.
        ('SizeOfHeapCommit',        c_uint64),          # The size of the local heap space to commit.
        ('LoaderFlags',             c_uint32),          # Reserved, must be zero.
        ('NumberOfRvaAndSizes',     c_uint32),          # The number of data-directory entries in the remainder of the optional header. Each describes a location and size.
        ('DataDirectory',           ARRAY(EFI_IMAGE_DATA_DIRECTORY, EFI_IMAGE_NUMBER_OF_DIRECTORY_ENTRIES)),
    ]


## attension
# EFI_IMAGE_NT_HEADERS32 and EFI_IMAGE_HEADERS64 are for use ONLY
# by tools.  All proper EFI code MUST use EFI_IMAGE_NT_HEADERS ONLY!!!
#

class EFI_IMAGE_NT_HEADERS32(Structure):
    _pack_ = 1
    _fields_ = [
        ('Signature',               c_uint32),   # Size: 4
        ('FileHeader',              EFI_IMAGE_FILE_HEADER),  # Size: 20
        ('OptionalHeader',          EFI_IMAGE_OPTIONAL_HEADER32), # Size: 224
    ]

EFI_IMAGE_SIZEOF_NT_OPTIONAL32_HEADER = 0xf8

class EFI_IMAGE_NT_HEADERS64(Structure):
    _pack_ = 1
    _fields_ = [
        ('Signature',               c_uint32),    # Size: 4
        ('FileHeader',              EFI_IMAGE_FILE_HEADER),  # Size: 20
        ('OptionalHeader',          EFI_IMAGE_OPTIONAL_HEADER64), # Size: 240
    ]

EFI_IMAGE_SIZEOF_NT_OPTIONAL64_HEADER = 0x108


## Directory Entries

EFI_IMAGE_DIRECTORY_ENTRY_EXPORT       = 0
EFI_IMAGE_DIRECTORY_ENTRY_IMPORT       = 1
EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE     = 2
EFI_IMAGE_DIRECTORY_ENTRY_EXCEPTION    = 3
EFI_IMAGE_DIRECTORY_ENTRY_SECURITY     = 4
EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC    = 5
EFI_IMAGE_DIRECTORY_ENTRY_DEBUG        = 6
EFI_IMAGE_DIRECTORY_ENTRY_COPYRIGHT    = 7
EFI_IMAGE_DIRECTORY_ENTRY_GLOBALPTR    = 8
EFI_IMAGE_DIRECTORY_ENTRY_TLS          = 9
EFI_IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG  = 10


class EFI_IMAGE_SECTION_HEADER_MISC(Union):
    _pack_ = 1
    _fields_ = [
        ('PhysicalAddress',         c_uint32),
        ('VirtualSize',             c_uint32),            # The total size of the section when loaded into memory. If this value is greater than SizeOfRawData, the section is zero-padded.
                                                          # This field is valid only for executable images and should be set to zero for object files.
    ]

EFI_IMAGE_SIZEOF_SHORT_NAME = 8

class EFI_IMAGE_SECTION_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Name',                  ARRAY(c_uint8, EFI_IMAGE_SIZEOF_SHORT_NAME)),             # An 8-byte, null-padded UTF-8 encoded string. If the string is exactly 8 characters long, there is no terminating null.
                                                                                            # For longer names, this field contains a slash (/) that is followed by an ASCII representation of a decimal number that is an offset into the string table.
                                                                                            # Executable images do not use a string table and do not support section names longer than 8 characters.
                                                                                            # Long names in object files are truncated if they are emitted to an executable file.
        ('Misc',                  EFI_IMAGE_SECTION_HEADER_MISC),           # The total size of the section when loaded into memory. If this value is greater than SizeOfRawData, the section is zero-padded.
                                                                            # This field is valid only for executable images and should be set to zero for object files.
        ('VirtualAddress',        c_uint32),            # For executable images, the address of the first byte of the section relative to the image base when the section is loaded into memory.
                                                        # For object files, this field is the address of the first byte before relocation is applied; for simplicity, compilers should set this to zero.
                                                        # Otherwise, it is an arbitrary value that is subtracted from offsets during relocation.
        ('SizeOfRawData',         c_uint32),            # The size of the section (for object files) or the size of the initialized data on disk (for image files).
                                                        # For executable images, this must be a multiple of FileAlignment from the optional header.
                                                        # If this is less than VirtualSize, the remainder of the section is zero-filled.
                                                        # Because the SizeOfRawData field is rounded but the VirtualSize field is not, it is possible for SizeOfRawData to be greater than VirtualSize as well.
                                                        # When a section contains only uninitialized data, this field should be zero.
        ('PointerToRawData',      c_uint32),            # The file pointer to the first page of the section within the COFF file. For executable images, this must be a multiple of FileAlignment from the optional header.
                                                        # For object files, the value should be aligned on a 4-byte boundary for best performance.
                                                        # When a section contains only uninitialized data, this field should be zero.
        ('PointerToRelocations',  c_uint32),            # The file pointer to the beginning of relocation entries for the section. This is set to zero for executable images or if there are no relocations.
        ('PointerToLinenumbers',  c_uint32),            # The file pointer to the beginning of line-number entries for the section. This is set to zero if there are no COFF line numbers.
                                                        # This value should be zero for an image because COFF debugging information is deprecated.
        ('NumberOfRelocations',   c_uint16),            # The number of relocation entries for the section. This is set to zero for executable images.
        ('NumberOfLinenumbers',   c_uint16),            # The number of line-number entries for the section. This value should be zero for an image because COFF debugging information is deprecated.
        ('Characteristics',       c_uint32),            # The flags that describe the characteristics of the section.   See Section Flags
    ]

EFI_IMAGE_SIZEOF_SECTION_HEADER = 40

## Section Flags
# 0x00000000 / 0x00000001 / 0x00000002 / 0x00000004 / 0x00000010 / 0x00000400 --- Reserved, must be zero
EFI_IMAGE_SCN_TYPE_NO_PAD             = 0x00000008  # The section should not be padded to the next boundary. This flag is obsolete and is replaced by IMAGE_SCN_ALIGN_1BYTES. This is valid only for object files.
EFI_IMAGE_SCN_CNT_CODE                = 0x00000020  # The section contains executable code.
EFI_IMAGE_SCN_CNT_INITIALIZED_DATA    = 0x00000040  # The section contains initialized data.
EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA  = 0x00000080  # The section contains uninitialized data.
EFI_IMAGE_SCN_LNK_OTHER               = 0x00000100  # Reserved for future use.
EFI_IMAGE_SCN_LNK_INFO                = 0x00000100  # The section contains comments or other information. The .drectve section has this type. This is valid for object files only.
EFI_IMAGE_SCN_LNK_REMOVE              = 0x00000800  # The section will not become part of the image. This is valid only for object files.
EFI_IMAGE_SCN_LNK_COMDAT              = 0x00001000  # The section contains COMDAT data. For more information, see COMDAT Sections (Object Only). This is valid only for object files.
EFI_IMAGE_SCN_GPREL                   = 0x00008000  # The section contains data referenced through the global pointer (GP).
EFI_IMAGE_SCN_MEM_PURGEABLE           = 0x00010000  # Reserved for future use.
EFI_IMAGE_SCN_MEM_16BIT               = 0x00020000  # Reserved for future use.
EFI_IMAGE_SCN_MEM_LOCKED              = 0x00040000  # Reserved for future use.
EFI_IMAGE_SCN_MEM_PRELOAD             = 0x00080000  # Reserved for future use.
EFI_IMAGE_SCN_ALIGN_1BYTES            = 0x00100000  # Align data on a 1-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_2BYTES            = 0x00200000  # Align data on a 2-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_4BYTES            = 0x00300000  # Align data on a 4-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_8BYTES            = 0x00400000  # Align data on an 8-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_16BYTES           = 0x00500000  # Align data on a 16-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_32BYTES           = 0x00600000  # Align data on a 32-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_64BYTES           = 0x00700000  # Align data on a 64-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_128BYTES          = 0x00800000  # Align data on a 128-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_256BYTES          = 0x00900000  # Align data on a 256-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_512BYTES          = 0x00A00000  # Align data on a 512-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_1024BYTES         = 0x00B00000  # Align data on a 1024-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_2048BYTES         = 0x00C00000  # Align data on a 2048-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_4096BYTES         = 0x00D00000  # Align data on a 4096-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_ALIGN_8192BYTES         = 0x00E00000  # Align data on an 8192-byte boundary. Valid only for object files.
EFI_IMAGE_SCN_LNK_NRELOC_OVFL         = 0x01000000  # The section contains extended relocations.
EFI_IMAGE_SCN_MEM_DISCARDABLE         = 0x02000000  # The section can be discarded as needed.
EFI_IMAGE_SCN_MEM_NOT_CACHED          = 0x04000000  # The section cannot be cached.
EFI_IMAGE_SCN_MEM_NOT_PAGED           = 0x08000000  # The section is not pageable.
EFI_IMAGE_SCN_MEM_SHARED              = 0x10000000  # The section can be shared in memory.
EFI_IMAGE_SCN_MEM_EXECUTE             = 0x20000000  # The section can be executed as code.
EFI_IMAGE_SCN_MEM_READ                = 0x40000000  # The section can be read.
EFI_IMAGE_SCN_MEM_WRITE               = 0x80000000  # The section can be written to.


EFI_IMAGE_SIZEOF_SYMBOL = 18


## Section Nuber Values:  the Section Value field in a symbol table entry is a one-based index into the section table. However, this field is a signed integer and can take negative values.
EFI_IMAGE_SYM_UNDEFINED    = 0    # The symbol record is not yet assigned a section. A value of zero indicates that a reference to an external symbol is defined elsewhere.
                              # A value of non-zero is a common symbol with a size that is specified by the value.
EFI_IMAGE_SYM_ABSOLUTE     = -1   # The symbol has an absolute (non-relocatable) value and is not an address.
EFI_IMAGE_SYM_DEBUG        = -2   # The symbol provides general type or debugging information but does not correspond to a section. Microsoft tools use this setting along with .file records (storage class FILE).

## Type Representation: Type (fundamental) values.
EFI_IMAGE_SYM_TYPE_NULL    = 0  # No type information or unknown base type. Microsoft tools use this setting
EFI_IMAGE_SYM_TYPE_VOID    = 1  # No valid type; used with void pointers and functions
EFI_IMAGE_SYM_TYPE_CHAR    = 2  # A character (signed byte)
EFI_IMAGE_SYM_TYPE_SHORT   = 3  # A 2-byte signed integer
EFI_IMAGE_SYM_TYPE_INT     = 4  # A natural integer type (normally 4 bytes in Windows)
EFI_IMAGE_SYM_TYPE_LONG    = 5  # A 4-byte signed integer
EFI_IMAGE_SYM_TYPE_FLOAT   = 6  # A 4-byte floating-point number
EFI_IMAGE_SYM_TYPE_DOUBLE  = 7  # An 8-byte floating-point number
EFI_IMAGE_SYM_TYPE_STRUCT  = 8  # A structure
EFI_IMAGE_SYM_TYPE_UNION   = 9  # A union
EFI_IMAGE_SYM_TYPE_ENUM    = 10 # An enumerated type
EFI_IMAGE_SYM_TYPE_MOE     = 11 # A member of enumeration (a specific value)
EFI_IMAGE_SYM_TYPE_BYTE    = 12 # A byte; unsigned 1-byte integer
EFI_IMAGE_SYM_TYPE_WORD    = 13 # A word; unsigned 2-byte integer
EFI_IMAGE_SYM_TYPE_UINT    = 14 # An unsigned integer of natural size (normally, 4 bytes)
EFI_IMAGE_SYM_TYPE_DWORD   = 15 # An unsigned 4-byte integer
## Type Representation: Type (derived) values.
EFI_IMAGE_SYM_DTYPE_NULL      = 0 # No derived type; the symbol is a simple scalar variable.
EFI_IMAGE_SYM_DTYPE_POINTER   = 1 # The symbol is a pointer to base type.
EFI_IMAGE_SYM_DTYPE_FUNCTION  = 2 # The symbol is a function that returns a base type.
EFI_IMAGE_SYM_DTYPE_ARRAY     = 3 # The symbol is an array of base type.

## Storage classes.
# EFI_IMAGE_SYM_CLASS_END_OF_FUNCTION   = (UINT8) -1
EFI_IMAGE_SYM_CLASS_NULL              = 0
EFI_IMAGE_SYM_CLASS_AUTOMATIC         = 1
EFI_IMAGE_SYM_CLASS_EXTERNAL          = 2
EFI_IMAGE_SYM_CLASS_STATIC            = 3
EFI_IMAGE_SYM_CLASS_REGISTER          = 4
EFI_IMAGE_SYM_CLASS_EXTERNAL_DEF      = 5
EFI_IMAGE_SYM_CLASS_LABEL             = 6
EFI_IMAGE_SYM_CLASS_UNDEFINED_LABEL   = 7
EFI_IMAGE_SYM_CLASS_MEMBER_OF_STRUCT  = 8
EFI_IMAGE_SYM_CLASS_ARGUMENT          = 9
EFI_IMAGE_SYM_CLASS_STRUCT_TAG        = 10
EFI_IMAGE_SYM_CLASS_MEMBER_OF_UNION   = 11
EFI_IMAGE_SYM_CLASS_UNION_TAG         = 12
EFI_IMAGE_SYM_CLASS_TYPE_DEFINITION   = 13
EFI_IMAGE_SYM_CLASS_UNDEFINED_STATIC  = 14
EFI_IMAGE_SYM_CLASS_ENUM_TAG          = 15
EFI_IMAGE_SYM_CLASS_MEMBER_OF_ENUM    = 16
EFI_IMAGE_SYM_CLASS_REGISTER_PARAM    = 17
EFI_IMAGE_SYM_CLASS_BIT_FIELD         = 18
EFI_IMAGE_SYM_CLASS_BLOCK             = 100
EFI_IMAGE_SYM_CLASS_FUNCTION          = 101
EFI_IMAGE_SYM_CLASS_END_OF_STRUCT     = 102
EFI_IMAGE_SYM_CLASS_FILE              = 103
EFI_IMAGE_SYM_CLASS_SECTION           = 104
EFI_IMAGE_SYM_CLASS_WEAK_EXTERNAL     = 105


## type packing constants
EFI_IMAGE_N_BTMASK  = 17
EFI_IMAGE_N_TMASK   = 60
EFI_IMAGE_N_TMASK1  = 300
EFI_IMAGE_N_TMASK2  = 360
EFI_IMAGE_N_BTSHFT  = 4
EFI_IMAGE_N_TSHIFT  = 2


## Communal selection types.
EFI_IMAGE_COMDAT_SELECT_NODUPLICATES    = 1
EFI_IMAGE_COMDAT_SELECT_ANY             = 2
EFI_IMAGE_COMDAT_SELECT_SAME_SIZE       = 3
EFI_IMAGE_COMDAT_SELECT_EXACT_MATCH     = 4
EFI_IMAGE_COMDAT_SELECT_ASSOCIATIVE     = 5

EFI_IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY  = 1
EFI_IMAGE_WEAK_EXTERN_SEARCH_LIBRARY    = 2
EFI_IMAGE_WEAK_EXTERN_SEARCH_ALIAS      = 3

class EFI_IMAGE_RELOCATION(Structure):
    _pack_ = 1
    _fields_ = [
        ('VirtualAddress',             c_uint32),       # The address of the item to which relocation is applied.
                                                        # This is the offset from the beginning of the section, plus the value of the section's RVA/Offset field.
                                                        # See Section Table (Section Headers). For example, if the first byte of the section has an address of 0x10, the third byte has an address of 0x12.
        ('SymbolTableIndex',           c_uint32),       # A zero-based index into the symbol table. This symbol gives the address that is to be used for the relocation.
                                                        # If the specified symbol has section storage class, then the symbol's address is the address with the first section of the same name.
        ('Type',                       c_uint16),       # A value that indicates the kind of relocation that should be performed. Valid relocation types depend on machine type. See Type Indicators.
    ]

EFI_IMAGE_SIZEOF_RELOCATION = 10


## Relocation Type
# for X64 and compatible processors
EFI_IMAGE_REL_AMD64_ABSOLUTE      = 0x0000  # The relocation is ignored.
EFI_IMAGE_REL_AMD64_ADDR64        = 0x0001  # The 64-bit VA of the relocation target.
EFI_IMAGE_REL_AMD64_ADDR32        = 0x0002  # The 32-bit VA of the relocation target.
EFI_IMAGE_REL_AMD64_ADDR32NB      = 0x0003  # The 32-bit address without an image base (RVA).
EFI_IMAGE_REL_AMD64_REL32         = 0x0004  # The 32-bit relative address from the byte following the relocation.
EFI_IMAGE_REL_AMD64_REL32_1       = 0x0005  # The 32-bit address relative to byte distance 1 from the relocation.
EFI_IMAGE_REL_AMD64_REL32_2       = 0x0006  # The 32-bit address relative to byte distance 2 from the relocation.
EFI_IMAGE_REL_AMD64_REL32_3       = 0x0007  # The 32-bit address relative to byte distance 3 from the relocation.
EFI_IMAGE_REL_AMD64_REL32_4       = 0x0008  # The 32-bit address relative to byte distance 4 from the relocation.
EFI_IMAGE_REL_AMD64_REL32_5       = 0x0009  # The 32-bit address relative to byte distance 5 from the relocation.
EFI_IMAGE_REL_AMD64_SECTION       = 0x000A  # The 16-bit section index of the section that contains the target. This is used to support debugging information.
EFI_IMAGE_REL_AMD64_SECREL        = 0x000B  # The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
EFI_IMAGE_REL_AMD64_SECREL7       = 0x000C  # A 7-bit unsigned offset from the base of the section that contains the target.
EFI_IMAGE_REL_AMD64_TOKEN         = 0x000D  # CLR tokens.
EFI_IMAGE_REL_AMD64_SREL32        = 0x000E  # A 32-bit signed span-dependent value emitted into the object.
EFI_IMAGE_REL_AMD64_PAIR          = 0x000F  # A pair that must immediately follow every span-dependent value.
EFI_IMAGE_REL_AMD64_SSPAN32       = 0x0010  # A 32-bit signed span-dependent value that is applied at link time.
# for ARM Processors
EFI_IMAGE_REL_ARM_ABSOLUTE        = 0x0000  # The relocation is ignored.
EFI_IMAGE_REL_ARM_ADDR32          = 0x0001  # The 32-bit VA of the target.
EFI_IMAGE_REL_ARM_ADDR32NB        = 0x0002  # The 32-bit RVA of the target.
EFI_IMAGE_REL_ARM_BRANCH24        = 0x0003  # The 24-bit relative displacement to the target.
EFI_IMAGE_REL_ARM_BRANCH11        = 0x0004  # The reference to a subroutine call. The reference consists of two 16-bit instructions with 11-bit offsets.
EFI_IMAGE_REL_ARM_REL32           = 0x000A  # The 32-bit relative address from the byte following the relocation.
EFI_IMAGE_REL_ARM_SECTION         = 0x000E  # The 16-bit section index of the section that contains the target. This is used to support debugging information.
EFI_IMAGE_REL_ARM_SECREL          = 0x000F  # The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
EFI_IMAGE_REL_ARM_MOV32           = 0x0010  # The 32-bit VA of the target. This relocation is applied using a MOVW instruction for the low 16 bits followed by a MOVT for the high 16 bits.
EFI_IMAGE_REL_THUMB_MOV32         = 0x0011  # The 32-bit VA of the target. This relocation is applied using a MOVW instruction for the low 16 bits followed by a MOVT for the high 16 bits.
EFI_IMAGE_REL_THUMB_BRANCH20      = 0x0012  # The instruction is fixed up with the 21-bit relative displacement to the 2-byte aligned target.
                                        # The least significant bit of the displacement is always zero and is not stored. This relocation corresponds to a Thumb-2 32-bit conditional B instruction.
EFI_IMAGE_REL_THUMB_BRANCH24      = 0x0014  # The instruction is fixed up with the 25-bit relative displacement to the 2-byte aligned target.
                                        # The least significant bit of the displacement is zero and is not stored.This relocation corresponds to a Thumb-2 B instruction.
EFI_IMAGE_REL_THUMB_BLX23         = 0x0015  # The instruction is fixed up with the 25-bit relative displacement to the 4-byte aligned target. The low 2 bits of the displacement are zero and are not stored.
                                        # This relocation corresponds to a Thumb-2 BLX instruction.
EFI_IMAGE_REL_ARM_PAIR            = 0x0016  # The relocation is valid only when it immediately follows a ARM_REFHI or THUMB_REFHI.
                                        # Its SymbolTableIndex contains a displacement and not an index into the symbol table.
# for ARM64 Processors
EFI_IMAGE_REL_ARM64_ABSOLUTE         = 0x0000   # The relocation is ignored.
EFI_IMAGE_REL_ARM64_ADDR32           = 0x0001   # The 32-bit VA of the target.
EFI_IMAGE_REL_ARM64_ADDR32NB         = 0x0002   # The 32-bit RVA of the target.
EFI_IMAGE_REL_ARM64_BRANCH26         = 0x0003   # The 26-bit relative displacement to the target, for B and BL instructions.
EFI_IMAGE_REL_ARM64_PAGEBASE_REL21   = 0x0004   # The page base of the target, for ADRP instruction.
EFI_IMAGE_REL_ARM64_REL21            = 0x0005   # The 12-bit relative displacement to the target, for instruction ADR
EFI_IMAGE_REL_ARM64_PAGEOFFSET_12A   = 0x0006   # The 12-bit page offset of the target, for instructions ADD/ADDS (immediate) with zero shift.
EFI_IMAGE_REL_ARM64_PAGEOFFSET_12L   = 0x0007   # The 12-bit page offset of the target, for instruction LDR (indexed, unsigned immediate).
EFI_IMAGE_REL_ARM64_SECREL           = 0x0008   # The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
EFI_IMAGE_REL_ARM64_SECREL_LOW12A    = 0x0009   # Bit 0:11 of section offset of the target, for instructions ADD/ADDS (immediate) with zero shift.
EFI_IMAGE_REL_ARM64_SECREL_HIGH12A   = 0x000A   # Bit 12:23 of section offset of the target, for instructions ADD/ADDS (immediate) with zero shift.
EFI_IMAGE_REL_ARM64_SECREL_LOW12L    = 0x000B   # Bit 0:11 of section offset of the target, for instruction LDR (indexed, unsigned immediate).
EFI_IMAGE_REL_ARM64_TOKEN            = 0x000C   # CLR token.
EFI_IMAGE_REL_ARM64_SECTION          = 0x000D   # The 16-bit section index of the section that contains the target. This is used to support debugging information.
EFI_IMAGE_REL_ARM64_ADDR64           = 0x000E   # The 64-bit VA of the relocation target.
EFI_IMAGE_REL_ARM64_BRANCH19         = 0x000F   # The 19-bit offset to the relocation target, for conditional B instruction.
EFI_IMAGE_REL_ARM64_BRANCH14         = 0x0010   # The 14-bit offset to the relocation target, for instructions TBZ and TBNZ.
EFI_IMAGE_REL_ARM64_REL32            = 0x0011   # The 32-bit relative address from the byte following the relocation.

# for IBM PowerPC Processors
EFI_IMAGE_REL_PPC_ABSOLUTE        = 0x0000  # The relocation is ignored.
EFI_IMAGE_REL_PPC_ADDR64          = 0x0001  # The 64-bit VA of the target.
EFI_IMAGE_REL_PPC_ADDR32          = 0x0002  # The 32-bit VA of the target.
EFI_IMAGE_REL_PPC_ADDR24          = 0x0003  # The low 24 bits of the VA of the target. This is valid only when the target symbol is absolute and can be sign-extended to its original value.
EFI_IMAGE_REL_PPC_ADDR16          = 0x0004  # The low 16 bits of the target's VA.
EFI_IMAGE_REL_PPC_ADDR14          = 0x0005  # The low 14 bits of the target's VA. This is valid only when the target symbol is absolute and can be sign-extended to its original value.
EFI_IMAGE_REL_PPC_REL24           = 0x0006  # A 24-bit PC-relative offset to the symbol's location.
EFI_IMAGE_REL_PPC_REL14           = 0x0007  # A 14-bit PC-relative offset to the symbol's location.
EFI_IMAGE_REL_PPC_ADDR32NB        = 0x000A  # The 32-bit RVA of the target.
EFI_IMAGE_REL_PPC_SECREL          = 0x000B  # The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
EFI_IMAGE_REL_PPC_SECTION         = 0x000C  # The 16-bit section index of the section that contains the target. This is used to support debugging information.
EFI_IMAGE_REL_PPC_SECREL16        = 0x000F  # The 16-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
EFI_IMAGE_REL_PPC_REFHI           = 0x0010  # The high 16 bits of the target's 32-bit VA. This is used for the first instruction in a two-instruction sequence that loads a full address.
                                        # This relocation must be immediately followed by a PAIR relocation whose SymbolTableIndex contains a signed 16-bit displacement that is added to
                                        # the upper 16 bits that was taken from the location that is being relocated.
EFI_IMAGE_REL_PPC_REFLO           = 0x0011  # The low 16 bits of the target's VA.
EFI_IMAGE_REL_PPC_PAIR            = 0x0012  # A relocation that is valid only when it immediately follows a REFHI or SECRELHI relocation.
                                        # Its SymbolTableIndex contains a displacement and not an index into the symbol table.
EFI_IMAGE_REL_PPC_SECRELLO        = 0x0013  # The low 16 bits of the 32-bit offset of the target from the beginning of its section.
EFI_IMAGE_REL_PPC_GPREL           = 0x0015  # The 16-bit signed displacement of the target relative to the GP register.
EFI_IMAGE_REL_PPC_TOKEN           = 0x0016  # The CLR token.

# for Intel 386 Processors
EFI_IMAGE_REL_I386_ABSOLUTE       = 0x0000  # The relocation is ignored.
EFI_IMAGE_REL_I386_DIR16          = 0x0001  # Not supported.
EFI_IMAGE_REL_I386_REL16          = 0x0002  # Not supported.
EFI_IMAGE_REL_I386_DIR32          = 0x0006  # The target's 32-bit VA.
EFI_IMAGE_REL_I386_DIR32NB        = 0x0007  # The target's 32-bit RVA.
EFI_IMAGE_REL_I386_SEG12          = 0x0009  # Not supported.
EFI_IMAGE_REL_I386_SECTION        = 0x000A  # The 16-bit section index of the section that contains the target. This is used to support debugging information.
EFI_IMAGE_REL_I386_SECREL         = 0x000B  # The 32-bit offset of the target from the beginning of its section. This is used to support debugging information and static thread local storage.
EFI_IMAGE_REL_I386_TOKEN          = 0x000C  # The CLR token.
EFI_IMAGE_REL_I386_SECREL7        = 0x000D  # A 7-bit offset from the base of the section that contains the target.
EFI_IMAGE_REL_I386_REL32          = 0x0014  # The 32-bit relative displacement to the target. This supports the x86 relative branch and call instructions.

# SH3, SH4, SHM (Hitachi SuperH Processors); IPF (Intel Itanium Processor Family); MIPS processors; Mitsubishi M32R;  These Relocation Types are not listed here.


class EFI_IMAGE_BASE_RELOCATION(Structure):
    _pack_ = 1
    _fields_ = [
        ('VirtualAddress',        c_uint32),
        ('SizeOfBlock',           c_uint32),
    ]

EFI_IMAGE_SIZEOF_BASE_RELOCATION  = 8

## Based relocation types.
EFI_IMAGE_REL_BASED_ABSOLUTE      = 0
EFI_IMAGE_REL_BASED_HIGH          = 1
EFI_IMAGE_REL_BASED_LOW           = 2
EFI_IMAGE_REL_BASED_HIGHLOW       = 3
EFI_IMAGE_REL_BASED_HIGHADJ       = 4
EFI_IMAGE_REL_BASED_MIPS_JMPADDR  = 5
EFI_IMAGE_REL_BASED_ARM_MOV32A    = 5
EFI_IMAGE_REL_BASED_RISCV_HI20    = 5
EFI_IMAGE_REL_BASED_ARM_MOV32T    = 7
EFI_IMAGE_REL_BASED_RISCV_LOW12I  = 7
EFI_IMAGE_REL_BASED_RISCV_LOW12S  = 8
EFI_IMAGE_REL_BASED_IA64_IMM64    = 9
EFI_IMAGE_REL_BASED_DIR64         = 10

class EFI_IMAGE_LINENUMBER_TYPE(Union):
    _pack_ = 1
    _fields_ = [
        ('SymbolTableIndex',        c_uint32),  # Symbol table index of function name if Linenumber is 0.
        ('VirtualAddress',          c_uint32),  # Virtual address of line number.
    ]

class EFI_IMAGE_LINENUMBER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Type',                  EFI_IMAGE_LINENUMBER_TYPE),
        ('Linenumber',            c_uint16),   # Line number.
    ]

EFI_IMAGE_SIZEOF_LINENUMBER  = 6


## Archive format.
EFI_IMAGE_ARCHIVE_START_SIZE        = 8
EFI_IMAGE_ARCHIVE_START             = "!<arch>\n"
EFI_IMAGE_ARCHIVE_END               = "`\n"
EFI_IMAGE_ARCHIVE_PAD               = "\n"
EFI_IMAGE_ARCHIVE_LINKER_MEMBER     = "/               "
EFI_IMAGE_ARCHIVE_LONGNAMES_MEMBER  = "//              "

class EFI_IMAGE_ARCHIVE_MEMBER_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Name',                  ARRAY(c_uint8, 16)), # File member name - `/' terminated.
        ('Date',                  ARRAY(c_uint8, 12)), #
        ('UserID',                ARRAY(c_uint8, 6)),  #
        ('GroupID',               ARRAY(c_uint8, 6)),  #
        ('Mode',                  ARRAY(c_uint8, 8)),  #
        ('Size',                  ARRAY(c_uint8, 10)), #
        ('EndHeader',             ARRAY(c_uint8, 2)),  #
    ]

EFI_IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR = 60

class EFI_IMAGE_EXPORT_DIRECTORY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Characteristics',       c_uint32),
        ('TimeDateStamp',         c_uint32),
        ('MajorVersion',          c_uint16),
        ('MinorVersion',          c_uint16),
        ('Name',                  c_uint32),
        ('Base',                  c_uint32),
        ('NumberOfFunctions',     c_uint32),
        ('NumberOfNames',         c_uint32),
        ('AddressOfFunctions',    c_uint32),
        ('AddressOfNames',        c_uint32),
        ('AddressOfNameOrdinals', c_uint32),
    ]

## Based export types.
EFI_IMAGE_EXPORT_ORDINAL_BASE     = 1
EFI_IMAGE_EXPORT_ADDR_SIZE        = 4
EFI_IMAGE_EXPORT_ORDINAL_SIZE     = 2


class EFI_IMAGE_IMPORT_BY_NAME(Structure):
    _pack_ = 1
    _fields_ = [
        ('Hint',        c_uint16),
        ('Name',        c_uint8),
    ]


class EFI_IMAGE_THUNK_DATA_UNION(Union):
    _pack_ = 1
    _fields_ = [
        ('Function',             c_uint32),
        ('Ordinal',              c_uint32),
        ('AddressOfData',        EFI_IMAGE_IMPORT_BY_NAME),
    ]


class EFI_IMAGE_THUNK_DATA(Structure):
    _pack_ = 1
    _fields_ = [
        ('u1',        EFI_IMAGE_THUNK_DATA_UNION),
    ]

class EFI_IMAGE_IMPORT_DESCRIPTOR(Structure):
    _pack_ = 1
    _fields_ = [
        ('Characteristics',          c_uint32),
        ('TimeDateStamp',            c_uint32),
        ('ForwarderChain',           c_uint32),
        ('Name',                     c_uint32),
        ('FirstThunk',               EFI_IMAGE_THUNK_DATA),
    ]

EFI_IMAGE_DEBUG_TYPE_CODEVIEW  = 2

class EFI_IMAGE_DEBUG_DIRECTORY_ENTRY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Characteristics',          c_uint32),
        ('TimeDateStamp',            c_uint32),
        ('MajorVersion',             c_uint16),
        ('MinorVersion',             c_uint16),
        ('Type',                     c_uint32),
        ('SizeOfData',               c_uint32),
        ('RVA',                      c_uint32),
        ('FileOffset',               c_uint32),
    ]
EFI_IMAGE_DEBUG_DIRECTORY_ENTRY_STRUCTURE_SIZE = 0x1c

CODEVIEW_SIGNATURE_NB10 = 0x3031424E  # "NB10"

class EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Signature',          c_uint32),  # "NB10"
        ('Unknown',            c_uint32),
        ('Unknown2',           c_uint32),
        ('Unknown3',           c_uint32),
    ]

CODEVIEW_SIGNATURE_RSDS = 0x53445352  # "RSDS"

class EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Signature',          c_uint32),  # "NB10"
        ('Unknown',            c_uint32),
        ('Unknown2',           c_uint32),
        ('Unknown3',           c_uint32),
        ('Unknown4',           c_uint32),
        ('Unknown5',           c_uint32),
    ]

class EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Signature',          c_uint32),  # "MTOC"
        ('MachOUuid',          GUID),
    ]

class RUNTIME_FUNCTION(Structure):
    _pack_ = 1
    _fields_ = [
        ('FunctionStartAddress',        c_uint32),
        ('FunctionEndAddress',          c_uint32),
        ('UnwindInfoAddress',           c_uint32),
    ]

class UNWIND_INFO(Structure):
    _pack_ = 1
    _fields_ = [
        ('Version_Flag',         c_uint8), # Version:3   Flags:5
        ('SizeOfProlog',         c_uint8),
        ('CountOfUnwindCodes',   c_uint8),
        ('FrameRegister',        c_uint8), # FrameRegister:4  FrameRegisterOffset:4
    ]

    def Get_Version(self):
       return self.Version_Flag >> 5

    def Get_Flag(self):
        return self.Version_Flag << 3

class EFI_IMAGE_RESOURCE_DIRECTORY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Characteristics',        c_uint32),
        ('TimeDateStamp',          c_uint32),
        ('MajorVersion',           c_uint16),
        ('MinorVersion',           c_uint16),
        ('NumberOfNamedEntries',   c_uint16),
        ('NumberOfIdEntries',      c_uint16),
    ]


def Get_EFI_IMAGE_RESOURCE_DIRECTORY_STRING(nums: int):
    class EFI_IMAGE_RESOURCE_DIRECTORY_STRING(Structure):
        _pack_ = 1
        _fields_ = [
            ('Length',                 c_uint16),
            ('String',                 ARRAY(c_uint8, nums)),
        ]
    return EFI_IMAGE_RESOURCE_DIRECTORY_STRING

class EFI_IMAGE_RESOURCE_DATA_ENTRY(Structure):
    _pack_ = 1
    _fields_ = [
        ('OffsetToData',       c_uint32),
        ('Size',               c_uint32),
        ('CodePage',           c_uint32),
        ('Reserved',           c_uint32),
    ]

class EFI_BLK_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('PageRVA',            c_uint32),           # BlkHeader PageRVA
        ('BlockSize',          c_uint32),           # BlkHeader BlockSize
    ]

EFI_BLK_HEADER_SIZE = 0x8

# EFI_TE_IMAGE_HEADER.
class EFI_TE_IMAGE_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Signature',                c_uint16),           # signature for TE format = "VZ"
        ('Machine',                  c_uint16),           # from the original file header
        ('NumberOfSections',         c_uint8),            # from the original file header
        ('Subsystem',                c_uint8),            # from original optional header
        ('StrippedSize',             c_uint16),           # how many bytes we removed from the header
        ('AddressOfEntryPoint',      c_uint32),           # offset to entry point -- from original optional header
        ('BaseOfCode',               c_uint32),           # from original image -- required for ITP debug
        ('ImageBase',                c_uint64),           # from original file header
        ('DataDirectory',            ARRAY(EFI_IMAGE_DATA_DIRECTORY, 2)), # only base relocation and debug directory
    ]

EFI_TE_IMAGE_HEADER_SIGNATURE = 0x5A56      # "VZ"
EFI_TE_IMAGE_HEADER_SIZE = 0x28

## Data directory indexes in our TE image header
EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC  = 0
EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG      = 1


# Union of PE32, PE32+, and TE headers
class EFI_IMAGE_OPTIONAL_HEADER_UNION(Union):
    _pack_ = 1
    _fields_ = [
        ('Pe32',                EFI_IMAGE_NT_HEADERS32),
        ('Pe32Plus',            EFI_IMAGE_NT_HEADERS64),
        ('Te',                  EFI_TE_IMAGE_HEADER),
    ]

class EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION(Union):
    _pack_ = 1
    _fields_ = [
        ('Pe32',                EFI_IMAGE_NT_HEADERS32),  # these 4 fields should all be pointer
        ('Pe32Plus',            EFI_IMAGE_NT_HEADERS64),
        ('Te',                  EFI_TE_IMAGE_HEADER),
        ('Union',               EFI_IMAGE_OPTIONAL_HEADER_UNION),
    ]


## DLL characteristics
# 0x0001 / 0x0002 / 0x0004 / 0x0008   --- Reserved, must be zero
EFI_IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA         = 0x0020  # Image can handle a high entropy 64-bit virtual address space.
EFI_IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE            = 0x0040  # DLL can be relocated at load time.
EFI_IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY         = 0x0080  # Code Integrity checks are enforced.
EFI_IMAGE_DLLCHARACTERISTICS_NX_COMPAT               = 0x0100  # Image is NX compatible.
EFI_IMAGE_DLLCHARACTERISTICS_NO_ISOLATION            = 0x0200  # Isolation aware, but do not isolate the image.
EFI_IMAGE_DLLCHARACTERISTICS_NO_SEH                  = 0x0400  # Does not use structured exception (SE) handling. No SE handler may be called in this image.
EFI_IMAGE_DLLCHARACTERISTICS_NO_BIND                 = 0x0800  # Do not bind the image.
EFI_IMAGE_DLLCHARACTERISTICS_APPCONTAINER            = 0x1000  # Image must execute in an AppContainer.
EFI_IMAGE_DLLCHARACTERISTICS_WDM_DRIVER              = 0x2000  # A WDM driver.
EFI_IMAGE_DLLCHARACTERISTICS_GUARD_CF                = 0x4000  # Image supports Control Flow Guard.
EFI_IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE   = 0x8000  # Terminal Server aware.


class EFI_SYMBOL_NAME_REPRESENTATION(Structure):
    _pack_ = 1
    _fields_ = [
        ('ShortName',               c_uint64),       # An array of 8 bytes. This array is padded with nulls on the right if the name is less than 8 bytes long.
        ('Zeroes',                  c_uint32),       # A field that is set to all zeros if the name is longer than 8 bytes.
        ('Offset',                  c_uint32),       # An offset into the string table.
    ]


class EFI_COFF_SYMBOL_TABLE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Name ',                   c_uint64),          # The name of the symbol, represented by a union of three structures.
                                                        # An array of 8 bytes is used if the name is not more than 8 bytes long. For more information, see Symbol Name Representation.
        ('Value',                   c_uint32),          # The value that is associated with the symbol. The interpretation of this field depends on SectionNumber and StorageClass.
                                                        # A typical meaning is the relocatable address.
        ('SectionNumber',           c_uint16),          # The signed integer that identifies the section, using a one-based index into the section table.
                                                        # Some values have special meaning, as defined in section 5.4.2, "Section Number Values."
        ('Type',                    c_uint16),          # A number that represents type. Microsoft tools set this field to 0x20 (function) or 0x0 (not a function). For more information, see Type Representation.
        ('StorageClass',            c_uint8),           # An enumerated value that represents storage class. For more information, see Storage Class.
        ('NumberOfAuxSymbols',      c_uint8),           # The number of auxiliary symbol table entries that follow this record.

    ]
