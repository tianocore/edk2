## @file
# process FD Region generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from struct import *
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from io import BytesIO
import string
import Common.LongFilePathOs as os
from stat import *
from Common import EdkLogger
from Common.BuildToolError import *
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.MultipleWorkspace import MultipleWorkspace as mws
from Common.DataType import BINARY_FILE_TYPE_FV

## generate Region
#
#
class Region(object):

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.Offset = None       # The begin position of the Region
        self.Size = None         # The Size of the Region
        self.PcdOffset = None
        self.PcdSize = None
        self.SetVarDict = {}
        self.RegionType = None
        self.RegionDataList = []

    ## PadBuffer()
    #
    #   Add padding bytes to the Buffer
    #
    #   @param Buffer         The buffer the generated region data will be put
    #                         in
    #   @param ErasePolarity  Flash erase polarity
    #   @param Size           Number of padding bytes requested
    #

    def PadBuffer(self, Buffer, ErasePolarity, Size):
        if Size > 0:
            if (ErasePolarity == '1') :
                PadByte = pack('B', 0xFF)
            else:
                PadByte = pack('B', 0)
            for i in range(0, Size):
                Buffer.write(PadByte)

    ## AddToBuffer()
    #
    #   Add region data to the Buffer
    #
    #   @param  self        The object pointer
    #   @param  Buffer      The buffer generated region data will be put
    #   @param  BaseAddress base address of region
    #   @param  BlockSize   block size of region
    #   @param  BlockNum    How many blocks in region
    #   @param  ErasePolarity      Flash erase polarity
    #   @param  MacroDict   macro value pair
    #   @retval string      Generated FV file path
    #

    def AddToBuffer(self, Buffer, BaseAddress, BlockSizeList, ErasePolarity, ImageBinDict,  MacroDict={}, Flag=False):
        Size = self.Size
        if not Flag:
            GenFdsGlobalVariable.InfLogger('\nGenerate Region at Offset 0x%X' % self.Offset)
            GenFdsGlobalVariable.InfLogger("   Region Size = 0x%X" % Size)
        GenFdsGlobalVariable.SharpCounter = 0
        if Flag and (self.RegionType != BINARY_FILE_TYPE_FV):
            return

        if self.RegionType == BINARY_FILE_TYPE_FV:
            #
            # Get Fv from FvDict
            #
            self.FvAddress = int(BaseAddress, 16) + self.Offset
            FvBaseAddress = '0x%X' % self.FvAddress
            FvOffset = 0
            for RegionData in self.RegionDataList:
                FileName = None
                if RegionData.endswith(".fv"):
                    RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                    if not Flag:
                        GenFdsGlobalVariable.InfLogger('   Region FV File Name = .fv : %s' % RegionData)
                    if RegionData[1] != ':' :
                        RegionData = mws.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                    if not os.path.exists(RegionData):
                        EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)

                    FileName = RegionData
                elif RegionData.upper() + 'fv' in ImageBinDict:
                    if not Flag:
                        GenFdsGlobalVariable.InfLogger('   Region Name = FV')
                    FileName = ImageBinDict[RegionData.upper() + 'fv']
                else:
                    #
                    # Generate FvImage.
                    #
                    FvObj = None
                    if RegionData.upper() in GenFdsGlobalVariable.FdfParser.Profile.FvDict:
                        FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict[RegionData.upper()]

                    if FvObj is not None :
                        if not Flag:
                            GenFdsGlobalVariable.InfLogger('   Region Name = FV')
                        #
                        # Call GenFv tool
                        #
                        self.BlockInfoOfRegion(BlockSizeList, FvObj)
                        self.FvAddress = self.FvAddress + FvOffset
                        FvAlignValue = GenFdsGlobalVariable.GetAlignment(FvObj.FvAlignment)
                        if self.FvAddress % FvAlignValue != 0:
                            EdkLogger.error("GenFds", GENFDS_ERROR,
                                            "FV (%s) is NOT %s Aligned!" % (FvObj.UiFvName, FvObj.FvAlignment))
                        FvBuffer = BytesIO()
                        FvBaseAddress = '0x%X' % self.FvAddress
                        BlockSize = None
                        BlockNum = None
                        FvObj.AddToBuffer(FvBuffer, FvBaseAddress, BlockSize, BlockNum, ErasePolarity, Flag=Flag)
                        if Flag:
                            continue

                        FvBufferLen = len(FvBuffer.getvalue())
                        if FvBufferLen > Size:
                            FvBuffer.close()
                            EdkLogger.error("GenFds", GENFDS_ERROR,
                                            "Size of FV (%s) is larger than Region Size 0x%X specified." % (RegionData, Size))
                        #
                        # Put the generated image into FD buffer.
                        #
                        Buffer.write(FvBuffer.getvalue())
                        FvBuffer.close()
                        FvOffset = FvOffset + FvBufferLen
                        Size = Size - FvBufferLen
                        continue
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "FV (%s) is NOT described in FDF file!" % (RegionData))
                #
                # Add the exist Fv image into FD buffer
                #
                if not Flag:
                    if FileName is not None:
                        FileLength = os.stat(FileName)[ST_SIZE]
                        if FileLength > Size:
                            EdkLogger.error("GenFds", GENFDS_ERROR,
                                            "Size of FV File (%s) is larger than Region Size 0x%X specified." \
                                            % (RegionData, Size))
                        BinFile = open(FileName, 'rb')
                        Buffer.write(BinFile.read())
                        BinFile.close()
                        Size = Size - FileLength
            #
            # Pad the left buffer
            #
            if not Flag:
                self.PadBuffer(Buffer, ErasePolarity, Size)

        if self.RegionType == 'CAPSULE':
            #
            # Get Capsule from Capsule Dict
            #
            for RegionData in self.RegionDataList:
                if RegionData.endswith(".cap"):
                    RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                    GenFdsGlobalVariable.InfLogger('   Region CAPSULE Image Name = .cap : %s' % RegionData)
                    if RegionData[1] != ':' :
                        RegionData = mws.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                    if not os.path.exists(RegionData):
                        EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)

                    FileName = RegionData
                elif RegionData.upper() + 'cap' in ImageBinDict:
                    GenFdsGlobalVariable.InfLogger('   Region Name = CAPSULE')
                    FileName = ImageBinDict[RegionData.upper() + 'cap']
                else:
                    #
                    # Generate Capsule image and Put it into FD buffer
                    #
                    CapsuleObj = None
                    if RegionData.upper() in GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict:
                        CapsuleObj = GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict[RegionData.upper()]

                    if CapsuleObj is not None :
                        CapsuleObj.CapsuleName = RegionData.upper()
                        GenFdsGlobalVariable.InfLogger('   Region Name = CAPSULE')
                        #
                        # Call GenFv tool to generate Capsule Image
                        #
                        FileName = CapsuleObj.GenCapsule()
                        CapsuleObj.CapsuleName = None
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "Capsule (%s) is NOT described in FDF file!" % (RegionData))

                #
                # Add the capsule image into FD buffer
                #
                FileLength = os.stat(FileName)[ST_SIZE]
                if FileLength > Size:
                    EdkLogger.error("GenFds", GENFDS_ERROR,
                                    "Size 0x%X of Capsule File (%s) is larger than Region Size 0x%X specified." \
                                    % (FileLength, RegionData, Size))
                BinFile = open(FileName, 'rb')
                Buffer.write(BinFile.read())
                BinFile.close()
                Size = Size - FileLength
            #
            # Pad the left buffer
            #
            self.PadBuffer(Buffer, ErasePolarity, Size)

        if self.RegionType in ('FILE', 'INF'):
            for RegionData in self.RegionDataList:
                if self.RegionType == 'INF':
                    RegionData.__InfParse__(None)
                    if len(RegionData.BinFileList) != 1:
                        EdkLogger.error('GenFds', GENFDS_ERROR, 'INF in FD region can only contain one binary: %s' % RegionData)
                    File = RegionData.BinFileList[0]
                    RegionData = RegionData.PatchEfiFile(File.Path, File.Type)
                else:
                    RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                    if RegionData[1] != ':' :
                        RegionData = mws.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                    if not os.path.exists(RegionData):
                        EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)
                #
                # Add the file image into FD buffer
                #
                FileLength = os.stat(RegionData)[ST_SIZE]
                if FileLength > Size:
                    EdkLogger.error("GenFds", GENFDS_ERROR,
                                    "Size of File (%s) is larger than Region Size 0x%X specified." \
                                    % (RegionData, Size))
                GenFdsGlobalVariable.InfLogger('   Region File Name = %s' % RegionData)
                BinFile = open(RegionData, 'rb')
                Buffer.write(BinFile.read())
                BinFile.close()
                Size = Size - FileLength
            #
            # Pad the left buffer
            #
            self.PadBuffer(Buffer, ErasePolarity, Size)

        if self.RegionType == 'DATA' :
            GenFdsGlobalVariable.InfLogger('   Region Name = DATA')
            DataSize = 0
            for RegionData in self.RegionDataList:
                Data = RegionData.split(',')
                DataSize = DataSize + len(Data)
                if DataSize > Size:
                   EdkLogger.error("GenFds", GENFDS_ERROR, "Size of DATA is larger than Region Size ")
                else:
                    for item in Data :
                        Buffer.write(pack('B', int(item, 16)))
                Size = Size - DataSize
            #
            # Pad the left buffer
            #
            self.PadBuffer(Buffer, ErasePolarity, Size)

        if self.RegionType is None:
            GenFdsGlobalVariable.InfLogger('   Region Name = None')
            self.PadBuffer(Buffer, ErasePolarity, Size)

    ## BlockSizeOfRegion()
    #
    #   @param  BlockSizeList        List of block information
    #   @param  FvObj                The object for FV
    #
    def BlockInfoOfRegion(self, BlockSizeList, FvObj):
        Start = 0
        End = 0
        RemindingSize = self.Size
        ExpectedList = []
        for (BlockSize, BlockNum, pcd) in BlockSizeList:
            End = Start + BlockSize * BlockNum
            # region not started yet
            if self.Offset >= End:
                Start = End
                continue
            # region located in current blocks
            else:
                # region ended within current blocks
                if self.Offset + self.Size <= End:
                    ExpectedList.append((BlockSize, (RemindingSize + BlockSize - 1) // BlockSize))
                    break
                # region not ended yet
                else:
                    # region not started in middle of current blocks
                    if self.Offset <= Start:
                        UsedBlockNum = BlockNum
                    # region started in middle of current blocks
                    else:
                        UsedBlockNum = (End - self.Offset) // BlockSize
                    Start = End
                    ExpectedList.append((BlockSize, UsedBlockNum))
                    RemindingSize -= BlockSize * UsedBlockNum

        if FvObj.BlockSizeList == []:
            FvObj.BlockSizeList = ExpectedList
        else:
            # first check whether FvObj.BlockSizeList items have only "BlockSize" or "NumBlocks",
            # if so, use ExpectedList
            for Item in FvObj.BlockSizeList:
                if Item[0] is None or Item[1] is None:
                    FvObj.BlockSizeList = ExpectedList
                    break
            # make sure region size is no smaller than the summed block size in FV
            Sum = 0
            for Item in FvObj.BlockSizeList:
                Sum += Item[0] * Item[1]
            if self.Size < Sum:
                EdkLogger.error("GenFds", GENFDS_ERROR, "Total Size of FV %s 0x%x is larger than Region Size 0x%x "
                                % (FvObj.UiFvName, Sum, self.Size))
            # check whether the BlockStatements in FV section is appropriate
            ExpectedListData = ''
            for Item in ExpectedList:
                ExpectedListData += "BlockSize = 0x%x\n\tNumBlocks = 0x%x\n\t" % Item
            Index = 0
            for Item in FvObj.BlockSizeList:
                if Item[0] != ExpectedList[Index][0]:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "BlockStatements of FV %s are not align with FD's, suggested FV BlockStatement"
                                    % FvObj.UiFvName, ExtraData=ExpectedListData)
                elif Item[1] != ExpectedList[Index][1]:
                    if (Item[1] < ExpectedList[Index][1]) and (Index == len(FvObj.BlockSizeList) - 1):
                        break;
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "BlockStatements of FV %s are not align with FD's, suggested FV BlockStatement"
                                        % FvObj.UiFvName, ExtraData=ExpectedListData)
                else:
                    Index += 1



