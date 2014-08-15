## @file
# process FD Region generation
#
#  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
from struct import *
from GenFdsGlobalVariable import GenFdsGlobalVariable
import StringIO
from CommonDataClass.FdfClass import RegionClassObject
import Common.LongFilePathOs as os
from stat import *
from Common import EdkLogger
from Common.BuildToolError import *
from Common.LongFilePathSupport import OpenLongFilePath as open

## generate Region
#
#
class Region(RegionClassObject):

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        RegionClassObject.__init__(self)


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
    #   @param  VtfDict     VTF objects
    #   @param  MacroDict   macro value pair
    #   @retval string      Generated FV file path
    #

    def AddToBuffer(self, Buffer, BaseAddress, BlockSizeList, ErasePolarity, ImageBinDict, vtfDict = None, MacroDict = {}):
        Size = self.Size
        GenFdsGlobalVariable.InfLogger('\nGenerate Region at Offset 0x%X' % self.Offset)
        GenFdsGlobalVariable.InfLogger("   Region Size = 0x%X" %Size)
        GenFdsGlobalVariable.SharpCounter = 0

        if self.RegionType == 'FV':
            #
            # Get Fv from FvDict
            #
            self.FvAddress = int(BaseAddress, 16) + self.Offset
            FvBaseAddress  = '0x%X' %self.FvAddress
            FvOffset       = 0
            for RegionData in self.RegionDataList:
                FileName = None
                if RegionData.endswith(".fv"):
                    RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                    GenFdsGlobalVariable.InfLogger('   Region FV File Name = .fv : %s'%RegionData)
                    if RegionData[1] != ':' :
                        RegionData = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                    if not os.path.exists(RegionData):
                        EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)

                    FileName = RegionData
                elif RegionData.upper() + 'fv' in ImageBinDict.keys():
                    GenFdsGlobalVariable.InfLogger('   Region Name = FV')
                    FileName = ImageBinDict[RegionData.upper() + 'fv']
                else:
                    #
                    # Generate FvImage.
                    #
                    FvObj = None
                    if RegionData.upper() in GenFdsGlobalVariable.FdfParser.Profile.FvDict.keys():
                        FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(RegionData.upper())

                    if FvObj != None :
                        GenFdsGlobalVariable.InfLogger('   Region Name = FV')
                        #
                        # Call GenFv tool
                        #
                        self.BlockInfoOfRegion(BlockSizeList, FvObj)
                        self.FvAddress = self.FvAddress + FvOffset
                        FvAlignValue = self.GetFvAlignValue(FvObj.FvAlignment)
                        if self.FvAddress % FvAlignValue != 0:
                            EdkLogger.error("GenFds", GENFDS_ERROR,
                                            "FV (%s) is NOT %s Aligned!" % (FvObj.UiFvName, FvObj.FvAlignment))
                        FvBuffer = StringIO.StringIO('')
                        FvBaseAddress = '0x%X' %self.FvAddress
                        BlockSize = None
                        BlockNum = None
                        FvObj.AddToBuffer(FvBuffer, FvBaseAddress, BlockSize, BlockNum, ErasePolarity, vtfDict)
                        if FvBuffer.len > Size:
                            FvBuffer.close()
                            EdkLogger.error("GenFds", GENFDS_ERROR,
                                            "Size of FV (%s) is larger than Region Size 0x%X specified." % (RegionData, Size))
                        #
                        # Put the generated image into FD buffer.
                        #
                        Buffer.write(FvBuffer.getvalue())
                        FvBuffer.close()
                        FvOffset = FvOffset + FvBuffer.len
                        Size = Size - FvBuffer.len
                        continue
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "FV (%s) is NOT described in FDF file!" % (RegionData))
                #
                # Add the exist Fv image into FD buffer
                #
                if FileName != None:
                    FileLength = os.stat(FileName)[ST_SIZE]
                    if FileLength > Size:
                        EdkLogger.error("GenFds", GENFDS_ERROR,
                                        "Size of FV File (%s) is larger than Region Size 0x%X specified." \
                                        % (RegionData, Size))
                    BinFile = open (FileName, 'r+b')
                    Buffer.write(BinFile.read())
                    BinFile.close()
                    Size = Size - FileLength
            #
            # Pad the left buffer
            #
            if Size > 0:
                if (ErasePolarity == '1') :
                    PadData = 0xFF
                else :
                    PadData = 0
                for i in range(0, Size):
                    Buffer.write(pack('B', PadData))

        if self.RegionType == 'CAPSULE':
            #
            # Get Capsule from Capsule Dict
            #
            for RegionData in self.RegionDataList:
                if RegionData.endswith(".cap"):
                    RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                    GenFdsGlobalVariable.InfLogger('   Region CAPSULE Image Name = .cap : %s'%RegionData)
                    if RegionData[1] != ':' :
                        RegionData = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                    if not os.path.exists(RegionData):
                        EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)

                    FileName = RegionData
                elif RegionData.upper() + 'cap' in ImageBinDict.keys():
                    GenFdsGlobalVariable.InfLogger('   Region Name = CAPSULE')
                    FileName = ImageBinDict[RegionData.upper() + 'cap']
                else:
                    #
                    # Generate Capsule image and Put it into FD buffer
                    #
                    CapsuleObj = None
                    if RegionData.upper() in GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict.keys():
                        CapsuleObj = GenFdsGlobalVariable.FdfParser.Profile.CapsuleDict[RegionData.upper()]

                    if CapsuleObj != None :
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
                BinFile = open (FileName, 'r+b')
                Buffer.write(BinFile.read())
                BinFile.close()
                Size = Size - FileLength
            #
            # Pad the left buffer
            #
            if Size > 0:
                if (ErasePolarity == '1') :
                    PadData = 0xFF
                else :
                    PadData = 0
                for i in range(0, Size):
                    Buffer.write(pack('B', PadData))

        if self.RegionType == 'FILE':
            for RegionData in self.RegionDataList:
                RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                if RegionData[1] != ':' :
                    RegionData = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
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
                GenFdsGlobalVariable.InfLogger('   Region File Name = %s'%RegionData)
                BinFile = open (RegionData, 'rb')
                Buffer.write(BinFile.read())
                BinFile.close()
                Size = Size - FileLength
            #
            # Pad the left buffer
            #
            if Size > 0:
                if (ErasePolarity == '1') :
                    PadData = 0xFF
                else :
                    PadData = 0
                for i in range(0, Size):
                    Buffer.write(pack('B', PadData))

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
            if Size > 0:
                if (ErasePolarity == '1') :
                    PadData = 0xFF
                else :
                    PadData = 0
                for i in range(0, Size):
                    Buffer.write(pack('B', PadData))

        if self.RegionType == None:
            GenFdsGlobalVariable.InfLogger('   Region Name = None')
            if (ErasePolarity == '1') :
                PadData = 0xFF
            else :
                PadData = 0
            for i in range(0, Size):
                Buffer.write(pack('B', PadData))

    def GetFvAlignValue(self, Str):
        AlignValue = 1
        Granu = 1
        Str = Str.strip().upper()
        if Str.endswith('K'):
            Granu = 1024
            Str = Str[:-1]
        elif Str.endswith('M'):
            Granu = 1024*1024
            Str = Str[:-1]
        elif Str.endswith('G'):
            Granu = 1024*1024*1024
            Str = Str[:-1]
        else:
            pass

        AlignValue = int(Str)*Granu
        return AlignValue
 
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
                    ExpectedList.append((BlockSize, (RemindingSize + BlockSize - 1)/BlockSize))
                    break
                # region not ended yet
                else:
                    # region not started in middle of current blocks
                    if self.Offset <= Start:
                        UsedBlockNum = BlockNum
                    # region started in middle of current blocks
                    else:
                        UsedBlockNum = (End - self.Offset)/BlockSize
                    Start = End
                    ExpectedList.append((BlockSize, UsedBlockNum))
                    RemindingSize -= BlockSize * UsedBlockNum
                   
        if FvObj.BlockSizeList == []:
            FvObj.BlockSizeList = ExpectedList
        else:
            # first check whether FvObj.BlockSizeList items have only "BlockSize" or "NumBlocks",
            # if so, use ExpectedList
            for Item in FvObj.BlockSizeList:
                if Item[0] == None or Item[1] == None:
                    FvObj.BlockSizeList = ExpectedList
                    break
            # make sure region size is no smaller than the summed block size in FV
            Sum = 0
            for Item in FvObj.BlockSizeList:
                Sum += Item[0] * Item[1]
            if self.Size < Sum:
                EdkLogger.error("GenFds", GENFDS_ERROR, "Total Size of FV %s 0x%x is larger than Region Size 0x%x "
                                %(FvObj.UiFvName, Sum, self.Size))
            # check whether the BlockStatements in FV section is appropriate
            ExpectedListData = ''
            for Item in ExpectedList:
                ExpectedListData += "BlockSize = 0x%x\n\tNumBlocks = 0x%x\n\t"%Item 
            Index = 0
            for Item in FvObj.BlockSizeList:
                if Item[0] != ExpectedList[Index][0]:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "BlockStatements of FV %s are not align with FD's, suggested FV BlockStatement"
                                    %FvObj.UiFvName, ExtraData = ExpectedListData)
                elif Item[1] != ExpectedList[Index][1]:
                    if (Item[1] < ExpectedList[Index][1]) and (Index == len(FvObj.BlockSizeList) - 1):
                        break;
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "BlockStatements of FV %s are not align with FD's, suggested FV BlockStatement"
                                        %FvObj.UiFvName, ExtraData = ExpectedListData)
                else:
                    Index += 1

            

