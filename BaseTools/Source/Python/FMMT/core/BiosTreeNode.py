## @file
# This file is used to define the BIOS Tree Node.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from FirmwareStorageFormat.UPLHeader import *
from FirmwareStorageFormat.FvHeader import *
from FirmwareStorageFormat.FfsFileHeader import *
from FirmwareStorageFormat.SectionHeader import *
from FirmwareStorageFormat.Common import *
from utils.FmmtLogger import FmmtLogger as logger
import uuid

SectionHeaderType = {
    0x01:'EFI_COMPRESSION_SECTION',
    0x02:'EFI_GUID_DEFINED_SECTION',
    0x03:'EFI_SECTION_DISPOSABLE',
    0x10:'EFI_SECTION_PE32',
    0x11:'EFI_SECTION_PIC',
    0x12:'EFI_SECTION_TE',
    0x13:'EFI_SECTION_DXE_DEPEX',
    0x14:'EFI_SECTION_VERSION',
    0x15:'EFI_SECTION_USER_INTERFACE',
    0x16:'EFI_SECTION_COMPATIBILITY16',
    0x17:'EFI_SECTION_FIRMWARE_VOLUME_IMAGE',
    0x18:'EFI_FREEFORM_SUBTYPE_GUID_SECTION',
    0x19:'EFI_SECTION_RAW',
    0x1B:'EFI_SECTION_PEI_DEPEX',
    0x1C:'EFI_SECTION_MM_DEPEX'
}
HeaderType = [0x01, 0x02, 0x14, 0x15, 0x18]

class BinaryNode:
    def __init__(self, name: str) -> None:
        self.Size = 0
        self.Name = "BINARY" + str(name)
        self.HOffset = 0
        self.Data = b''

class ElfNode:
    def __init__(self, buffer: bytes) -> None:
        self.Header = ELF_HEADER32.from_buffer_copy(buffer)
        if self.Header.ELF_Identification[0:4] != b'\x7fELF':
            logger.error('Invalid Elf Header! Elf Identification {} is not ".ELF".'.format(self.Header.ELF_Identification))
            raise Exception("Process Failed: Invalid ELF Header Identification!")
        self.Class = self.Header.ELF_Identification[4]
        if self.Class == 0x02:
            self.Header = ELF_HEADER64.from_buffer_copy(buffer)
        elif self.Class != 0x01:
            logger.error('Invalid Elf Class! Elf Class {} is not 0x01 or 0x02.'.format(self.Class))
            raise Exception("Process Failed: Invalid ELF Class!")

        self.ProList = []
        self.SecList = []
        self.UpldInfoSection = None
        self.UpldInfo = None
        self.UpldBuffer = b''
        self.Name = "ELF"
        self.HeaderLength = len(struct2stream(self.Header))
        self.HOffset = 0
        self.DOffset = 0
        self.ROffset = 0
        self.Data = b''
        self.PadData = b''
        self.Upld_Info_Align = False

    def GetProgramList(self, buffer: bytes) -> None:
        for i in range(self.Header.ELF_PHNum):
            if self.Class == 0x01:
                ElfProgramHeader = ELF_PROGRAM_HEADER32.from_buffer_copy(buffer[i*self.Header.ELF_PHEntSize:])
            elif self.Class == 0x02:
                ElfProgramHeader = ELF_PROGRAM_HEADER64.from_buffer_copy(buffer[i*self.Header.ELF_PHEntSize:])
            self.ProList.append(ElfProgramHeader)

    def GetSectionList(self, buffer: bytes) -> None:
        for i in range(self.Header.ELF_SHNum):
            if self.Class == 0x01:
                ElfSectionHeader = ELF_SECTION_HEADER32.from_buffer_copy(buffer[i*self.Header.ELF_SHEntSize:])
            elif self.Class == 0x02:
                ElfSectionHeader = ELF_SECTION_HEADER64.from_buffer_copy(buffer[i*self.Header.ELF_SHEntSize:])
            self.SecList.append(ElfSectionHeader)

    def FindUPLDSection(self, buffer: bytes) -> None:
        for item in self.SecList:
            if buffer[item.SH_Offset:item.SH_Offset+4] == b'PLDH' or buffer[item.SH_Offset:item.SH_Offset+4] == b'UPLD':
                self.UpldInfoSection = item
                self.UpldInfo = UNIVERSAL_PAYLOAD_INFO.from_buffer_copy(buffer[item.SH_Offset:item.SH_Offset+item.SH_Size])
                self.UpldBuffer = struct2stream(self.UpldInfo)
                if (self.UpldInfoSection.SH_Offset) % 4 == 0:
                # if (self.UpldInfoSection.SH_Offset - self.Header.ELF_Entry) % 4 == 0:
                    self.Upld_Info_Align = True

class FvNode:
    def __init__(self, name, buffer: bytes) -> None:
        self.Header = EFI_FIRMWARE_VOLUME_HEADER.from_buffer_copy(buffer)
        Map_num = (self.Header.HeaderLength - 56)//8
        self.Header = Refine_FV_Header(Map_num).from_buffer_copy(buffer)
        self.FvId = "FV" + str(name)
        self.Name = "FV" + str(name)
        if self.Header.ExtHeaderOffset:
            self.ExtHeader = EFI_FIRMWARE_VOLUME_EXT_HEADER.from_buffer_copy(buffer[self.Header.ExtHeaderOffset:])
            self.Name =  uuid.UUID(bytes_le=struct2stream(self.ExtHeader.FvName))
            self.ExtEntryOffset = self.Header.ExtHeaderOffset + 20
            if self.ExtHeader.ExtHeaderSize != 20:
                self.ExtEntryExist = 1
                self.ExtEntry = EFI_FIRMWARE_VOLUME_EXT_ENTRY.from_buffer_copy(buffer[self.ExtEntryOffset:])
                self.ExtTypeExist = 1
                if self.ExtEntry.ExtEntryType == 0x01:
                    nums = (self.ExtEntry.ExtEntrySize - 8) // 16
                    self.ExtEntry = Refine_FV_EXT_ENTRY_OEM_TYPE_Header(nums).from_buffer_copy(buffer[self.ExtEntryOffset:])
                elif self.ExtEntry.ExtEntryType == 0x02:
                    nums = self.ExtEntry.ExtEntrySize - 20
                    self.ExtEntry = Refine_FV_EXT_ENTRY_GUID_TYPE_Header(nums).from_buffer_copy(buffer[self.ExtEntryOffset:])
                elif self.ExtEntry.ExtEntryType == 0x03:
                    self.ExtEntry = EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE.from_buffer_copy(buffer[self.ExtEntryOffset:])
                else:
                    self.ExtTypeExist = 0
            else:
                self.ExtEntryExist = 0
        self.Size = self.Header.FvLength
        self.HeaderLength = self.Header.HeaderLength
        self.HOffset = 0
        self.DOffset = 0
        self.ROffset = 0
        self.Data = b''
        if self.Header.Signature != 1213613663:
            logger.error('Invalid Fv Header! Fv {} signature {} is not "_FVH".'.format(struct2stream(self.Header), self.Header.Signature))
            raise Exception("Process Failed: Fv Header Signature!")
        self.PadData = b''
        self.Free_Space = 0
        self.ModCheckSum()

    def ModCheckSum(self) -> None:
        # Fv Header Sums to 0.
        Header = struct2stream(self.Header)[::-1]
        Size = self.HeaderLength // 2
        Sum = 0
        for i in range(Size):
            Sum += int(Header[i*2: i*2 + 2].hex(), 16)
        if Sum & 0xffff:
            self.Header.Checksum = 0x10000 - (Sum - self.Header.Checksum) % 0x10000

    def ModFvExt(self) -> None:
        # If used space changes and self.ExtEntry.UsedSize exists, self.ExtEntry.UsedSize need to be changed.
        if self.Header.ExtHeaderOffset and self.ExtEntryExist and self.ExtTypeExist and self.ExtEntry.Hdr.ExtEntryType == 0x03:
            self.ExtEntry.UsedSize = self.Header.FvLength - self.Free_Space

    def ModFvSize(self) -> None:
        # If Fv Size changed, self.Header.FvLength and self.Header.BlockMap[i].NumBlocks need to be changed.
        BlockMapNum = len(self.Header.BlockMap)
        for i in range(BlockMapNum):
            if self.Header.BlockMap[i].Length:
                self.Header.BlockMap[i].NumBlocks = self.Header.FvLength // self.Header.BlockMap[i].Length

    def ModExtHeaderData(self) -> None:
        if self.Header.ExtHeaderOffset:
            ExtHeaderData = struct2stream(self.ExtHeader)
            ExtHeaderDataOffset = self.Header.ExtHeaderOffset - self.HeaderLength
            self.Data = self.Data[:ExtHeaderDataOffset] + ExtHeaderData + self.Data[ExtHeaderDataOffset+20:]
        if self.Header.ExtHeaderOffset and self.ExtEntryExist:
            ExtHeaderEntryData = struct2stream(self.ExtEntry)
            ExtHeaderEntryDataOffset = self.Header.ExtHeaderOffset + 20 - self.HeaderLength
            self.Data = self.Data[:ExtHeaderEntryDataOffset] + ExtHeaderEntryData + self.Data[ExtHeaderEntryDataOffset+len(ExtHeaderEntryData):]

class FfsNode:
    def __init__(self, buffer: bytes) -> None:
        self.Header = EFI_FFS_FILE_HEADER.from_buffer_copy(buffer)
        # self.Attributes = unpack("<B", buffer[21:22])[0]
        if self.Header.FFS_FILE_SIZE != 0 and self.Header.Attributes != 0xff and self.Header.Attributes & 0x01 == 1:
            logger.error('Error Ffs Header! Ffs {} Header Size and Attributes is not matched!'.format(uuid.UUID(bytes_le=struct2stream(self.Header.Name))))
            raise Exception("Process Failed: Error Ffs Header!")
        if self.Header.FFS_FILE_SIZE == 0 and self.Header.Attributes & 0x01 == 1:
            self.Header = EFI_FFS_FILE_HEADER2.from_buffer_copy(buffer)
        self.Name = uuid.UUID(bytes_le=struct2stream(self.Header.Name))
        self.UiName = b''
        self.Version = b''
        self.Size = self.Header.FFS_FILE_SIZE
        self.HeaderLength = self.Header.HeaderLength
        self.HOffset = 0
        self.DOffset = 0
        self.ROffset = 0
        self.Data = b''
        self.PadData = b''
        self.SectionMaxAlignment = SECTION_COMMON_ALIGNMENT  # 4-align

    def ModCheckSum(self) -> None:
        HeaderData = struct2stream(self.Header)
        HeaderSum = 0
        for item in HeaderData:
            HeaderSum += item
        HeaderSum -= self.Header.State
        HeaderSum -= self.Header.IntegrityCheck.Checksum.File
        if HeaderSum & 0xff:
            Header = self.Header.IntegrityCheck.Checksum.Header + 0x100 - HeaderSum % 0x100
            self.Header.IntegrityCheck.Checksum.Header = Header % 0x100

class SectionNode:
    def __init__(self, buffer: bytes) -> None:
        if buffer[0:3] != b'\xff\xff\xff':
            self.Header = EFI_COMMON_SECTION_HEADER.from_buffer_copy(buffer)
        else:
            self.Header = EFI_COMMON_SECTION_HEADER2.from_buffer_copy(buffer)
        if self.Header.Type in SectionHeaderType:
            self.Name = SectionHeaderType[self.Header.Type]
        elif self.Header.Type == 0:
            self.Name = "EFI_SECTION_ALL"
        else:
            self.Name = "SECTION"
        if self.Header.Type in HeaderType:
            self.ExtHeader = self.GetExtHeader(self.Header.Type, buffer[self.Header.Common_Header_Size():], (self.Header.SECTION_SIZE-self.Header.Common_Header_Size()))
            self.HeaderLength = self.Header.Common_Header_Size() + self.ExtHeader.ExtHeaderSize()
        else:
            self.ExtHeader = None
            self.HeaderLength = self.Header.Common_Header_Size()
        self.Size = self.Header.SECTION_SIZE
        self.Type = self.Header.Type
        self.HOffset = 0
        self.DOffset = 0
        self.ROffset = 0
        self.Data = b''
        self.OriData = b''
        self.OriHeader = b''
        self.PadData = b''
        self.IsPadSection = False
        self.SectionMaxAlignment = SECTION_COMMON_ALIGNMENT  # 4-align

    def GetExtHeader(self, Type: int, buffer: bytes, nums: int=0) -> None:
        if Type == 0x01:
            return EFI_COMPRESSION_SECTION.from_buffer_copy(buffer)
        elif Type == 0x02:
            return EFI_GUID_DEFINED_SECTION.from_buffer_copy(buffer)
        elif Type == 0x14:
            return Get_VERSION_Header((nums - 2)//2).from_buffer_copy(buffer)
        elif Type == 0x15:
            return Get_USER_INTERFACE_Header(nums//2).from_buffer_copy(buffer)
        elif Type == 0x18:
            return EFI_FREEFORM_SUBTYPE_GUID_SECTION.from_buffer_copy(buffer)

class FreeSpaceNode:
    def __init__(self, buffer: bytes) -> None:
        self.Name = 'Free_Space'
        self.Data = buffer
        self.Size = len(buffer)
        self.HOffset = 0
        self.DOffset = 0
        self.ROffset = 0
        self.PadData = b''
