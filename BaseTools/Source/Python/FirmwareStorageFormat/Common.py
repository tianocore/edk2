## @file
# This file is used to define the common C struct and functions.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from ctypes import *
from functools import reduce
import uuid

# ZeroGuid = uuid.UUID('{00000000-0000-0000-0000-000000000000}')
# EFI_FIRMWARE_FILE_SYSTEM2_GUID = uuid.UUID('{8C8CE578-8A3D-4f1c-9935-896185C32DD3}')
# EFI_FIRMWARE_FILE_SYSTEM3_GUID = uuid.UUID('{5473C07A-3DCB-4dca-BD6F-1E9689E7349A}')
# EFI_FFS_VOLUME_TOP_FILE_GUID = uuid.UUID('{1BA0062E-C779-4582-8566-336AE8F78F09}')

EFI_FIRMWARE_FILE_SYSTEM2_GUID = uuid.UUID("8c8ce578-8a3d-4f1c-9935-896185c32dd3")
EFI_FIRMWARE_FILE_SYSTEM2_GUID_BYTE = b'x\xe5\x8c\x8c=\x8a\x1cO\x995\x89a\x85\xc3-\xd3'
# EFI_FIRMWARE_FILE_SYSTEM2_GUID_BYTE = EFI_FIRMWARE_FILE_SYSTEM2_GUID.bytes
EFI_FIRMWARE_FILE_SYSTEM3_GUID = uuid.UUID("5473C07A-3DCB-4dca-BD6F-1E9689E7349A")
# EFI_FIRMWARE_FILE_SYSTEM3_GUID_BYTE = b'x\xe5\x8c\x8c=\x8a\x1cO\x995\x89a\x85\xc3-\xd3'
EFI_FIRMWARE_FILE_SYSTEM3_GUID_BYTE = b'z\xc0sT\xcb=\xcaM\xbdo\x1e\x96\x89\xe74\x9a'
EFI_SYSTEM_NVDATA_FV_GUID = uuid.UUID("fff12b8d-7696-4c8b-a985-2747075b4f50")
EFI_SYSTEM_NVDATA_FV_GUID_BYTE = b"\x8d+\xf1\xff\x96v\x8bL\xa9\x85'G\x07[OP"
EFI_FFS_VOLUME_TOP_FILE_GUID = uuid.UUID("1ba0062e-c779-4582-8566-336ae8f78f09")
EFI_FFS_VOLUME_TOP_FILE_GUID_BYTE = b'.\x06\xa0\x1by\xc7\x82E\x85f3j\xe8\xf7\x8f\t'
ZEROVECTOR_BYTE = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'
PADVECTOR = uuid.UUID("ffffffff-ffff-ffff-ffff-ffffffffffff")
EFI_FSP_GUID = uuid.UUID("912740be-2284-4734-b971-84b027353f0c")

FVH_SIGNATURE = b'_FVH'

#Alignment
SECTION_COMMON_ALIGNMENT = 4
FFS_COMMON_ALIGNMENT = 8

class GUID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Guid1',            c_uint32),
        ('Guid2',            c_uint16),
        ('Guid3',            c_uint16),
        ('Guid4',            ARRAY(c_uint8, 8)),
    ]

    def from_list(self, listformat: list) -> None:
        self.Guid1 = listformat[0]
        self.Guid2 = listformat[1]
        self.Guid3 = listformat[2]
        for i in range(8):
            self.Guid4[i] = listformat[i+3]

    def __cmp__(self, otherguid) -> bool:
        if not isinstance(otherguid, GUID):
            return 'Input is not the GUID instance!'
        rt = False
        if self.Guid1 == otherguid.Guid1 and self.Guid2 == otherguid.Guid2 and self.Guid3 == otherguid.Guid3:
            rt = True
            for i in range(8):
                rt = rt & (self.Guid4[i] == otherguid.Guid4[i])
        return rt

def ModifyGuidFormat(target_guid: str) -> GUID:
    target_guid = target_guid.replace('-', '')
    target_list = []
    start = [0,8,12,16,18,20,22,24,26,28,30]
    end = [8,12,16,18,20,22,24,26,28,30,32]
    num = len(start)
    for pos in range(num):
        new_value = int(target_guid[start[pos]:end[pos]], 16)
        target_list.append(new_value)
    new_format = GUID()
    new_format.from_list(target_list)
    return new_format


# Get data from ctypes to bytes.
def struct2stream(s) -> bytes:
    length = sizeof(s)
    p = cast(pointer(s), POINTER(c_char * length))
    return p.contents.raw



def GetPadSize(Size: int, alignment: int) -> int:
    if Size % alignment == 0:
        return 0
    Pad_Size = alignment - Size % alignment
    return Pad_Size

def Bytes2Val (bytes):
    return reduce(lambda x,y: (x<<8)|y,  bytes[::-1] )

def Val2Bytes (value, blen):
    BytesList = [hex((value>>(i*8) & 0xff)) for i in range(blen)]
    FinalBytes = b''
    for item in BytesList:
        FinalBytes += item
    return FinalBytes
