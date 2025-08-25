## @file
# This file is used to define the Section Header C Struct.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from struct import *
from ctypes import *
from FirmwareStorageFormat.Common import *

EFI_COMMON_SECTION_HEADER_LEN = 4
EFI_COMMON_SECTION_HEADER2_LEN = 8

EFI_SECTION_ALL                   = 0x00

EFI_SECTION_COMPRESSION           = 0x01
EFI_SECTION_GUID_DEFINED          = 0x02

EFI_SECTION_PE32                  = 0x10
EFI_SECTION_PIC                   = 0x11
EFI_SECTION_TE                    = 0x12
EFI_SECTION_DXE_DEPEX             = 0x13
EFI_SECTION_VERSION               = 0x14
EFI_SECTION_USER_INTERFACE        = 0x15
EFI_SECTION_COMPATIBILITY16       = 0x16
EFI_SECTION_FIRMWARE_VOLUME_IMAGE = 0x17
EFI_SECTION_FREEFORM_SUBTYPE_GUID = 0x18
EFI_SECTION_RAW                   = 0x19
EFI_SECTION_PEI_DEPEX             = 0x1B
EFI_SECTION_SMM_DEPEX             = 0x1C

class EFI_COMMON_SECTION_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Size',                     ARRAY(c_uint8, 3)),
        ('Type',                     c_uint8),
    ]

    @property
    def SECTION_SIZE(self) -> int:
        return self.Size[0] | self.Size[1] << 8 | self.Size[2] << 16

    def Common_Header_Size(self) -> int:
        return 4

class EFI_COMMON_SECTION_HEADER2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Size',                     ARRAY(c_uint8, 3)),
        ('Type',                     c_uint8),
        ('ExtendedSize',             c_uint32),
    ]

    @property
    def SECTION_SIZE(self) -> int:
        return self.ExtendedSize

    def Common_Header_Size(self) -> int:
        return 8

class EFI_COMPRESSION_SECTION(Structure):
    _pack_ = 1
    _fields_ = [
        ('UncompressedLength',       c_uint32),
        ('CompressionType',          c_uint8),
    ]

    def ExtHeaderSize(self) -> int:
        return 5

class EFI_FREEFORM_SUBTYPE_GUID_SECTION(Structure):
    _pack_ = 1
    _fields_ = [
        ('SubTypeGuid',              GUID),
    ]

    def ExtHeaderSize(self) -> int:
        return 16

class EFI_GUID_DEFINED_SECTION(Structure):
    _pack_ = 1
    _fields_ = [
        ('SectionDefinitionGuid',    GUID),
        ('DataOffset',               c_uint16),
        ('Attributes',               c_uint16),
    ]

    def ExtHeaderSize(self) -> int:
        return 20

def Get_USER_INTERFACE_Header(nums: int):
    class EFI_SECTION_USER_INTERFACE(Structure):
        _pack_ = 1
        _fields_ = [
            ('FileNameString',       ARRAY(c_uint16, nums)),
        ]

        def ExtHeaderSize(self) -> int:
            return 2 * nums

        def GetUiString(self) -> str:
            UiString = ''
            for i in range(nums):
                if self.FileNameString[i]:
                    UiString += chr(self.FileNameString[i])
            return UiString

    return EFI_SECTION_USER_INTERFACE

def Get_VERSION_Header(nums: int):
    class EFI_SECTION_VERSION(Structure):
        _pack_ = 1
        _fields_ = [
            ('BuildNumber',          c_uint16),
            ('VersionString',        ARRAY(c_uint16, nums)),
        ]

        def ExtHeaderSize(self) -> int:
            return 2 * (nums+1)

        def GetVersionString(self) -> str:
            VersionString = ''
            for i in range(nums):
                if self.VersionString[i]:
                    VersionString += chr(self.VersionString[i])
            return VersionString

    return EFI_SECTION_VERSION
