## @file
# This file is used to define the FV Header C Struct.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from ast import Str
from struct import *
from ctypes import *
from FirmwareStorageFormat.Common import *

class EFI_FV_BLOCK_MAP_ENTRY(Structure):
    _pack_ = 1
    _fields_ = [
        ('NumBlocks',            c_uint32),
        ('Length',               c_uint32),
    ]


class EFI_FIRMWARE_VOLUME_HEADER(Structure):
    _fields_ = [
        ('ZeroVector',           ARRAY(c_uint8, 16)),
        ('FileSystemGuid',       GUID),
        ('FvLength',             c_uint64),
        ('Signature',            c_uint32),
        ('Attributes',           c_uint32),
        ('HeaderLength',         c_uint16),
        ('Checksum',             c_uint16),
        ('ExtHeaderOffset',      c_uint16),
        ('Reserved',             c_uint8),
        ('Revision',             c_uint8),
        ('BlockMap',             ARRAY(EFI_FV_BLOCK_MAP_ENTRY, 1)),
        ]

def Refine_FV_Header(nums):
    class EFI_FIRMWARE_VOLUME_HEADER(Structure):
        _fields_ = [
            ('ZeroVector',           ARRAY(c_uint8, 16)),
            ('FileSystemGuid',       GUID),
            ('FvLength',             c_uint64),
            ('Signature',            c_uint32),
            ('Attributes',           c_uint32),
            ('HeaderLength',         c_uint16),
            ('Checksum',             c_uint16),
            ('ExtHeaderOffset',      c_uint16),
            ('Reserved',             c_uint8),
            ('Revision',             c_uint8),
            ('BlockMap',             ARRAY(EFI_FV_BLOCK_MAP_ENTRY, nums)),
            ]
    return EFI_FIRMWARE_VOLUME_HEADER

class EFI_FIRMWARE_VOLUME_EXT_HEADER(Structure):
    _fields_ = [
        ('FvName',               GUID),
        ('ExtHeaderSize',        c_uint32)
        ]

class EFI_FIRMWARE_VOLUME_EXT_ENTRY(Structure):
    _fields_ = [
        ('ExtEntrySize',         c_uint16),
        ('ExtEntryType',         c_uint16)
        ]

class EFI_FIRMWARE_VOLUME_EXT_ENTRY_OEM_TYPE_0(Structure):
    _fields_ = [
        ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
        ('TypeMask',             c_uint32)
        ]

class EFI_FIRMWARE_VOLUME_EXT_ENTRY_OEM_TYPE(Structure):
    _fields_ = [
        ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
        ('TypeMask',             c_uint32),
        ('Types',                ARRAY(GUID, 1))
        ]

def Refine_FV_EXT_ENTRY_OEM_TYPE_Header(nums: int) -> EFI_FIRMWARE_VOLUME_EXT_ENTRY_OEM_TYPE:
    class EFI_FIRMWARE_VOLUME_EXT_ENTRY_OEM_TYPE(Structure):
        _fields_ = [
            ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
            ('TypeMask',             c_uint32),
            ('Types',                ARRAY(GUID, nums))
        ]
    return EFI_FIRMWARE_VOLUME_EXT_ENTRY_OEM_TYPE(Structure)

class EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE_0(Structure):
    _fields_ = [
        ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
        ('FormatType',           GUID)
        ]

class EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE(Structure):
    _fields_ = [
        ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
        ('FormatType',           GUID),
        ('Data',                 ARRAY(c_uint8, 1))
        ]

def Refine_FV_EXT_ENTRY_GUID_TYPE_Header(nums: int) -> EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE:
    class EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE(Structure):
        _fields_ = [
            ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
            ('FormatType',           GUID),
            ('Data',                 ARRAY(c_uint8, nums))
        ]
    return EFI_FIRMWARE_VOLUME_EXT_ENTRY_GUID_TYPE(Structure)

class EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE(Structure):
    _fields_ = [
        ('Hdr',                  EFI_FIRMWARE_VOLUME_EXT_ENTRY),
        ('UsedSize',             c_uint32)
        ]
