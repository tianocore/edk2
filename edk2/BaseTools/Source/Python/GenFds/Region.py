## @file
# process FD Region generation
#
#  Copyright (c) 2007, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
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
import os
from Common import EdkLogger
from Common.BuildToolError import *


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

    def AddToBuffer(self, Buffer, BaseAddress, BlockSizeList, ErasePolarity, FvBinDict, vtfDict = None, MacroDict = {}):
        Size = self.Size
        GenFdsGlobalVariable.InfLogger('Generate Region at Offset 0x%X' % self.Offset)
        GenFdsGlobalVariable.InfLogger("   Region Size = 0x%X" %Size)
        GenFdsGlobalVariable.SharpCounter = 0

        if self.RegionType == 'FV':
            #
            # Get Fv from FvDict
            #
            FvBuffer = StringIO.StringIO('')
            RegionBlockSize = self.BlockSizeOfRegion(BlockSizeList)
            RegionBlockNum = self.BlockNumOfRegion(RegionBlockSize)

            self.FvAddress = int(BaseAddress, 16) + self.Offset
            FvBaseAddress = '0x%X' %self.FvAddress

            for RegionData in self.RegionDataList:

                if RegionData.endswith(".fv"):
                    RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                    GenFdsGlobalVariable.InfLogger('   Region FV File Name = .fv : %s'%RegionData)
                    if RegionData[1] != ':' :
                        RegionData = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                    if not os.path.exists(RegionData):
                        EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)

                    BinFile = open (RegionData, 'r+b')
                    FvBuffer.write(BinFile.read())
                    if FvBuffer.len > Size:
                        EdkLogger.error("GenFds", GENFDS_ERROR,
                                        "Size of FV File (%s) is larger than Region Size 0x%X specified." \
                                        % (RegionData, Size))
                    break

                if RegionData.upper() in FvBinDict.keys():
                    continue

                FvObj = None
                if RegionData.upper() in GenFdsGlobalVariable.FdfParser.Profile.FvDict.keys():
                    FvObj = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(RegionData.upper())

                if FvObj != None :
                    GenFdsGlobalVariable.InfLogger('   Region Name = FV')
                    #
                    # Call GenFv tool
                    #
                    BlockSize = RegionBlockSize
                    BlockNum = RegionBlockNum
                    if FvObj.BlockSizeList != []:
                        if FvObj.BlockSizeList[0][0] != None:
                            BlockSize = FvObj.BlockSizeList[0][0]
                        if FvObj.BlockSizeList[0][1] != None:
                            BlockNum = FvObj.BlockSizeList[0][1]
                    self.FvAddress = self.FvAddress + FvBuffer.len
                    FvAlignValue = self.GetFvAlignValue(FvObj.FvAlignment)
                    if self.FvAddress % FvAlignValue != 0:
                        EdkLogger.error("GenFds", GENFDS_ERROR,
                                        "FV (%s) is NOT %s Aligned!" % (FvObj.UiFvName, FvObj.FvAlignment))
                    FvBaseAddress = '0x%X' %self.FvAddress
                    FileName = FvObj.AddToBuffer(FvBuffer, FvBaseAddress, BlockSize, BlockNum, ErasePolarity, vtfDict)

                    if FvBuffer.len > Size:
                        EdkLogger.error("GenFds", GENFDS_ERROR,
                                        "Size of FV (%s) is larger than Region Size 0x%X specified." % (RegionData, Size))
                else:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "FV (%s) is NOT described in FDF file!" % (RegionData))


            if FvBuffer.len > 0:
                Buffer.write(FvBuffer.getvalue())
            else:
                BinFile = open (FileName, 'rb')
                Buffer.write(BinFile.read())

            FvBuffer.close()

        if self.RegionType == 'FILE':
            FvBuffer = StringIO.StringIO('')
            for RegionData in self.RegionDataList:
                RegionData = GenFdsGlobalVariable.MacroExtend(RegionData, MacroDict)
                GenFdsGlobalVariable.InfLogger('   Region File Name = FILE: %s'%RegionData)
                if RegionData[1] != ':' :
                    RegionData = os.path.join (GenFdsGlobalVariable.WorkSpaceDir, RegionData)
                if not os.path.exists(RegionData):
                    EdkLogger.error("GenFds", FILE_NOT_FOUND, ExtraData=RegionData)

                BinFile = open (RegionData, 'r+b')
                FvBuffer.write(BinFile.read())
                if FvBuffer.len > Size :
                    EdkLogger.error("GenFds", GENFDS_ERROR,
                                    "Size of File (%s) large than Region Size " % RegionData)

            #
            # If File contents less than region size, append "0xff" after it
            #
            if FvBuffer.len < Size:
                for index in range(0, (Size-FvBuffer.len)):
                    if (ErasePolarity == '1'):
                        FvBuffer.write(pack('B', int('0xFF', 16)))
                    else:
                        FvBuffer.write(pack('B', int('0x00', 16)))
            Buffer.write(FvBuffer.getvalue())
            FvBuffer.close()

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
            if DataSize < Size:
                if (ErasePolarity == '1'):
                    PadData = 0xFF
                else:
                    PadData = 0
                for i in range(Size - DataSize):
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
    #   @retval int                  Block size of region
    #
    def BlockSizeOfRegion(self, BlockSizeList):
        Offset = 0x00
        BlockSize = 0
        for item in BlockSizeList:
            Offset = Offset + item[0]  * item[1]
            GenFdsGlobalVariable.VerboseLogger ("Offset = 0x%X" %Offset)
            GenFdsGlobalVariable.VerboseLogger ("self.Offset 0x%X" %self.Offset)

            if self.Offset < Offset :
                if Offset - self.Offset < self.Size:
                    EdkLogger.error("GenFds", GENFDS_ERROR,
                                    "Region at Offset 0x%X can NOT fit into Block array with BlockSize %X" \
                                    % (self.Offset, item[0]))
                BlockSize = item[0]
                GenFdsGlobalVariable.VerboseLogger ("BlockSize = %X" %BlockSize)
                return BlockSize
        return BlockSize

    ## BlockNumOfRegion()
    #
    #   @param  BlockSize            block size of region
    #   @retval int                  Block number of region
    #
    def BlockNumOfRegion (self, BlockSize):
        if BlockSize == 0 :
            EdkLogger.error("GenFds", GENFDS_ERROR, "Region: %s is not in the FD address scope!" % self.Offset)
        BlockNum = self.Size / BlockSize
        GenFdsGlobalVariable.VerboseLogger ("BlockNum = 0x%X" %BlockNum)
        return BlockNum

