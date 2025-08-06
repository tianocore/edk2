#!/usr/bin/python3
'''
Copyright (c) Apple Inc. 2021
SPDX-License-Identifier: BSD-2-Clause-Patent

Class that abstracts PE/COFF debug info parsing via a Python file like
object. You can port this code into an arbitrary debugger by invoking
the classes and passing in a file like object that abstracts the debugger
reading memory.

If you run this file directly it will parse the passed in PE/COFF files
for debug info:
python3 ./efi_pefcoff.py DxeCore.efi
IA32`<path...>/DxeCore.dll load = 0x00000000
EntryPoint = 0x000030d2  TextAddress = 0x00000240 DataAddress = 0x000042c0
.text    0x00000240 (0x04080) flags:0x60000020
.data    0x000042C0 (0x001C0) flags:0xC0000040
.reloc   0x00004480 (0x00240) flags:0x42000040

Note: PeCoffClass uses virtual addresses and not file offsets.
       It needs to work when images are loaded into memory.
       as long as virtual address map to file addresses this
       code can process binary files.

Note: This file can also contain generic worker functions (like GuidNames)
      that abstract debugger agnostic services to the debugger.

This file should never import debugger specific modules.
'''

import sys
import os
import uuid
import struct
import re
import zlib
from ctypes import c_char, c_uint8, c_uint16, c_uint32, c_uint64, c_void_p
from ctypes import ARRAY, sizeof
from ctypes import Structure, LittleEndianStructure

#
# The empty LittleEndianStructure must have _fields_ assigned prior to use or
#  sizeof(). Anything that is size UINTN may need to get adjusted.
#
# The issue is ctypes matches our local machine, not the machine we are
#  trying to debug. Call patch_ctypes() passing in the byte width from the
#  debugger python to make sure you are in sync.
#
# Splitting out the _field_ from the Structure (LittleEndianStructure) class
#  allows it to be patched.
#


class EFI_LOADED_IMAGE_PROTOCOL(LittleEndianStructure):
    pass


EFI_LOADED_IMAGE_PROTOCOL_fields_ = [
    ('Revision',                      c_uint32),
    ('ParentHandle',                  c_void_p),
    ('SystemTable',                   c_void_p),
    ('DeviceHandle',                  c_void_p),
    ('FilePath',                      c_void_p),
    ('Reserved',                      c_void_p),
    ('LoadOptionsSize',               c_uint32),
    ('LoadOptions',                   c_void_p),
    ('ImageBase',                     c_void_p),
    ('ImageSize',                     c_uint64),
    ('ImageCodeType',                 c_uint32),
    ('ImageDataType',                 c_uint32),
    ('Unload',                        c_void_p),
]


class EFI_GUID(LittleEndianStructure):
    _fields_ = [
        ('Data1',               c_uint32),
        ('Data2',               c_uint16),
        ('Data3',               c_uint16),
        ('Data4',               ARRAY(c_uint8, 8))
    ]


class EFI_SYSTEM_TABLE_POINTER(LittleEndianStructure):
    _fields_ = [
        ('Signature',                     c_uint64),
        ('EfiSystemTableBase',            c_uint64),
        ('Crc32',                         c_uint32)
    ]


class EFI_DEBUG_IMAGE_INFO_NORMAL(LittleEndianStructure):
    pass


EFI_DEBUG_IMAGE_INFO_NORMAL_fields_ = [
    ('ImageInfoType',                 c_uint32),
    ('LoadedImageProtocolInstance',   c_void_p),
    ('ImageHandle',                   c_void_p)
]


class EFI_DEBUG_IMAGE_INFO(LittleEndianStructure):
    pass


EFI_DEBUG_IMAGE_INFO_fields_ = [
    ('NormalImage',                   c_void_p),
]


class EFI_DEBUG_IMAGE_INFO_TABLE_HEADER(LittleEndianStructure):
    pass


EFI_DEBUG_IMAGE_INFO_TABLE_HEADER_fields_ = [
    ('UpdateStatus',                  c_uint32),
    ('TableSize',                     c_uint32),
    ('EfiDebugImageInfoTable',        c_void_p),
]


class EFI_TABLE_HEADER(LittleEndianStructure):
    _fields_ = [
        ('Signature',                     c_uint64),
        ('Revision',                      c_uint32),
        ('HeaderSize',                    c_uint32),
        ('CRC32',                         c_uint32),
        ('Reserved',                      c_uint32),
    ]


class EFI_CONFIGURATION_TABLE(LittleEndianStructure):
    pass


EFI_CONFIGURATION_TABLE_fields_ = [
    ('VendorGuid',                    EFI_GUID),
    ('VendorTable',                   c_void_p)
]


class EFI_SYSTEM_TABLE(LittleEndianStructure):
    pass


EFI_SYSTEM_TABLE_fields_ = [
    ('Hdr',                           EFI_TABLE_HEADER),
    ('FirmwareVendor',                c_void_p),
    ('FirmwareRevision',              c_uint32),
    ('ConsoleInHandle',               c_void_p),
    ('ConIn',                         c_void_p),
    ('ConsoleOutHandle',              c_void_p),
    ('ConOut',                        c_void_p),
    ('StandardErrHandle',             c_void_p),
    ('StdErr',                        c_void_p),
    ('RuntimeService',                c_void_p),
    ('BootService',                   c_void_p),
    ('NumberOfTableEntries',          c_void_p),
    ('ConfigurationTable',            c_void_p),
]


class EFI_IMAGE_DATA_DIRECTORY(LittleEndianStructure):
    _fields_ = [
        ('VirtualAddress',       c_uint32),
        ('Size',                 c_uint32)
    ]


class EFI_TE_IMAGE_HEADER(LittleEndianStructure):
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


class EFI_IMAGE_DOS_HEADER(LittleEndianStructure):
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


class EFI_IMAGE_FILE_HEADER(LittleEndianStructure):
    _fields_ = [
        ('Machine',               c_uint16),
        ('NumberOfSections',      c_uint16),
        ('TimeDateStamp',         c_uint32),
        ('PointerToSymbolTable',  c_uint32),
        ('NumberOfSymbols',       c_uint32),
        ('SizeOfOptionalHeader',  c_uint16),
        ('Characteristics',       c_uint16)
    ]


class EFI_IMAGE_OPTIONAL_HEADER32(LittleEndianStructure):
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
        ('CheckSum',                 c_uint32),
        ('Subsystem',                     c_uint16),
        ('DllCharacteristics',            c_uint16),
        ('SizeOfStackReserve',            c_uint32),
        ('SizeOfStackCommit',            c_uint32),
        ('SizeOfHeapReserve',             c_uint32),
        ('SizeOfHeapCommit',             c_uint32),
        ('LoaderFlags',              c_uint32),
        ('NumberOfRvaAndSizes',           c_uint32),
        ('DataDirectory',                 ARRAY(EFI_IMAGE_DATA_DIRECTORY, 16))
    ]


class EFI_IMAGE_NT_HEADERS32(LittleEndianStructure):
    _fields_ = [
        ('Signature',            c_uint32),
        ('FileHeader',           EFI_IMAGE_FILE_HEADER),
        ('OptionalHeader',       EFI_IMAGE_OPTIONAL_HEADER32)
    ]


class EFI_IMAGE_OPTIONAL_HEADER64(LittleEndianStructure):
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
        ('CheckSum',                 c_uint32),
        ('Subsystem',                     c_uint16),
        ('DllCharacteristics',            c_uint16),
        ('SizeOfStackReserve',            c_uint64),
        ('SizeOfStackCommit',            c_uint64),
        ('SizeOfHeapReserve',             c_uint64),
        ('SizeOfHeapCommit',             c_uint64),
        ('LoaderFlags',              c_uint32),
        ('NumberOfRvaAndSizes',           c_uint32),
        ('DataDirectory',                 ARRAY(EFI_IMAGE_DATA_DIRECTORY, 16))
    ]


class EFI_IMAGE_NT_HEADERS64(LittleEndianStructure):
    _fields_ = [
        ('Signature',            c_uint32),
        ('FileHeader',           EFI_IMAGE_FILE_HEADER),
        ('OptionalHeader',       EFI_IMAGE_OPTIONAL_HEADER64)
    ]


class EFI_IMAGE_DEBUG_DIRECTORY_ENTRY(LittleEndianStructure):
    _fields_ = [
        ('Characteristics',            c_uint32),
        ('TimeDateStamp',              c_uint32),
        ('MajorVersion',               c_uint16),
        ('MinorVersion',               c_uint16),
        ('Type',                       c_uint32),
        ('SizeOfData',                 c_uint32),
        ('RVA',                        c_uint32),
        ('FileOffset',                 c_uint32),
    ]


class EFI_IMAGE_SECTION_HEADER(LittleEndianStructure):
    _fields_ = [
        ('Name',                       ARRAY(c_char, 8)),
        ('VirtualSize',                c_uint32),
        ('VirtualAddress',             c_uint32),
        ('SizeOfRawData',              c_uint32),
        ('PointerToRawData',           c_uint32),
        ('PointerToRelocations',       c_uint32),
        ('PointerToLinenumbers',       c_uint32),
        ('NumberOfRelocations',        c_uint16),
        ('NumberOfLinenumbers',        c_uint16),
        ('Characteristics',            c_uint32),
    ]


EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC = 0x10b
EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC = 0x20b

DIRECTORY_DEBUG = 6


image_machine_dict = {
    0x014c: "IA32",
    0x0200: "IPF",
    0x0EBC: "EBC",
    0x8664: "X64",
    0x01c2: "ARM",
    0xAA64: "AArch64",
    0x5032: "RISC32",
    0x5064: "RISC64",
    0x5128: "RISCV128",
}


def patch_void_p_to_ctype(patch_type, to_patch):
    '''Optionally patch c_void_p in the Structure._fields_'''
    if patch_type is None:
        return to_patch

    result = []
    for name, c_type in to_patch:
        if type(c_type) == type(c_void_p):
            result.append((name, c_uint32))
        else:
            result.append((name, c_type))
    return result


def patch_ctypes(pointer_width=8):
    '''
    Pass in the pointer width of the system being debugged. If it is not
    the same as c_void_p then patch the _fields_ with the correct type.
    For any ctypes Structure that has a c_void_p this function needs to be
    called prior to use or sizeof() to initialize _fields_.
    '''

    if sizeof(c_void_p) == pointer_width:
        patch_type = None
    elif pointer_width == 16:
        assert False
    elif pointer_width == 8:
        patch_type = c_uint64
    elif pointer_width == 4:
        patch_type = c_uint32
    else:
        raise Exception(f'ERROR: Unkown pointer_width = {pointer_width}')

    # If you add a ctypes Structure class with a c_void_p you need to add
    # it to this list. Note: you should use c_void_p for UINTN values.
    EFI_LOADED_IMAGE_PROTOCOL._fields_ = patch_void_p_to_ctype(
        patch_type, EFI_LOADED_IMAGE_PROTOCOL_fields_)
    EFI_DEBUG_IMAGE_INFO_NORMAL._fields_ = patch_void_p_to_ctype(
        patch_type, EFI_DEBUG_IMAGE_INFO_NORMAL_fields_)
    EFI_DEBUG_IMAGE_INFO._fields_ = patch_void_p_to_ctype(
        patch_type, EFI_DEBUG_IMAGE_INFO_fields_)
    EFI_DEBUG_IMAGE_INFO_TABLE_HEADER._fields_ = patch_void_p_to_ctype(
        patch_type, EFI_DEBUG_IMAGE_INFO_TABLE_HEADER_fields_)
    EFI_CONFIGURATION_TABLE._fields_ = patch_void_p_to_ctype(
        patch_type, EFI_CONFIGURATION_TABLE_fields_)
    EFI_SYSTEM_TABLE._fields_ = patch_void_p_to_ctype(
        patch_type, EFI_SYSTEM_TABLE_fields_)

    # patch up anything else that needs to know pointer_width
    EfiStatusClass(pointer_width)


def ctype_to_str(ctype, indent='', hide_list=[]):
    '''
    Given a ctype object print out as a string by walking the _fields_
    in the cstring Class
     '''
    result = ''
    for field in ctype._fields_:
        attr = getattr(ctype, field[0])
        tname = type(attr).__name__
        if field[0] in hide_list:
            continue

        result += indent + f'{field[0]} = '
        if tname == 'EFI_GUID':
            result += GuidNames.to_name(GuidNames.to_uuid(attr)) + '\n'
        elif issubclass(type(attr), Structure):
            result += f'{tname}\n' + \
                ctype_to_str(attr, indent + '  ', hide_list)
        elif isinstance(attr, int):
            result += f'0x{attr:x}\n'
        else:
            result += f'{attr}\n'

    return result


def hexline(addr, data):
    hexstr = ''
    printable = ''
    for i in range(0, len(data)):
        hexstr += f'{data[i]:02x} '
        printable += chr(data[i]) if data[i] > 0x20 and data[i] < 0x7f else '.'
    return f'{addr:04x}  {hexstr:48s} |{printable:s}|'


def hexdump(data, indent=''):
    if not isinstance(data, bytearray):
        data = bytearray(data)

    result = ''
    for i in range(0, len(data), 16):
        result += indent + hexline(i, data[i:i+16]) + '\n'
    return result


class EfiTpl:
    ''' Return string for EFI_TPL'''

    def __init__(self, tpl):
        self.tpl = tpl

    def __str__(self):
        if self.tpl < 4:
            result = f'{self.tpl:d}'
        elif self.tpl < 8:
            result = "TPL_APPLICATION"
            if self.tpl - 4 > 0:
                result += f' + {self.tpl - 4:d}'
        elif self.tpl < 16:
            result = "TPL_CALLBACK"
            if self.tpl - 8 > 0:
                result += f' + {self.tpl - 8:d}'
        elif self.tpl < 31:
            result = "TPL_NOTIFY"
            if self.tpl - 16 > 0:
                result += f' + {self.tpl - 16:d}'
        elif self.tpl == 31:
            result = "TPL_HIGH_LEVEL"
        else:
            result = f'Invalid TPL = {self.tpl:d}'
        return result


class EfiBootMode:
    '''
    Class to return human readable string for EFI_BOOT_MODE

    Methods
    -----------
    to_str(boot_mode, default)
        return string for boot_mode, and return default if there is not a
        match.
    '''

    EFI_BOOT_MODE_dict = {
        0x00: "BOOT_WITH_FULL_CONFIGURATION",
        0x01: "BOOT_WITH_MINIMAL_CONFIGURATION",
        0x02: "BOOT_ASSUMING_NO_CONFIGURATION_CHANGES",
        0x03: "BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS",
        0x04: "BOOT_WITH_DEFAULT_SETTINGS",
        0x05: "BOOT_ON_S4_RESUME",
        0x06: "BOOT_ON_S5_RESUME",
        0x07: "BOOT_WITH_MFG_MODE_SETTINGS",
        0x10: "BOOT_ON_S2_RESUME",
        0x11: "BOOT_ON_S3_RESUME",
        0x12: "BOOT_ON_FLASH_UPDATE",
        0x20: "BOOT_IN_RECOVERY_MODE",
    }

    def __init__(self, boot_mode):
        self._boot_mode = boot_mode

    def __str__(self):
        return self.to_str(self._boot_mode)

    @classmethod
    def to_str(cls, boot_mode, default=''):
        return cls.EFI_BOOT_MODE_dict.get(boot_mode, default)


class EfiStatusClass:
    '''
    Class to decode EFI_STATUS to a human readable string. You need to
    pass in pointer_width to get the corret value since the EFI_STATUS
    code values are different based on the sizeof UINTN. The default is
    sizeof(UINTN) == 8.

    Attributes
    ??????
    _dict_ : dictionary
        dictionary of EFI_STATUS that has beed updated to match
        pointer_width.

    Methods
    -----------
    patch_dictionary(pointer_width)

    to_str(status, default)
    '''

    _dict_ = {}
    _EFI_STATUS_UINT32_dict = {
        0: "Success",
        1: "Warning Unknown Glyph",
        2: "Warning Delete Failure",
        3: "Warning Write Failure",
        4: "Warning Buffer Too Small",
        5: "Warning Stale Data",
        6: "Warngin File System",
        (0x20000000 | 0): "Warning interrupt source pending",
        (0x20000000 | 1): "Warning interrupt source quiesced",

        (0x80000000 | 1): "Load Error",
        (0x80000000 | 2): "Invalid Parameter",
        (0x80000000 | 3): "Unsupported",
        (0x80000000 | 4): "Bad Buffer Size",
        (0x80000000 | 5): "Buffer Too Small",
        (0x80000000 | 6): "Not Ready",
        (0x80000000 | 7): "Device Error",
        (0x80000000 | 8): "Write Protected",
        (0x80000000 | 9): "Out of Resources",
        (0x80000000 | 10): "Volume Corrupt",
        (0x80000000 | 11): "Volume Full",
        (0x80000000 | 12): "No Media",
        (0x80000000 | 13): "Media changed",
        (0x80000000 | 14): "Not Found",
        (0x80000000 | 15): "Access Denied",
        (0x80000000 | 16): "No Response",
        (0x80000000 | 17): "No mapping",
        (0x80000000 | 18): "Time out",
        (0x80000000 | 19): "Not started",
        (0x80000000 | 20): "Already started",
        (0x80000000 | 21): "Aborted",
        (0x80000000 | 22): "ICMP Error",
        (0x80000000 | 23): "TFTP Error",
        (0x80000000 | 24): "Protocol Error",
        (0x80000000 | 25): "Incompatible Version",
        (0x80000000 | 26): "Security Violation",
        (0x80000000 | 27): "CRC Error",
        (0x80000000 | 28): "End of Media",
        (0x80000000 | 31): "End of File",
        (0x80000000 | 32): "Invalid Language",
        (0x80000000 | 33): "Compromised Data",
        (0x80000000 | 35): "HTTP Error",

        (0xA0000000 | 0): "Interrupt Pending",
    }

    def __init__(self, status=None, pointer_width=8):
        self.status = status
        # this will convert to 64-bit version if needed
        self.patch_dictionary(pointer_width)

    def __str__(self):
        return self.to_str(self.status)

    @classmethod
    def to_str(cls, status, default=''):
        return cls._dict_.get(status, default)

    @classmethod
    def patch_dictionary(cls, pointer_width):
        '''Patch UINTN upper bits like values '''

        if cls._dict_:
            # only patch the class variable once
            return False

        if pointer_width == 4:
            cls._dict = cls._EFI_STATUS_UINT32_dict
        elif pointer_width == 8:
            for key, value in cls._EFI_STATUS_UINT32_dict.items():
                mask = (key & 0xE0000000) << 32
                new_key = (key & 0x1FFFFFFF) | mask
                cls._dict_[new_key] = value
            return True
        else:
            return False


class GuidNames:
    '''
    Class to expose the C names of EFI_GUID's. The _dict_ starts with
    common EFI System Table entry EFI_GUID's. _dict_ can get updated with the
    build generated Guid.xref file if a path to a module is passed
    into add_build_guid_file(). If symbols are loaded for any module
    in the build the path the build product should imply the
    relative location of that builds Guid.xref file.

    Attributes
    ??????----
    _dict_ : dictionary
        dictionary of EFI_GUID (uuid) strings to C global names

    Methods
    -------
    to_uuid(uuid)
        convert a hex UUID string or bytearray to a uuid.UUID
    to_name(uuid)
        convert a UUID string to a C global constant name.
    to_guid(guid_name)
        convert a C global constant EFI_GUID name to uuid hex string.
    is_guid_str(name)
       name is a hex UUID string.
       Example: 49152E77-1ADA-4764-B7A2-7AFEFED95E8B

    to_c_guid(value)
        convert a uuid.UUID or UUID string to a c_guid string
        (see is_c_guid())
    from_c_guid(value)
        covert a C guid string to a hex UUID string.
    is_c_guid(name)
        name is the C initialization value for an EFI_GUID. Example:
        { 0x414e6bdd, 0xe47b, 0x47cc, { 0xb2, 0x44, 0xbb, 0x61,
                                        0x02, 0x0c, 0xf5, 0x16 }}

    add_build_guid_file(module_path, custom_file):
        assume module_path is an edk2 build product and load the Guid.xref
        file from that build to fill in _dict_. If you know the path and
        file name of a custom Guid.xref  you can pass it in as custom_file.

    '''
    _dict_ = {  # Common EFI System Table values
        '05AD34BA-6F02-4214-952E-4DA0398E2BB9':
            'gEfiDxeServicesTableGuid',
        '7739F24C-93D7-11D4-9A3A-0090273FC14D':
            'gEfiHobListGuid',
        '4C19049F-4137-4DD3-9C10-8B97A83FFDFA':
            'gEfiMemoryTypeInformationGuid',
        '49152E77-1ADA-4764-B7A2-7AFEFED95E8B':
            'gEfiDebugImageInfoTableGuid',
        '060CC026-4C0D-4DDA-8F41-595FEF00A502':
            'gMemoryStatusCodeRecordGuid',
        'EB9D2D31-2D88-11D3-9A16-0090273FC14D':
            'gEfiSmbiosTableGuid',
        'EB9D2D30-2D88-11D3-9A16-0090273FC14D':
            'gEfiAcpi10TableGuid',
        '8868E871-E4F1-11D3-BC22-0080C73C8881':
            'gEfiAcpi20TableGuid',
    }

    guid_files = []

    def __init__(self, uuid=None, pointer_width=8):
        self.uuid = None if uuid is None else self.to_uuid(uuid)

    def __str__(self):
        if self.uuid is None:
            result = ''
            for key, value in GuidNames._dict_.items():
                result += f'{key}: {value}\n'
        else:
            result = self.to_name(self.uuid)

        return result

    @classmethod
    def to_uuid(cls, obj):
        try:
            return uuid.UUID(bytes_le=bytes(obj))
        except (ValueError, TypeError):
            try:
                return uuid.UUID(bytes_le=obj)
            except (ValueError, TypeError):
                return uuid.UUID(obj)

    @classmethod
    def to_name(cls, uuid):
        if not isinstance(uuid, str):
            uuid = str(uuid)
        if cls.is_c_guid(uuid):
            uuid = cls.from_c_guid(uuid)
        return cls._dict_.get(uuid.upper(), uuid.upper())

    @classmethod
    def to_guid(cls, guid_name):
        for key, value in cls._dict_.items():
            if guid_name == value:
                return key.upper()
        else:
            raise KeyError(key)

    @classmethod
    def is_guid_str(cls, name):
        if not isinstance(name, str):
            return False
        return name.count('-') >= 4

    @classmethod
    def to_c_guid(cls, value):
        if isinstance(value, uuid.UUID):
            guid = value
        else:
            guid = uuid.UUID(value)

        (data1, data2, data3,
         data4_0, data4_1, data4_2, data4_3,
         data4_4, data4_5, data4_6, data4_7) = struct.unpack(
            '<IHH8B', guid.bytes_le)
        return (f'{{ 0x{data1:08X}, 0x{data2:04X}, 0x{data3:04X}, '
                f'{{ 0x{data4_0:02X}, 0x{data4_1:02X}, 0x{data4_2:02X}, '
                f'0x{data4_3:02X}, 0x{data4_4:02X}, 0x{data4_5:02X}, '
                f'0x{data4_6:02X}, 0x{data4_7:02X} }} }}')

    @ classmethod
    def from_c_guid(cls, value):
        try:
            hex = [int(x, 16) for x in re.findall(r"[\w']+", value)]
            return (f'{hex[0]:08X}-{hex[1]:04X}-{hex[2]:04X}'
                    + f'-{hex[3]:02X}{hex[4]:02X}-{hex[5]:02X}{hex[6]:02X}'
                    + f'{hex[7]:02X}{hex[8]:02X}{hex[9]:02X}{hex[10]:02X}')
        except ValueError:
            return value

    @ classmethod
    def is_c_guid(cls, name):
        if not isinstance(name, str):
            return False
        return name.count('{') == 2 and name.count('}') == 2

    @ classmethod
    def add_build_guid_file(cls, module_path, custom_file=None):
        if custom_file is not None:
            xref = custom_file
        else:
            # module_path will look like:
            # <repo>/Build/OvmfX64/DEBUG_XCODE5/X64/../DxeCore.dll
            # Walk backwards looking for a toolchain like name.
            # Then look for GUID database:
            # Build/OvmfX64//DEBUG_XCODE5/FV/Guid.xref
            for i in reversed(module_path.split(os.sep)):
                if (i.startswith('DEBUG_') or
                    i.startswith('RELEASE_') or
                        i.startswith('NOOPT_')):
                    build_root = os.path.join(
                        module_path.rsplit(i, 1)[0], i)
                    break

            xref = os.path.join(build_root, 'FV', 'Guid.xref')

        if xref in cls.guid_files:
            # only processes the file one time
            return True

        with open(xref) as f:
            content = f.readlines()
            cls.guid_files.append(xref)

            for lines in content:
                try:
                    if cls.is_guid_str(lines):
                        # a regex would be more pedantic
                        words = lines.split()
                        cls._dict_[words[0].upper()] = words[1].strip('\n')
                except ValueError:
                    pass

            return True

        return False


class EFI_HOB_GENERIC_HEADER(LittleEndianStructure):
    _fields_ = [
        ('HobType',             c_uint16),
        ('HobLength',           c_uint16),
        ('Reserved',            c_uint32)
    ]


class EFI_HOB_HANDOFF_INFO_TABLE(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('Version',             c_uint32),
        ('BootMode',            c_uint32),
        ('EfiMemoryTop',        c_uint64),
        ('EfiMemoryBottom',     c_uint64),
        ('EfiFreeMemoryTop',    c_uint64),
        ('EfiFreeMemoryBottom', c_uint64),
        ('EfiEndOfHobList',     c_uint64),
    ]


class EFI_HOB_MEMORY_ALLOCATION(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('Name',                EFI_GUID),
        ('MemoryBaseAddress',   c_uint64),
        ('MemoryLength',        c_uint64),
        ('MemoryType',          c_uint32),
        ('Reserved',            c_uint32),
    ]


class EFI_HOB_RESOURCE_DESCRIPTOR(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('Owner',               EFI_GUID),
        ('ResourceType',        c_uint32),
        ('ResourceAttribute',   c_uint32),
        ('PhysicalStart',       c_uint64),
        ('ResourceLength',      c_uint64),
    ]


class EFI_HOB_GUID_TYPE(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('Name',                EFI_GUID),
    ]


class EFI_HOB_FIRMWARE_VOLUME(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('BaseAddress',         c_uint64),
        ('Length',              c_uint64),
    ]


class EFI_HOB_CPU(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('SizeOfMemorySpace',   c_uint8),
        ('SizeOfIoSpace',       c_uint8),
        ('Reserved',            ARRAY(c_uint8, 6)),
    ]


class EFI_HOB_MEMORY_POOL(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
    ]


class EFI_HOB_FIRMWARE_VOLUME2(LittleEndianStructure):
    _fields_ = [
        ('Header',              EFI_HOB_GENERIC_HEADER),
        ('BaseAddress',         c_uint64),
        ('Length',              c_uint64),
        ('FvName',              EFI_GUID),
        ('FileName',            EFI_GUID)
    ]


class EFI_HOB_FIRMWARE_VOLUME3(LittleEndianStructure):
    _fields_ = [
        ('HobType',             c_uint16),
        ('HobLength',           c_uint16),
        ('Reserved',            c_uint32),
        ('BaseAddress',         c_uint64),
        ('Length',              c_uint64),
        ('AuthenticationStatus', c_uint32),
        ('ExtractedFv',         c_uint8),
        ('FvName',              EFI_GUID),
        ('FileName',            EFI_GUID),
    ]


class EFI_HOB_UEFI_CAPSULE(LittleEndianStructure):
    _fields_ = [
        ('HobType',             c_uint16),
        ('HobLength',           c_uint16),
        ('Reserved',            c_uint32),
        ('BaseAddress',         c_uint64),
        ('Length',              c_uint64),
    ]


class EfiHob:
    '''
    Parse EFI Device Paths based on the edk2 C Structures defined above.
    In the context of this class verbose means hexdump extra data.


    Attributes
    ??????
    Hob : list
        List of HOBs. Each entry contains the name, HOB type, HOB length,
        the ctype struct for the HOB, and any extra data.

    Methods
    -----------
    get_hob_by_type(hob_type)
        return string that decodes the HOBs of hob_type. If hob_type is
        None then return all HOBs.
    '''

    Hob = []
    verbose = False

    hob_dict = {
        1: EFI_HOB_HANDOFF_INFO_TABLE,
        2: EFI_HOB_MEMORY_ALLOCATION,
        3: EFI_HOB_RESOURCE_DESCRIPTOR,
        4: EFI_HOB_GUID_TYPE,
        5: EFI_HOB_FIRMWARE_VOLUME,
        6: EFI_HOB_CPU,
        7: EFI_HOB_MEMORY_POOL,
        9: EFI_HOB_FIRMWARE_VOLUME2,
        0xb: EFI_HOB_UEFI_CAPSULE,
        0xc: EFI_HOB_FIRMWARE_VOLUME3,
        0xffff: EFI_HOB_GENERIC_HEADER,
    }

    def __init__(self, file, address=None, verbose=False, count=1000):
        self._file = file
        EfiHob.verbose = verbose

        if len(EfiHob.Hob) != 0 and address is None:
            return

        if address is not None:
            hob_ptr = address
        else:
            hob_ptr = EfiConfigurationTable(file).GetConfigTable(
                '7739F24C-93D7-11D4-9A3A-0090273FC14D')

        self.read_hobs(hob_ptr)

    @ classmethod
    def __str__(cls):
        return cls.get_hob_by_type(None)

    @ classmethod
    def get_hob_by_type(cls, hob_type):
        result = ""
        for (Name, HobType, HobLen, chob, extra) in cls.Hob:
            if hob_type is not None:
                if hob_type != HobType:
                    continue

            result += f'Type: {Name:s} (0x{HobType:01x}) Len: 0x{HobLen:03x}\n'
            result += ctype_to_str(chob, '  ', ['Reserved'])
            if cls.verbose:
                if extra is not None:
                    result += hexdump(extra, '    ')

        return result

    def read_hobs(self, hob_ptr, count=1000):
        if hob_ptr is None:
            return

        try:
            for _ in range(count):  # while True
                hdr, _ = self._ctype_read_ex(EFI_HOB_GENERIC_HEADER, hob_ptr)
                if hdr.HobType == 0xffff:
                    break

                type_str = self.hob_dict.get(
                    hdr.HobType, EFI_HOB_GENERIC_HEADER)
                hob, extra = self._ctype_read_ex(
                    type_str, hob_ptr, hdr.HobLength)
                EfiHob.Hob.append(
                    (type(hob).__name__,
                     hdr.HobType,
                     hdr.HobLength,
                     hob,
                     extra))
                hob_ptr += hdr.HobLength
        except ValueError:
            pass

    def _ctype_read_ex(self, ctype_struct, offset=0, rsize=None):
        if offset != 0:
            self._file.seek(offset)

        type_size = sizeof(ctype_struct)
        size = rsize if rsize else type_size
        data = self._file.read(size)
        cdata = ctype_struct.from_buffer(bytearray(data))

        if size > type_size:
            return cdata, data[type_size:]
        else:
            return cdata, None


class EFI_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Type',                c_uint8),
        ('SubType',             c_uint8),

        # UINT8 Length[2]
        # Cheat and use c_uint16 since we don't care about alignment
        ('Length',              c_uint16)
    ]


class PCI_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('Function',            c_uint8),
        ('Device',              c_uint8)
    ]


class PCCARD_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('FunctionNumber',      c_uint8),
    ]


class MEMMAP_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('StartingAddress',     c_uint64),
        ('EndingAddress',       c_uint64),
    ]


class VENDOR_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('Guid',                EFI_GUID),
    ]


class CONTROLLER_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('ControllerNumber',    c_uint32),
    ]


class BMC_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('InterfaceType',       c_uint8),
        ('BaseAddress',         ARRAY(c_uint8, 8)),
    ]


class BBS_BBS_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('DeviceType',          c_uint16),
        ('StatusFlag',          c_uint16)
    ]


class ACPI_HID_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('HID',                 c_uint32),
        ('UID',                 c_uint32)
    ]


class ACPI_EXTENDED_HID_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('HID',                 c_uint32),
        ('UID',                 c_uint32),
        ('CID',                 c_uint32)
    ]


class ACPI_ADR_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('ARD',                 c_uint32)
    ]


class ACPI_NVDIMM_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('NFITDeviceHandle',    c_uint32)
    ]


class ATAPI_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("PrimarySecondary",    c_uint8),
        ("SlaveMaster",         c_uint8),
        ("Lun",                 c_uint16)
    ]


class SCSI_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Pun",                 c_uint16),
        ("Lun",                 c_uint16)
    ]


class FIBRECHANNEL_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Reserved",            c_uint32),
        ("WWN",                 c_uint64),
        ("Lun",                 c_uint64)
    ]


class F1394_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Reserved",            c_uint32),
        ("Guid",                c_uint64)
    ]


class USB_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("ParentPortNumber",    c_uint8),
        ("InterfaceNumber",     c_uint8),
    ]


class I2O_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Tid",                 c_uint32)
    ]


class INFINIBAND_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("ResourceFlags",       c_uint32),
        ("PortGid",             ARRAY(c_uint8, 16)),
        ("ServiceId",           c_uint64),
        ("TargetPortId",        c_uint64),
        ("DeviceId",            c_uint64)
    ]


class UART_FLOW_CONTROL_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Guid",                EFI_GUID),
        ("FlowControlMap",      c_uint32)
    ]


class SAS_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Guid",                EFI_GUID),
        ("Reserved",            c_uint32),
        ("SasAddress",          c_uint64),
        ("Lun",                 c_uint64),
        ("DeviceTopology",      c_uint16),
        ("RelativeTargetPort",  c_uint16)
    ]


class EFI_MAC_ADDRESS(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("Addr",             ARRAY(c_uint8, 32)),
    ]


class MAC_ADDR_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('MacAddress',          EFI_MAC_ADDRESS),
        ('IfType',              c_uint8)
    ]


class IPv4_ADDRESS(LittleEndianStructure):
    _fields_ = [
        ("Addr",             ARRAY(c_uint8, 4)),
    ]


class IPv4_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('LocalIpAddress',      IPv4_ADDRESS),
        ('RemoteIpAddress',     IPv4_ADDRESS),
        ('LocalPort',           c_uint16),
        ('RemotePort',          c_uint16),
        ('Protocol',            c_uint16),
        ('StaticIpAddress',     c_uint8),
        ('GatewayIpAddress',    IPv4_ADDRESS),
        ('SubnetMask',          IPv4_ADDRESS)
    ]


class IPv6_ADDRESS(LittleEndianStructure):
    _fields_ = [
        ("Addr",             ARRAY(c_uint8, 16)),
    ]


class IPv6_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('LocalIpAddress',      IPv6_ADDRESS),
        ('RemoteIpAddress',     IPv6_ADDRESS),
        ('LocalPort',           c_uint16),
        ('RemotePort',          c_uint16),
        ('Protocol',            c_uint16),
        ('IpAddressOrigin',     c_uint8),
        ('PrefixLength',        c_uint8),
        ('GatewayIpAddress',    IPv6_ADDRESS)
    ]


class UART_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('Reserved',            c_uint32),
        ('BaudRate',            c_uint64),
        ('DataBits',            c_uint8),
        ('Parity',              c_uint8),
        ('StopBits',            c_uint8)
    ]


class USB_CLASS_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('VendorId',            c_uint16),
        ('ProductId',           c_uint16),
        ('DeviceClass',         c_uint8),
        ('DeviceCSjblass',      c_uint8),
        ('DeviceProtocol',      c_uint8),
    ]


class USB_WWID_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('InterfaceNumber',     c_uint16),
        ('VendorId',            c_uint16),
        ('ProductId',           c_uint16),
    ]


class DEVICE_LOGICAL_UNIT_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('Lun',                 c_uint8)
    ]


class SATA_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',                      EFI_DEVICE_PATH),
        ('HBAPortNumber',               c_uint16),
        ('PortMultiplierPortNumber',    c_uint16),
        ('Lun',                         c_uint16),
    ]


class ISCSI_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',                EFI_DEVICE_PATH),
        ('NetworkProtocol',       c_uint16),
        ('LoginOption',           c_uint16),
        ('Lun',                   c_uint64),
        ('TargetPortalGroupTag',  c_uint16),
    ]


class VLAN_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("VlandId",             c_uint16)
    ]


class FIBRECHANNELEX_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Reserved",            c_uint16),
        ("WWN",                 ARRAY(c_uint8, 8)),
        ("Lun",                 ARRAY(c_uint8, 8)),
    ]


class SASEX_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("SasAddress",          ARRAY(c_uint8, 8)),
        ("Lun",                 ARRAY(c_uint8, 8)),
        ("DeviceTopology",      c_uint16),
        ("RelativeTargetPort",  c_uint16)
    ]


class NVME_NAMESPACE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("NamespaceId",         c_uint32),
        ("NamespaceUuid",       c_uint64)
    ]


class DNS_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("IsIPv6",              c_uint8),
        ("DnsServerIp",         IPv6_ADDRESS)

    ]


class UFS_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Pun",                 c_uint8),
        ("Lun",                 c_uint8),
    ]


class SD_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("SlotNumber",          c_uint8)
    ]


class BLUETOOTH_ADDRESS(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("Address",             ARRAY(c_uint8, 6))
    ]


class BLUETOOTH_LE_ADDRESS(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ("Format",          c_uint8),
        ("Class",           c_uint16)
    ]


class BLUETOOTH_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("BD_ADDR",             BLUETOOTH_ADDRESS)
    ]


class WIFI_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("SSId",                ARRAY(c_uint8, 32))
    ]


class EMMC_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("SlotNumber",          c_uint8)
    ]


class BLUETOOTH_LE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("BD_ADDR",             BLUETOOTH_LE_ADDRESS)
    ]


class NVDIMM_NAMESPACE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("Uuid",                EFI_GUID)
    ]


class REST_SERVICE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("RESTService",         c_uint8),
        ("AccessMode",          c_uint8)
    ]


class REST_VENDOR_SERVICE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ("RESTService",         c_uint8),
        ("AccessMode",          c_uint8),
        ("Guid",                EFI_GUID),
    ]


class HARDDRIVE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('PartitionNumber',     c_uint32),
        ('PartitionStart',      c_uint64),
        ('PartitionSize',       c_uint64),
        ('Signature',           ARRAY(c_uint8, 16)),
        ('MBRType',             c_uint8),
        ('SignatureType',       c_uint8)
    ]


class CDROM_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('BootEntry',           c_uint32),
        ('PartitionStart',      c_uint64),
        ('PartitionSize',       c_uint64)
    ]


class MEDIA_PROTOCOL_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('Protocol',            EFI_GUID)
    ]


class MEDIA_FW_VOL_FILEPATH_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('FvFileName',          EFI_GUID)
    ]


class MEDIA_FW_VOL_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('FvName',              EFI_GUID)
    ]


class MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('Reserved',            c_uint32),
        ('StartingOffset',      c_uint64),
        ('EndingOffset',        c_uint64)
    ]


class MEDIA_RAM_DISK_DEVICE_PATH(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Header',              EFI_DEVICE_PATH),
        ('StartingAddr',        c_uint64),
        ('EndingAddr',          c_uint64),
        ('TypeGuid',            EFI_GUID),
        ('Instance',            c_uint16)
    ]


class EfiDevicePath:
    '''
    Parse EFI Device Paths based on the edk2 C Structures defined above.
    In the context of this class verbose means hexdump extra data.


    Attributes
    ??????
    DevicePath : list
        List of devixe path instances. Each instance is a list of nodes
        for the given Device Path instance.

    Methods
    -----------
    device_path_node(address)
        return the Device Path ctype hdr, ctype, and any extra data in
        the Device Path node. This is just a single Device Path node,
        not the entire Device Path.
    device_path_node_str(address)
        return the device path node (not the entire Device Path) as a string
    '''

    DevicePath = []

    device_path_dict = {
        # ( Type, SubType ) : Device Path C typedef
        # HARDWARE_DEVICE_PATH
        (1,  1): PCI_DEVICE_PATH,
        (1,  2): PCCARD_DEVICE_PATH,
        (1,  3): MEMMAP_DEVICE_PATH,
        (1,  4): VENDOR_DEVICE_PATH,
        (1,  5): CONTROLLER_DEVICE_PATH,
        (1,  6): BMC_DEVICE_PATH,

        # ACPI_DEVICE_PATH
        (2,  1): ACPI_HID_DEVICE_PATH,
        (2,  2): ACPI_EXTENDED_HID_DEVICE_PATH,
        (2,  3): ACPI_ADR_DEVICE_PATH,
        (2,  4): ACPI_NVDIMM_DEVICE_PATH,

        # MESSAGING_DEVICE_PATH
        (3,  1): ATAPI_DEVICE_PATH,
        (3,  2): SCSI_DEVICE_PATH,
        (3,  3): FIBRECHANNEL_DEVICE_PATH,
        (3,  4): F1394_DEVICE_PATH,
        (3,  5): USB_DEVICE_PATH,
        (3,  6): I2O_DEVICE_PATH,

        (3,  9): INFINIBAND_DEVICE_PATH,
        (3, 10): VENDOR_DEVICE_PATH,
        (3, 11): MAC_ADDR_DEVICE_PATH,
        (3, 12): IPv4_DEVICE_PATH,
        (3, 13): IPv6_DEVICE_PATH,
        (3, 14): UART_DEVICE_PATH,
        (3, 15): USB_CLASS_DEVICE_PATH,
        (3, 16): USB_WWID_DEVICE_PATH,
        (3, 17): DEVICE_LOGICAL_UNIT_DEVICE_PATH,
        (3, 18): SATA_DEVICE_PATH,
        (3, 19): ISCSI_DEVICE_PATH,
        (3, 20): VLAN_DEVICE_PATH,
        (3, 21): FIBRECHANNELEX_DEVICE_PATH,
        (3, 22): SASEX_DEVICE_PATH,
        (3, 23): NVME_NAMESPACE_DEVICE_PATH,
        (3, 24): DNS_DEVICE_PATH,
        (3, 25): UFS_DEVICE_PATH,
        (3, 26): SD_DEVICE_PATH,
        (3, 27): BLUETOOTH_DEVICE_PATH,
        (3, 28): WIFI_DEVICE_PATH,
        (3, 29): EMMC_DEVICE_PATH,
        (3, 30): BLUETOOTH_LE_DEVICE_PATH,
        (3, 31): DNS_DEVICE_PATH,
        (3, 32): NVDIMM_NAMESPACE_DEVICE_PATH,

        (3, 33): REST_SERVICE_DEVICE_PATH,
        (3, 34): REST_VENDOR_SERVICE_DEVICE_PATH,

        # MEDIA_DEVICE_PATH
        (4,  1): HARDDRIVE_DEVICE_PATH,
        (4,  2): CDROM_DEVICE_PATH,
        (4,  3): VENDOR_DEVICE_PATH,
        (4,  4): EFI_DEVICE_PATH,
        (4,  5): MEDIA_PROTOCOL_DEVICE_PATH,
        (4,  6): MEDIA_FW_VOL_FILEPATH_DEVICE_PATH,
        (4,  7): MEDIA_FW_VOL_DEVICE_PATH,
        (4,  8): MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH,
        (4,  9): MEDIA_RAM_DISK_DEVICE_PATH,

        # BBS_DEVICE_PATH
        (5, 1): BBS_BBS_DEVICE_PATH,

    }

    guid_override_dict = {
        uuid.UUID('37499A9D-542F-4C89-A026-35DA142094E4'):
            UART_FLOW_CONTROL_DEVICE_PATH,
        uuid.UUID('D487DDB4-008B-11D9-AFDC-001083FFCA4D'):
            SAS_DEVICE_PATH,
    }

    def __init__(self, file, ptr=None, verbose=False, count=64):
        '''
        Convert ptr into a list of Device Path nodes. If verbose also hexdump
        extra data.
        '''
        self._file = file
        self._verbose = verbose
        if ptr is None:
            return

        try:
            instance = []
            for _ in range(count):  # while True
                hdr, _ = self._ctype_read_ex(EFI_DEVICE_PATH, ptr)
                if hdr.Length < sizeof(EFI_DEVICE_PATH):
                    # Not a valid device path
                    break

                if hdr.Type == 0x7F:  # END_DEVICE_PATH_TYPE
                    self.DevicePath.append(instance)
                    if hdr.SubType == 0xFF:  # END_ENTIRE_DEVICE_PATH_SUBTYPE
                        break
                    if hdr.SubType == 0x01:  # END_INSTANCE_DEVICE_PATH_SUBTYPE
                        # start new device path instance
                        instance = []

                type_str = self.device_path_dict.get(
                    (hdr.Type, hdr.SubType), EFI_DEVICE_PATH)
                node, extra = self._ctype_read_ex(type_str, ptr, hdr.Length)
                if 'VENDOR_DEVICE_PATH' in type(node).__name__:
                    guid_type = self.guid_override_dict.get(
                                        GuidNames.to_uuid(node.Guid), None)
                    if guid_type:
                        # use the ctype associated with the GUID
                        node, extra = self._ctype_read_ex(
                                                guid_type, ptr, hdr.Length)

                instance.append((type(node).__name__, hdr.Type,
                                hdr.SubType, hdr.Length, node, extra))
                ptr += hdr.Length
        except ValueError:
            pass

    def __str__(self):
        ''' '''
        if not self.valid():
            return '<class: EfiDevicePath>'

        result = ""
        for instance in self.DevicePath:
            for (Name, Type, SubType, Length, cnode, extra) in instance:
                result += f'{Name:s} {Type:2d}:{SubType:2d} Len: {Length:3d}\n'
                result += ctype_to_str(cnode, '  ', ['Reserved'])
                if self._verbose:
                    if extra is not None:
                        result += hexdump(extra, '    ')
            result += '\n'

        return result

    def valid(self):
        return True if self.DevicePath else False

    def device_path_node(self, address):
        try:
            hdr, _ = self._ctype_read_ex(EFI_DEVICE_PATH, address)
            if hdr.Length < sizeof(EFI_DEVICE_PATH):
                return None, None, None

            type_str = self.device_path_dict.get(
                (hdr.Type, hdr.SubType), EFI_DEVICE_PATH)
            cnode, extra = self._ctype_read_ex(type_str, address, hdr.Length)
            return hdr, cnode, extra
        except ValueError:
            return None, None, None

    def device_path_node_str(self, address, verbose=False):
        hdr, cnode, extra = self.device_path_node(address)
        if hdr is None:
            return ''

        cname = type(cnode).__name__
        result = f'{cname:s} {hdr.Type:2d}:{hdr.SubType:2d} '
        result += f'Len: 0x{hdr.Length:03x}\n'
        result += ctype_to_str(cnode, '  ', ['Reserved'])
        if verbose:
            if extra is not None:
                result += hexdump(extra, '    ')

        return result

    def _ctype_read_ex(self, ctype_struct, offset=0, rsize=None):
        if offset != 0:
            self._file.seek(offset)

        type_size = sizeof(ctype_struct)
        size = rsize if rsize else type_size
        data = self._file.read(size)
        if data is None:
            return None, None

        cdata = ctype_struct.from_buffer(bytearray(data))

        if size > type_size:
            return cdata, data[type_size:]
        else:
            return cdata, None


class EfiConfigurationTable:
    '''
    A class to abstract EFI Configuration Tables from gST->ConfigurationTable
    and gST->NumberOfTableEntries. Pass in the gST pointer from EFI,
    likely you need to look up this address after you have loaded symbols

    Attributes
    ??????
    ConfigurationTableDict : dictionary
        dictionary of EFI Configuration Table entries

    Methods
    -----------
    GetConfigTable(uuid)
        pass in VendorGuid and return VendorTable from EFI System Table
    DebugImageInfo(table)
        return tuple of load address and size of PE/COFF images
    '''

    ConfigurationTableDict = {}

    def __init__(self, file, gST_addr=None):
        self._file = file
        if gST_addr is None:
            # search for gST via EFI_SYSTEM_TABLE_POINTER
            system_table_pointer = self._get_system_table_pointer()
            if not system_table_pointer is None:
                gST_addr = system_table_pointer.EfiSystemTableBase
            else:
                return

        gST = self._ctype_read(EFI_SYSTEM_TABLE, gST_addr)
        self.read_efi_config_table(gST.NumberOfTableEntries,
                                   gST.ConfigurationTable,
                                   self._ctype_read)

    @ classmethod
    def __str__(cls):
        '''return EFI_CONFIGURATION_TABLE entries as a string'''
        result = ""
        for key, value in cls.ConfigurationTableDict.items():
            result += f'{GuidNames().to_name(key):>37s}: '
            result += f'VendorTable = 0x{value:08x}\n'

        return result

    def _ctype_read(self, ctype_struct, offset=0):
        '''ctype worker function to read data'''
        if offset != 0:
            self._file.seek(offset)

        data = self._file.read(sizeof(ctype_struct))
        return ctype_struct.from_buffer(bytearray(data))

    def _get_system_table_pointer(self):
        start_addr = 0x0
        end_addr = 0xf0000000
        alignment = 0x00400000
        # The translation of "IBI SYST" for the signature of EFI_SYSTEM_TABLE
        efi_system_table_signature = 0x5453595320494249

        system_table_pointer = None

        current_addr = start_addr
        while current_addr < end_addr:
            try:
                self._file.seek(current_addr)
                data = self._file.read(sizeof(EFI_SYSTEM_TABLE_POINTER))
            except:
                current_addr = current_addr + alignment
                continue

            system_table_pointer = EFI_SYSTEM_TABLE_POINTER.from_buffer(bytearray(data))
            if system_table_pointer.Signature == efi_system_table_signature:
                buf1 = bytearray(system_table_pointer)[:16]
                crc32_value = zlib.crc32(buf1)
                crc32_value = zlib.crc32(b'\0' * (sizeof(EFI_SYSTEM_TABLE_POINTER) - len(buf1)), crc32_value)
                if crc32_value == system_table_pointer.Crc32:
                    break
            system_table_pointer = None
            current_addr = current_addr + alignment
        return system_table_pointer

    @ classmethod
    def read_efi_config_table(cls, table_cnt, table_ptr, ctype_read):
        '''Create a dictionary of EFI Configuration table entries'''
        EmptryTables = EFI_CONFIGURATION_TABLE * table_cnt
        Tables = ctype_read(EmptryTables, table_ptr)
        for i in range(table_cnt):
            cls.ConfigurationTableDict[str(GuidNames.to_uuid(
                Tables[i].VendorGuid)).upper()] = Tables[i].VendorTable

        return cls.ConfigurationTableDict

    def GetConfigTable(self, uuid):
        ''' Return VendorTable for VendorGuid (uuid.UUID) or None'''
        return self.ConfigurationTableDict.get(uuid.upper())

    def DebugImageInfo(self, table=None):
        '''
        Walk the debug image info table to find the LoadedImage protocols
        for all the loaded PE/COFF images and return a list of load address
        and image size.
        '''
        ImageLoad = []

        if table is None:
            table = self.GetConfigTable('49152e77-1ada-4764-b7a2-7afefed95e8b')

        DbgInfoHdr = self._ctype_read(EFI_DEBUG_IMAGE_INFO_TABLE_HEADER, table)
        NormalImageArray = EFI_DEBUG_IMAGE_INFO * DbgInfoHdr.TableSize
        NormalImageArray = self._ctype_read(
            NormalImageArray, DbgInfoHdr.EfiDebugImageInfoTable)
        for i in range(DbgInfoHdr.TableSize):
            ImageInfo = self._ctype_read(
                EFI_DEBUG_IMAGE_INFO_NORMAL, NormalImageArray[i].NormalImage)
            LoadedImage = self._ctype_read(
                EFI_LOADED_IMAGE_PROTOCOL,
                ImageInfo.LoadedImageProtocolInstance)
            ImageLoad.append((LoadedImage.ImageBase, LoadedImage.ImageSize))

        return ImageLoad


class PeTeImage:
    '''
    A class to abstract PE/COFF or TE image processing via passing in a
    Python file like object. If you pass in an address the PE/COFF is parsed,
    if you pass in NULL for an address then you get a class instance you can
    use to search memory for a PE/COFF hader given a pc value.

    Attributes
    ??????
    LoadAddress : int
        Load address of the PE/COFF image
    AddressOfEntryPoint : int
        Address of the Entry point of the PE/COFF image
    TextAddress : int
        Start of the PE/COFF text section
    DataAddress : int
        Start of the PE/COFF data section
    CodeViewPdb : str
        File name of the symbols file
    CodeViewUuid : uuid:UUID
        GUID for "RSDS" Debug Directory entry, or Mach-O UUID for "MTOC"

    Methods
    -----------
    pcToPeCoff(address, step, max_range, rom_range)
        Given an address(pc) find the PE/COFF image it is in
    sections_to_str()
        return a string giving info for all the PE/COFF sections
    '''

    def __init__(self, file, address=0):
        self._file = file

        # book keeping, but public
        self.PeHdr = None
        self.TeHdr = None
        self.Machine = None
        self.Subsystem = None
        self.CodeViewSig = None
        self.e_lfanew = 0
        self.NumberOfSections = 0
        self.Sections = None

        # Things debuggers may want to know
        self.LoadAddress = 0 if address is None else address
        self.EndLoadAddress = 0
        self.AddressOfEntryPoint = 0
        self.TextAddress = 0
        self.DataAddress = 0
        self.CodeViewPdb = None
        self.CodeViewUuid = None
        self.TeAdjust = 0

        self.dir_name = {
            0: 'Export Table',
            1: 'Import Table',
            2: 'Resource Table',
            3: 'Exception Table',
            4: 'Certificate Table',
            5: 'Relocation Table',
            6: 'Debug',
            7: 'Architecture',
            8: 'Global Ptr',
            9: 'TLS Table',
            10: 'Load Config Table',
            11: 'Bound Import',
            12: 'IAT',
            13: 'Delay Import Descriptor',
            14: 'CLR Runtime Header',
            15: 'Reserved',
        }

        if address is not None:
            if self.maybe():
                self.parse()

    def __str__(self):
        if self.PeHdr is None and self.TeHdr is None:
            # no PE/COFF header found
            return "<class: PeTeImage>"

        if self.CodeViewPdb:
            pdb = f'{self.Machine}`{self.CodeViewPdb}'
        else:
            pdb = 'No Debug Info:'

        if self.CodeViewUuid:
            guid = f'{self.CodeViewUuid}:'
        else:
            guid = ''

        slide = f'slide = {self.TeAdjust:d} ' if self.TeAdjust != 0 else ' '
        res = guid + f'{pdb} load = 0x{self.LoadAddress:08x} ' + slide
        return res

    def _seek(self, offset):
        """
        seek() relative to start of PE/COFF (TE) image
        """
        self._file.seek(self.LoadAddress + offset)

    def _read_offset(self, size, offset=None):
        """
        read() relative to start of PE/COFF (TE) image
        if offset is not None then seek() before the read
        """
        if offset is not None:
            self._seek(offset)

        return self._file.read(size)

    def _read_ctype(self, ctype_struct, offset=None):
        data = self._read_offset(sizeof(ctype_struct), offset)
        return ctype_struct.from_buffer(bytearray(data), 0)

    def _unsigned(self, i):
        """return a 32-bit unsigned int (UINT32) """
        return int.from_bytes(i, byteorder='little', signed=False)

    def pcToPeCoff(self,
                   address,
                   step=None,
                   max_range=None,
                   rom_range=[0xFE800000, 0xFFFFFFFF]):
        """
        Given an address search backwards for PE/COFF (TE) header
        For DXE 4K is probably OK
        For PEI you might have to search every 4 bytes.
        """
        if step is None:
            step = 0x1000

        if max_range is None:
            max_range = 0x200000

        if address in range(*rom_range):
            # The XIP code in the ROM ends up 4 byte aligned.
            step = 4
            max_range = min(max_range, 0x100000)

        # Align address to page boundary for memory image search.
        address = address & ~(step-1)
        # Search every step backward
        offset_range = list(range(0, min(max_range, address), step))
        for offset in offset_range:
            if self.maybe(address - offset):
                if self.parse():
                    return True

        return False

    def maybe(self, offset=None):
        """Probe to see if this offset is likely a PE/COFF or TE file """
        self.LoadAddress = 0
        e_magic = self._read_offset(2, offset)
        header_ok = e_magic == b'MZ' or e_magic == b'VZ'
        if offset is not None and header_ok:
            self.LoadAddress = offset
        return header_ok

    def parse(self):
        """Parse PE/COFF (TE) debug directory entry """
        DosHdr = self._read_ctype(EFI_IMAGE_DOS_HEADER, 0)
        if DosHdr.e_magic == self._unsigned(b'VZ'):
            # TE image
            self.TeHdr = self._read_ctype(EFI_TE_IMAGE_HEADER, 0)

            self.TeAdjust = sizeof(self.TeHdr) - self.TeHdr.StrippedSize
            self.Machine = image_machine_dict.get(self.TeHdr.Machine, None)
            self.Subsystem = self.TeHdr.Subsystem
            self.AddressOfEntryPoint = self.TeHdr.AddressOfEntryPoint

            debug_dir_size = self.TeHdr.DataDirectoryDebug.Size
            debug_dir_offset = (self.TeAdjust +
                                self.TeHdr.DataDirectoryDebug.VirtualAddress)
        else:
            if DosHdr.e_magic == self._unsigned(b'MZ'):
                self.e_lfanew = DosHdr.e_lfanew
            else:
                self.e_lfanew = 0

            self.PeHdr = self._read_ctype(
                EFI_IMAGE_NT_HEADERS64, self.e_lfanew)
            if self.PeHdr.Signature != self._unsigned(b'PE\0\0'):
                return False

            if self.PeHdr.OptionalHeader.Magic == \
                    EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC:
                self.PeHdr = self._read_ctype(
                    EFI_IMAGE_NT_HEADERS32, self.e_lfanew)

            if self.PeHdr.OptionalHeader.NumberOfRvaAndSizes <= \
                    DIRECTORY_DEBUG:
                return False

            self.Machine = image_machine_dict.get(
                self.PeHdr.FileHeader.Machine, None)
            self.Subsystem = self.PeHdr.OptionalHeader.Subsystem
            self.AddressOfEntryPoint = \
                self.PeHdr.OptionalHeader.AddressOfEntryPoint
            self.TeAdjust = 0

            debug_dir_size = self.PeHdr.OptionalHeader.DataDirectory[
                DIRECTORY_DEBUG].Size
            debug_dir_offset = self.PeHdr.OptionalHeader.DataDirectory[
                DIRECTORY_DEBUG].VirtualAddress

        if self.Machine is None or self.Subsystem not in [0, 10, 11, 12]:
            return False

        self.AddressOfEntryPoint += self.LoadAddress

        self.sections()
        return self.processDebugDirEntry(debug_dir_offset, debug_dir_size)

    def sections(self):
        '''Parse the PE/COFF (TE) section table'''
        if self.Sections is not None:
            return
        elif self.TeHdr is not None:
            self.NumberOfSections = self.TeHdr.NumberOfSections
            offset = sizeof(EFI_TE_IMAGE_HEADER)
        elif self.PeHdr is not None:
            self.NumberOfSections = self.PeHdr.FileHeader.NumberOfSections
            offset = sizeof(c_uint32) + \
                sizeof(EFI_IMAGE_FILE_HEADER)
            offset += self.PeHdr.FileHeader.SizeOfOptionalHeader
            offset += self.e_lfanew
        else:
            return

        self.Sections = EFI_IMAGE_SECTION_HEADER * self.NumberOfSections
        self.Sections = self._read_ctype(self.Sections, offset)

        for i in range(self.NumberOfSections):
            name = str(self.Sections[i].Name, 'ascii', 'ignore')
            addr = self.Sections[i].VirtualAddress
            addr += self.LoadAddress + self.TeAdjust
            if name == '.text':
                self.TextAddress = addr
            elif name == '.data':
                self.DataAddress = addr

            end_addr = addr + self.Sections[i].VirtualSize - 1
            if end_addr > self.EndLoadAddress:
                self.EndLoadAddress = end_addr

    def sections_to_str(self):
        # return text summary of sections
        # name virt addr (virt size) flags:Characteristics
        result = ''
        for i in range(self.NumberOfSections):
            name = str(self.Sections[i].Name, 'ascii', 'ignore')
            result += f'{name:8s} '
            result += f'0x{self.Sections[i].VirtualAddress:08X} '
            result += f'(0x{self.Sections[i].VirtualSize:05X}) '
            result += f'flags:0x{self.Sections[i].Characteristics:08X}\n'

        return result

    def directory_to_str(self):
        result = ''
        if self.TeHdr:
            debug_size = self.TeHdr.DataDirectoryDebug.Size
            if debug_size > 0:
                debug_offset = (self.TeAdjust
                                + self.TeHdr.DataDirectoryDebug.VirtualAddress)
                result += f"Debug 0x{debug_offset:08X} 0x{debug_size}\n"

            relocation_size = self.TeHdr.DataDirectoryBaseReloc.Size
            if relocation_size > 0:
                relocation_offset = (
                    self.TeAdjust
                    + self.TeHdr.DataDirectoryBaseReloc.VirtualAddress)
                result += f'Relocation 0x{relocation_offset:08X} '
                result += f' 0x{relocation_size}\n'

        elif self.PeHdr:
            for i in range(self.PeHdr.OptionalHeader.NumberOfRvaAndSizes):
                size = self.PeHdr.OptionalHeader.DataDirectory[i].Size
                if size == 0:
                    continue

                virt_addr = self.PeHdr.OptionalHeader.DataDirectory[
                    i].VirtualAddress
                name = self.dir_name.get(i, '?')
                result += f'{name:s} 0x{virt_addr:08X} 0x{size:X}\n'

        return result

    def processDebugDirEntry(self, virt_address, virt_size):
        """Process PE/COFF Debug Directory Entry"""
        if (virt_address == 0 or
                virt_size < sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)):
            return False

        data = bytearray(self._read_offset(virt_size, virt_address))
        for offset in range(0,
                            virt_size,
                            sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)):
            DirectoryEntry = EFI_IMAGE_DEBUG_DIRECTORY_ENTRY.from_buffer(
                data[offset:])
            if DirectoryEntry.Type != 2:
                continue

            entry = self._read_offset(
                DirectoryEntry.SizeOfData, DirectoryEntry.RVA + self.TeAdjust)
            self.CodeViewSig = entry[:4]
            if self.CodeViewSig == b'MTOC':
                self.CodeViewUuid = uuid.UUID(bytes_le=entry[4:4+16])
                PdbOffset = 20
            elif self.CodeViewSig == b'RSDS':
                self.CodeViewUuid = uuid.UUID(bytes_le=entry[4:4+16])
                PdbOffset = 24
            elif self.CodeViewSig == b'NB10':
                PdbOffset = 16
            else:
                continue

            # can't find documentation about Pdb string encoding?
            # guessing utf-8 since that will match file systems in macOS
            # and Linux Windows is UTF-16, or ANSI adjusted for local.
            # We might need a different value for Windows here?
            self.CodeViewPdb = entry[PdbOffset:].split(b'\x00')[
                0].decode('utf-8')
            return True
        return False


def main():
    '''Process arguments as PE/COFF files'''
    for fname in sys.argv[1:]:
        with open(fname, 'rb') as f:
            image = PeTeImage(f)
            print(image)
            res = f'EntryPoint = 0x{image.AddressOfEntryPoint:08x}  '
            res += f'TextAddress = 0x{image.TextAddress:08x} '
            res += f'DataAddress = 0x{image.DataAddress:08x}'
            print(res)
            print(image.sections_to_str())
            print('Data Directories:')
            print(image.directory_to_str())


if __name__ == "__main__":
    main()
