## @file
# This file is used to define the Ffs Header C Struct.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from struct import *
from ctypes import *
from FirmwareStorageFormat.Common import *

EFI_FFS_FILE_HEADER_LEN = 24
EFI_FFS_FILE_HEADER2_LEN = 32

## File Types Definitions

EFI_FV_FILETYPE_ALL                   = 0x00
EFI_FV_FILETYPE_RAW                   = 0x01
EFI_FV_FILETYPE_FREEFORM              = 0x02
EFI_FV_FILETYPE_SECURITY_CORE         = 0x03
EFI_FV_FILETYPE_PEI_CORE              = 0x04
EFI_FV_FILETYPE_DXE_CORE              = 0x05
EFI_FV_FILETYPE_PEIM                  = 0x06
EFI_FV_FILETYPE_DRIVER                = 0x07
EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  = 0x08
EFI_FV_FILETYPE_APPLICATION           = 0x09
EFI_FV_FILETYPE_SMM                   = 0x0A
EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE = 0x0B
EFI_FV_FILETYPE_COMBINED_SMM_DXE      = 0x0C
EFI_FV_FILETYPE_SMM_CORE              = 0x0D
EFI_FV_FILETYPE_MM_STANDALONE         = 0x0E
EFI_FV_FILETYPE_MM_CORE_STANDALONE    = 0x0F
EFI_FV_FILETYPE_OEM_MIN               = 0xc0
EFI_FV_FILETYPE_OEM_MAX               = 0xdf
EFI_FV_FILETYPE_DEBUG_MIN             = 0xe0
EFI_FV_FILETYPE_DEBUG_MAX             = 0xef
EFI_FV_FILETYPE_FFS_MIN               = 0xf0
EFI_FV_FILETYPE_FFS_MAX               = 0xff
EFI_FV_FILETYPE_FFS_PAD               = 0xf0

## FFS File Attributes.

FFS_ATTRIB_LARGE_FILE         = 0x01
FFS_ATTRIB_DATA_ALIGNMENT2    = 0x02
FFS_ATTRIB_FIXED              = 0x04
FFS_ATTRIB_DATA_ALIGNMENT     = 0x38
FFS_ATTRIB_CHECKSUM           = 0x40

class CHECK_SUM(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header',                   c_uint8),
        ('File',                     c_uint8),
    ]

class EFI_FFS_INTEGRITY_CHECK(Union):
    _pack_ = 1
    _fields_ = [
        ('Checksum',                 CHECK_SUM),
        ('Checksum16',               c_uint16),
    ]


class EFI_FFS_FILE_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Name',                     GUID),
        ('IntegrityCheck',           EFI_FFS_INTEGRITY_CHECK),
        ('Type',                     c_uint8),
        ('Attributes',               c_uint8),
        ('Size',                     ARRAY(c_uint8, 3)),
        ('State',                    c_uint8),
    ]

    @property
    def FFS_FILE_SIZE(self) -> int:
        return self.Size[0] | self.Size[1] << 8 | self.Size[2] << 16

    @property
    def HeaderLength(self) -> int:
        return 24

class EFI_FFS_FILE_HEADER2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Name',                     GUID),
        ('IntegrityCheck',           EFI_FFS_INTEGRITY_CHECK),
        ('Type',                     c_uint8),
        ('Attributes',               c_uint8),
        ('Size',                     ARRAY(c_uint8, 3)),
        ('State',                    c_uint8),
        ('ExtendedSize',             c_uint64),
    ]

    @property
    def FFS_FILE_SIZE(self) -> int:
        return self.ExtendedSize

    @property
    def HeaderLength(self) -> int:
        return 32
