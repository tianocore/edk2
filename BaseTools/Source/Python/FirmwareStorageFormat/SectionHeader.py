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
