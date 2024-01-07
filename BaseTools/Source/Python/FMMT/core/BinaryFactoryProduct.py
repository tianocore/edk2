## @file
# This file is used to implement of the various bianry parser.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from re import T
import copy
import os
import sys
from FirmwareStorageFormat.Common import *
from core.BiosTreeNode import *
from core.BiosTree import *
from core.GuidTools import GUIDTools
from utils.FmmtLogger import FmmtLogger as logger

ROOT_TREE = 'ROOT'
ROOT_ELF_TREE = 'ROOT_ELF_TREE'
ROOT_FV_TREE = 'ROOT_FV_TREE'
ROOT_FFS_TREE = 'ROOT_FFS_TREE'
ROOT_SECTION_TREE = 'ROOT_SECTION_TREE'

ELF_TREE = 'ELF'
ELF_SECTION_TREE = 'ELF_SECTION_TREE'
FV_TREE = 'FV'
DATA_FV_TREE = 'DATA_FV'
FFS_TREE = 'FFS'
FFS_PAD = 'FFS_PAD'
FFS_FREE_SPACE = 'FFS_FREE_SPACE'
SECTION_TREE = 'SECTION'
SEC_FV_TREE = 'SEC_FV_IMAGE'
BINARY_DATA = 'BINARY'
Fv_count = 0

## Abstract factory
class BinaryFactory():
    type:list = []

    def Create_Product():
        pass

class BinaryProduct():
    ## Use GuidTool to decompress data.
    def DeCompressData(self, GuidTool, Section_Data: bytes, FileName) -> bytes:
        guidtool = GUIDTools().__getitem__(struct2stream(GuidTool))
        if not guidtool.ifexist:
            logger.error("GuidTool {} is not found when decompressing {} file.\n".format(guidtool.command, FileName))
            raise Exception("Process Failed: GuidTool not found!")
        DecompressedData = guidtool.unpack(Section_Data)
        return DecompressedData

    def ParserData():
        pass

class ElfFactory(BinaryFactory):
    type = [ROOT_ELF_TREE, ELF_TREE]

    def Create_Product():
        return ElfProduct()

class SectionFactory(BinaryFactory):
    type = [SECTION_TREE]

    def Create_Product():
        return SectionProduct()

class FfsFactory(BinaryFactory):
    type = [ROOT_SECTION_TREE, FFS_TREE]

    def Create_Product():
        return FfsProduct()

class FvFactory(BinaryFactory):
    type = [ROOT_FFS_TREE, FV_TREE, SEC_FV_TREE]

    def Create_Product():
        return FvProduct()

class FdFactory(BinaryFactory):
    type = [ROOT_FV_TREE, ROOT_TREE]

    def Create_Product():
        return FdProduct()

class SectionProduct(BinaryProduct):
    ## Decompress the compressed section.
    def ParserData(self, Section_Tree, whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        if Section_Tree.Data.Type == 0x01:
            Section_Tree.Data.OriData = Section_Tree.Data.Data
            self.ParserSection(Section_Tree, b'')
        # Guided Define Section
        elif Section_Tree.Data.Type == 0x02:
            Section_Tree.Data.OriData = Section_Tree.Data.Data
            DeCompressGuidTool = Section_Tree.Data.ExtHeader.SectionDefinitionGuid
            Section_Tree.Data.Data = self.DeCompressData(DeCompressGuidTool, Section_Tree.Data.Data, Section_Tree.Parent.Data.Name)
            Section_Tree.Data.Size = len(Section_Tree.Data.Data) + Section_Tree.Data.HeaderLength
            self.ParserSection(Section_Tree, b'')
        elif Section_Tree.Data.Type == 0x03:
            Section_Tree.Data.OriData = Section_Tree.Data.Data
            self.ParserSection(Section_Tree, b'')
        # SEC_FV Section
        elif Section_Tree.Data.Type == 0x17:
            global Fv_count
            Sec_Fv_Info = FvNode(Fv_count, Section_Tree.Data.Data)
            Sec_Fv_Tree = BIOSTREE('FV'+ str(Fv_count))
            Sec_Fv_Tree.type = SEC_FV_TREE
            Sec_Fv_Tree.Data = Sec_Fv_Info
            Sec_Fv_Tree.Data.HOffset = Section_Tree.Data.DOffset
            Sec_Fv_Tree.Data.DOffset = Sec_Fv_Tree.Data.HOffset + Sec_Fv_Tree.Data.Header.HeaderLength
            Sec_Fv_Tree.Data.Data = Section_Tree.Data.Data[Sec_Fv_Tree.Data.Header.HeaderLength:]
            Section_Tree.insertChild(Sec_Fv_Tree)
            Fv_count += 1

    def ParserSection(self, ParTree, Whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        Rel_Offset = 0
        Section_Offset = 0
        # Get the Data from parent tree, if do not have the tree then get it from the whole_data.
        if ParTree.Data != None:
            Data_Size = len(ParTree.Data.Data)
            Section_Offset = ParTree.Data.DOffset
            Whole_Data = ParTree.Data.Data
        else:
            Data_Size = len(Whole_Data)
        # Parser all the data to collect all the Section recorded in its Parent Section.
        while Rel_Offset < Data_Size:
            # Create a SectionNode and set it as the SectionTree's Data
            Section_Info = SectionNode(Whole_Data[Rel_Offset:])
            Section_Tree = BIOSTREE(Section_Info.Name)
            Section_Tree.type = SECTION_TREE
            Section_Info.Data = Whole_Data[Rel_Offset+Section_Info.HeaderLength: Rel_Offset+Section_Info.Size]
            Section_Info.DOffset = Section_Offset + Section_Info.HeaderLength + Rel_Whole_Offset
            Section_Info.HOffset = Section_Offset + Rel_Whole_Offset
            Section_Info.ROffset = Rel_Offset
            if Section_Info.Header.Type == 0:
                break
            # The final Section in parent Section does not need to add padding, else must be 4-bytes align with parent Section start offset
            Pad_Size = 0
            if (Rel_Offset+Section_Info.HeaderLength+len(Section_Info.Data) != Data_Size):
                Pad_Size = GetPadSize(Section_Info.Size, SECTION_COMMON_ALIGNMENT)
                Section_Info.PadData = Pad_Size * b'\x00'
            if Section_Info.Header.Type == 0x02:
                Section_Info.DOffset = Section_Offset + Section_Info.ExtHeader.DataOffset + Rel_Whole_Offset
                Section_Info.Data = Whole_Data[Rel_Offset+Section_Info.ExtHeader.DataOffset: Rel_Offset+Section_Info.Size]
            if Section_Info.Header.Type == 0x14:
                ParTree.Data.Version = Section_Info.ExtHeader.GetVersionString()
            if Section_Info.Header.Type == 0x15:
                ParTree.Data.UiName = Section_Info.ExtHeader.GetUiString()
            if Section_Info.Header.Type == 0x19:
                if Section_Info.Data.replace(b'\x00', b'') == b'':
                    Section_Info.IsPadSection = True
            Section_Offset += Section_Info.Size + Pad_Size
            Rel_Offset += Section_Info.Size + Pad_Size
            Section_Tree.Data = Section_Info
            ParTree.insertChild(Section_Tree)

class FfsProduct(BinaryProduct):
    # ParserFFs / GetSection
    def ParserData(self, ParTree, Whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        Rel_Offset = 0
        Section_Offset = 0
        # Get the Data from parent tree, if do not have the tree then get it from the whole_data.
        if ParTree.Data != None:
            Data_Size = len(ParTree.Data.Data)
            Section_Offset = ParTree.Data.DOffset
            Whole_Data = ParTree.Data.Data
        else:
            Data_Size = len(Whole_Data)
        # Parser all the data to collect all the Section recorded in Ffs.
        while Rel_Offset < Data_Size:
            # Create a SectionNode and set it as the SectionTree's Data
            Section_Info = SectionNode(Whole_Data[Rel_Offset:])
            Section_Tree = BIOSTREE(Section_Info.Name)
            Section_Tree.type = SECTION_TREE
            Section_Info.Data = Whole_Data[Rel_Offset+Section_Info.HeaderLength: Rel_Offset+Section_Info.Size]
            Section_Info.DOffset = Section_Offset + Section_Info.HeaderLength + Rel_Whole_Offset
            Section_Info.HOffset = Section_Offset + Rel_Whole_Offset
            Section_Info.ROffset = Rel_Offset
            if Section_Info.Header.Type == 0:
                break
            # The final Section in Ffs does not need to add padding, else must be 4-bytes align with Ffs start offset
            Pad_Size = 0
            if (Rel_Offset+Section_Info.HeaderLength+len(Section_Info.Data) != Data_Size):
                Pad_Size = GetPadSize(Section_Info.Size, SECTION_COMMON_ALIGNMENT)
                Section_Info.PadData = Pad_Size * b'\x00'
            if Section_Info.Header.Type == 0x02:
                Section_Info.DOffset = Section_Offset + Section_Info.ExtHeader.DataOffset + Rel_Whole_Offset
                Section_Info.Data = Whole_Data[Rel_Offset+Section_Info.ExtHeader.DataOffset: Rel_Offset+Section_Info.Size]
            # If Section is Version or UI type, it saves the version and UI info of its parent Ffs.
            if Section_Info.Header.Type == 0x14:
                ParTree.Data.Version = Section_Info.ExtHeader.GetVersionString()
            if Section_Info.Header.Type == 0x15:
                ParTree.Data.UiName = Section_Info.ExtHeader.GetUiString()
            if Section_Info.Header.Type == 0x19:
                if Section_Info.Data.replace(b'\x00', b'') == b'':
                    Section_Info.IsPadSection = True
            Section_Offset += Section_Info.Size + Pad_Size
            Rel_Offset += Section_Info.Size + Pad_Size
            Section_Tree.Data = Section_Info
            ParTree.insertChild(Section_Tree)

class FvProduct(BinaryProduct):
    ##  ParserFv / GetFfs
    def ParserData(self, ParTree, Whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        Ffs_Offset = 0
        Rel_Offset = 0
        # Get the Data from parent tree, if do not have the tree then get it from the whole_data.
        if ParTree.Data != None:
            Data_Size = len(ParTree.Data.Data)
            Ffs_Offset = ParTree.Data.DOffset
            Whole_Data = ParTree.Data.Data
        else:
            Data_Size = len(Whole_Data)
        # Parser all the data to collect all the Ffs recorded in Fv.
        while Rel_Offset < Data_Size:
            # Create a FfsNode and set it as the FFsTree's Data
            if Data_Size - Rel_Offset < 24:
                Ffs_Tree = BIOSTREE('Free_Space')
                Ffs_Tree.type = FFS_FREE_SPACE
                Ffs_Tree.Data = FreeSpaceNode(Whole_Data[Rel_Offset:])
                Ffs_Tree.Data.HOffset = Ffs_Offset + Rel_Whole_Offset
                Ffs_Tree.Data.DOffset = Ffs_Tree.Data.HOffset
                ParTree.Data.Free_Space = Data_Size - Rel_Offset
                ParTree.insertChild(Ffs_Tree)
                Rel_Offset = Data_Size
            else:
                Ffs_Info = FfsNode(Whole_Data[Rel_Offset:])
                Ffs_Tree = BIOSTREE(Ffs_Info.Name)
                Ffs_Info.HOffset = Ffs_Offset + Rel_Whole_Offset
                Ffs_Info.DOffset = Ffs_Offset + Ffs_Info.Header.HeaderLength + Rel_Whole_Offset
                Ffs_Info.ROffset = Rel_Offset
                if Ffs_Info.Name == PADVECTOR:
                    Ffs_Tree.type = FFS_PAD
                    Ffs_Info.Data = Whole_Data[Rel_Offset+Ffs_Info.Header.HeaderLength: Rel_Offset+Ffs_Info.Size]
                    Ffs_Info.Size = len(Ffs_Info.Data) + Ffs_Info.Header.HeaderLength
                    # if current Ffs is the final ffs of Fv and full of b'\xff', define it with Free_Space
                    if struct2stream(Ffs_Info.Header).replace(b'\xff', b'') == b'':
                        Ffs_Tree.type = FFS_FREE_SPACE
                        Ffs_Info.Data = Whole_Data[Rel_Offset:]
                        Ffs_Info.Size = len(Ffs_Info.Data)
                        ParTree.Data.Free_Space = Ffs_Info.Size
                else:
                    Ffs_Tree.type = FFS_TREE
                    Ffs_Info.Data = Whole_Data[Rel_Offset+Ffs_Info.Header.HeaderLength: Rel_Offset+Ffs_Info.Size]
                # The final Ffs in Fv does not need to add padding, else must be 8-bytes align with Fv start offset
                Pad_Size = 0
                if Ffs_Tree.type != FFS_FREE_SPACE and (Rel_Offset+Ffs_Info.Header.HeaderLength+len(Ffs_Info.Data) != Data_Size):
                    Pad_Size = GetPadSize(Ffs_Info.Size, FFS_COMMON_ALIGNMENT)
                    Ffs_Info.PadData = Pad_Size * b'\xff'
                Ffs_Offset += Ffs_Info.Size + Pad_Size
                Rel_Offset += Ffs_Info.Size + Pad_Size
                Ffs_Tree.Data = Ffs_Info
                ParTree.insertChild(Ffs_Tree)

class FdProduct(BinaryProduct):
    type = [ROOT_FV_TREE, ROOT_TREE]

    ## Create DataTree with first level /fv Info, then parser each Fv.
    def ParserData(self, WholeFvTree, whole_data: bytes=b'', offset: int=0) -> None:
        # Get all Fv image in Fd with offset and length
        Fd_Struct = self.GetFvFromFd(whole_data)
        data_size = len(whole_data)
        Binary_count = 0
        global Fv_count
        # If the first Fv image is the Binary Fv, add it into the tree.
        if Fd_Struct[0][1] != 0:
            Binary_node = BIOSTREE('BINARY'+ str(Binary_count))
            Binary_node.type = BINARY_DATA
            Binary_node.Data = BinaryNode(str(Binary_count))
            Binary_node.Data.Data = whole_data[:Fd_Struct[0][1]]
            Binary_node.Data.Size = len(Binary_node.Data.Data)
            Binary_node.Data.HOffset = 0 + offset
            WholeFvTree.insertChild(Binary_node)
            Binary_count += 1
        # Add the first collected Fv image into the tree.
        Cur_node = BIOSTREE(Fd_Struct[0][0]+ str(Fv_count))
        Cur_node.type = Fd_Struct[0][0]
        Cur_node.Data = FvNode(Fv_count, whole_data[Fd_Struct[0][1]:Fd_Struct[0][1]+Fd_Struct[0][2][0]])
        Cur_node.Data.HOffset = Fd_Struct[0][1] + offset
        Cur_node.Data.DOffset = Cur_node.Data.HOffset+Cur_node.Data.Header.HeaderLength
        Cur_node.Data.Data = whole_data[Fd_Struct[0][1]+Cur_node.Data.Header.HeaderLength:Fd_Struct[0][1]+Cur_node.Data.Size]
        WholeFvTree.insertChild(Cur_node)
        Fv_count += 1
        Fv_num = len(Fd_Struct)
        # Add all the collected Fv image and the Binary Fv image between them into the tree.
        for i in range(Fv_num-1):
            if Fd_Struct[i][1]+Fd_Struct[i][2][0] != Fd_Struct[i+1][1]:
                Binary_node = BIOSTREE('BINARY'+ str(Binary_count))
                Binary_node.type = BINARY_DATA
                Binary_node.Data = BinaryNode(str(Binary_count))
                Binary_node.Data.Data = whole_data[Fd_Struct[i][1]+Fd_Struct[i][2][0]:Fd_Struct[i+1][1]]
                Binary_node.Data.Size = len(Binary_node.Data.Data)
                Binary_node.Data.HOffset = Fd_Struct[i][1]+Fd_Struct[i][2][0] + offset
                WholeFvTree.insertChild(Binary_node)
                Binary_count += 1
            Cur_node = BIOSTREE(Fd_Struct[i+1][0]+ str(Fv_count))
            Cur_node.type = Fd_Struct[i+1][0]
            Cur_node.Data = FvNode(Fv_count, whole_data[Fd_Struct[i+1][1]:Fd_Struct[i+1][1]+Fd_Struct[i+1][2][0]])
            Cur_node.Data.HOffset = Fd_Struct[i+1][1] + offset
            Cur_node.Data.DOffset = Cur_node.Data.HOffset+Cur_node.Data.Header.HeaderLength
            Cur_node.Data.Data = whole_data[Fd_Struct[i+1][1]+Cur_node.Data.Header.HeaderLength:Fd_Struct[i+1][1]+Cur_node.Data.Size]
            WholeFvTree.insertChild(Cur_node)
            Fv_count += 1
        # If the final Fv image is the Binary Fv, add it into the tree
        if Fd_Struct[-1][1] + Fd_Struct[-1][2][0] != data_size:
            Binary_node = BIOSTREE('BINARY'+ str(Binary_count))
            Binary_node.type = BINARY_DATA
            Binary_node.Data = BinaryNode(str(Binary_count))
            Binary_node.Data.Data = whole_data[Fd_Struct[-1][1]+Fd_Struct[-1][2][0]:]
            Binary_node.Data.Size = len(Binary_node.Data.Data)
            Binary_node.Data.HOffset = Fd_Struct[-1][1]+Fd_Struct[-1][2][0] + offset
            WholeFvTree.insertChild(Binary_node)
            Binary_count += 1

    ## Get the first level Fv from Fd file.
    def GetFvFromFd(self, whole_data: bytes=b'') -> list:
        Fd_Struct = []
        data_size = len(whole_data)
        cur_index = 0
        # Get all the EFI_FIRMWARE_FILE_SYSTEM2_GUID_BYTE FV image offset and length.
        while cur_index < data_size:
            if EFI_FIRMWARE_FILE_SYSTEM2_GUID_BYTE in whole_data[cur_index:]:
                target_index = whole_data[cur_index:].index(EFI_FIRMWARE_FILE_SYSTEM2_GUID_BYTE) + cur_index
                if whole_data[target_index+24:target_index+28] == FVH_SIGNATURE:
                    Fd_Struct.append([FV_TREE, target_index - 16, unpack("Q", whole_data[target_index+16:target_index+24])])
                    cur_index = Fd_Struct[-1][1] + Fd_Struct[-1][2][0]
                else:
                    cur_index = target_index + 16
            else:
                cur_index = data_size
        cur_index = 0
        # Get all the EFI_FIRMWARE_FILE_SYSTEM3_GUID_BYTE FV image offset and length.
        while cur_index < data_size:
            if EFI_FIRMWARE_FILE_SYSTEM3_GUID_BYTE in whole_data[cur_index:]:
                target_index = whole_data[cur_index:].index(EFI_FIRMWARE_FILE_SYSTEM3_GUID_BYTE) + cur_index
                if whole_data[target_index+24:target_index+28] == FVH_SIGNATURE:
                    Fd_Struct.append([FV_TREE, target_index - 16, unpack("Q", whole_data[target_index+16:target_index+24])])
                    cur_index = Fd_Struct[-1][1] + Fd_Struct[-1][2][0]
                else:
                    cur_index = target_index + 16
            else:
                cur_index = data_size
        cur_index = 0
        # Get all the EFI_SYSTEM_NVDATA_FV_GUID_BYTE FV image offset and length.
        while cur_index < data_size:
            if EFI_SYSTEM_NVDATA_FV_GUID_BYTE in whole_data[cur_index:]:
                target_index = whole_data[cur_index:].index(EFI_SYSTEM_NVDATA_FV_GUID_BYTE) + cur_index
                if whole_data[target_index+24:target_index+28] == FVH_SIGNATURE:
                    Fd_Struct.append([DATA_FV_TREE, target_index - 16, unpack("Q", whole_data[target_index+16:target_index+24])])
                    cur_index = Fd_Struct[-1][1] + Fd_Struct[-1][2][0]
                else:
                    cur_index = target_index + 16
            else:
                cur_index = data_size
        # Sort all the collect Fv image with offset.
        Fd_Struct.sort(key=lambda x:x[1])
        tmp_struct = copy.deepcopy(Fd_Struct)
        tmp_index = 0
        Fv_num = len(Fd_Struct)
        # Remove the Fv image included in another Fv image.
        for i in range(1,Fv_num):
            if tmp_struct[i][1]+tmp_struct[i][2][0] < tmp_struct[i-1][1]+tmp_struct[i-1][2][0]:
                Fd_Struct.remove(Fd_Struct[i-tmp_index])
                tmp_index += 1
        return Fd_Struct

class ElfSectionProduct(BinaryProduct):
    ## Decompress the compressed section.
    def ParserData(self, Section_Tree, whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        pass
    def ParserSectionData(self, Section_Tree, whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        pass
    def ParserProgramData(self, Section_Tree, whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        pass

class ElfProduct(BinaryProduct):

    def ParserData(self, ParTree, Whole_Data: bytes, Rel_Whole_Offset: int=0) -> None:
        Elf_Info = ElfNode(Whole_Data)
        if Elf_Info.Header.ELF_PHOff != 0:
            Elf_Info.GetProgramList(Whole_Data[Elf_Info.Header.ELF_PHOff:])
        if Elf_Info.Header.ELF_SHOff != 0:
            Elf_Info.GetSectionList(Whole_Data[Elf_Info.Header.ELF_SHOff:])
            Elf_Info.FindUPLDSection(Whole_Data)
        Elf_Tree = BIOSTREE(Elf_Info.Name)
        Elf_Tree.type = ELF_TREE
        Elf_Info.Data = Whole_Data[Elf_Info.HeaderLength:]
        Elf_Tree.Data = Elf_Info
        ParTree.insertChild(Elf_Tree)

class ParserEntry():
    FactoryTable:dict = {
        SECTION_TREE: SectionFactory,
        ROOT_SECTION_TREE: FfsFactory,
        FFS_TREE: FfsFactory,
        ROOT_FFS_TREE: FvFactory,
        FV_TREE: FvFactory,
        SEC_FV_TREE: FvFactory,
        ROOT_FV_TREE: FdFactory,
        ROOT_TREE: FdFactory,
        ROOT_ELF_TREE: ElfFactory,
    }

    def GetTargetFactory(self, Tree_type: str) -> BinaryFactory:
        if Tree_type in self.FactoryTable:
            return self.FactoryTable[Tree_type]

    def Generate_Product(self, TargetFactory: BinaryFactory, Tree, Data: bytes, Offset: int) -> None:
        New_Product = TargetFactory.Create_Product()
        New_Product.ParserData(Tree, Data, Offset)

    def DataParser(self, Tree, Data: bytes, Offset: int) -> None:
        TargetFactory = self.GetTargetFactory(Tree.type)
        if TargetFactory:
            self.Generate_Product(TargetFactory, Tree, Data, Offset)
