## @file
# process FV generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.LongFilePathOs as os
import subprocess
from io import BytesIO
from struct import *
from . import FfsFileStatement
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from Common.Misc import SaveFileOnChange, PackGUID
from Common.LongFilePathSupport import CopyLongFilePath
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.DataType import *

FV_UI_EXT_ENTY_GUID = 'A67DF1FA-8DE8-4E98-AF09-4BDF2EFFBC7C'

## generate FV
#
#
class FV (object):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self, Name=None):
        self.UiFvName = Name
        self.CreateFileName = None
        self.BlockSizeList = []
        self.DefineVarDict = {}
        self.SetVarDict = {}
        self.FvAlignment = None
        self.FvAttributeDict = {}
        self.FvNameGuid = None
        self.FvNameString = None
        self.AprioriSectionList = []
        self.FfsList = []
        self.BsBaseAddress = None
        self.RtBaseAddress = None
        self.FvInfFile = None
        self.FvAddressFile = None
        self.BaseAddress = None
        self.InfFileName = None
        self.FvAddressFileName = None
        self.CapsuleName = None
        self.FvBaseAddress = None
        self.FvForceRebase = None
        self.FvRegionInFD = None
        self.UsedSizeEnable = False
        self.FvExtEntryTypeValue = []
        self.FvExtEntryType = []
        self.FvExtEntryData = []
    ## AddToBuffer()
    #
    #   Generate Fv and add it to the Buffer
    #
    #   @param  self        The object pointer
    #   @param  Buffer      The buffer generated FV data will be put
    #   @param  BaseAddress base address of FV
    #   @param  BlockSize   block size of FV
    #   @param  BlockNum    How many blocks in FV
    #   @param  ErasePolarity      Flash erase polarity
    #   @param  MacroDict   macro value pair
    #   @retval string      Generated FV file path
    #
    def AddToBuffer (self, Buffer, BaseAddress=None, BlockSize= None, BlockNum=None, ErasePloarity='1',  MacroDict = {}, Flag=False):
        if BaseAddress is None and self.UiFvName.upper() + 'fv' in GenFdsGlobalVariable.ImageBinDict:
            return GenFdsGlobalVariable.ImageBinDict[self.UiFvName.upper() + 'fv']

        #
        # Check whether FV in Capsule is in FD flash region.
        # If yes, return error. Doesn't support FV in Capsule image is also in FD flash region.
        #
        if self.CapsuleName is not None:
            for FdObj in GenFdsGlobalVariable.FdfParser.Profile.FdDict.values():
                for RegionObj in FdObj.RegionList:
                    if RegionObj.RegionType == BINARY_FILE_TYPE_FV:
                        for RegionData in RegionObj.RegionDataList:
                            if RegionData.endswith(".fv"):
                                continue
                            elif RegionData.upper() + 'fv' in GenFdsGlobalVariable.ImageBinDict:
                                continue
                            elif self.UiFvName.upper() == RegionData.upper():
                                GenFdsGlobalVariable.ErrorLogger("Capsule %s in FD region can't contain a FV %s in FD region." % (self.CapsuleName, self.UiFvName.upper()))
        if not Flag:
            GenFdsGlobalVariable.InfLogger( "\nGenerating %s FV" %self.UiFvName)
        GenFdsGlobalVariable.LargeFileInFvFlags.append(False)
        FFSGuid = None

        if self.FvBaseAddress is not None:
            BaseAddress = self.FvBaseAddress
        if not Flag:
            self._InitializeInf(BaseAddress, BlockSize, BlockNum, ErasePloarity)
        #
        # First Process the Apriori section
        #
        MacroDict.update(self.DefineVarDict)

        GenFdsGlobalVariable.VerboseLogger('First generate Apriori file !')
        FfsFileList = []
        for AprSection in self.AprioriSectionList:
            FileName = AprSection.GenFfs (self.UiFvName, MacroDict, IsMakefile=Flag)
            FfsFileList.append(FileName)
            # Add Apriori file name to Inf file
            if not Flag:
                self.FvInfFile.append("EFI_FILE_NAME = " + \
                                            FileName          + \
                                            TAB_LINE_BREAK)

        # Process Modules in FfsList
        for FfsFile in self.FfsList:
            if Flag:
                if isinstance(FfsFile, FfsFileStatement.FileStatement):
                    continue
            if GenFdsGlobalVariable.EnableGenfdsMultiThread and GenFdsGlobalVariable.ModuleFile and GenFdsGlobalVariable.ModuleFile.Path.find(os.path.normpath(FfsFile.InfFileName)) == -1:
                continue
            FileName = FfsFile.GenFfs(MacroDict, FvParentAddr=BaseAddress, IsMakefile=Flag, FvName=self.UiFvName)
            FfsFileList.append(FileName)
            if not Flag:
                self.FvInfFile.append("EFI_FILE_NAME = " + \
                                            FileName          + \
                                            TAB_LINE_BREAK)
        if not Flag:
            FvInfFile = ''.join(self.FvInfFile)
            SaveFileOnChange(self.InfFileName, FvInfFile, False)
        #
        # Call GenFv tool
        #
        FvOutputFile = os.path.join(GenFdsGlobalVariable.FvDir, self.UiFvName)
        FvOutputFile = FvOutputFile + '.Fv'
        # BUGBUG: FvOutputFile could be specified from FDF file (FV section, CreateFile statement)
        if self.CreateFileName is not None:
            FvOutputFile = self.CreateFileName

        if Flag:
            GenFdsGlobalVariable.ImageBinDict[self.UiFvName.upper() + 'fv'] = FvOutputFile
            return FvOutputFile

        FvInfoFileName = os.path.join(GenFdsGlobalVariable.FfsDir, self.UiFvName + '.inf')
        if not Flag:
            CopyLongFilePath(GenFdsGlobalVariable.FvAddressFileName, FvInfoFileName)
            OrigFvInfo = None
            if os.path.exists (FvInfoFileName):
                OrigFvInfo = open(FvInfoFileName, 'r').read()
            if GenFdsGlobalVariable.LargeFileInFvFlags[-1]:
                FFSGuid = GenFdsGlobalVariable.EFI_FIRMWARE_FILE_SYSTEM3_GUID
            GenFdsGlobalVariable.GenerateFirmwareVolume(
                                    FvOutputFile,
                                    [self.InfFileName],
                                    AddressFile=FvInfoFileName,
                                    FfsList=FfsFileList,
                                    ForceRebase=self.FvForceRebase,
                                    FileSystemGuid=FFSGuid
                                    )

            NewFvInfo = None
            if os.path.exists (FvInfoFileName):
                NewFvInfo = open(FvInfoFileName, 'r').read()
            if NewFvInfo is not None and NewFvInfo != OrigFvInfo:
                FvChildAddr = []
                AddFileObj = open(FvInfoFileName, 'r')
                AddrStrings = AddFileObj.readlines()
                AddrKeyFound = False
                for AddrString in AddrStrings:
                    if AddrKeyFound:
                        #get base address for the inside FvImage
                        FvChildAddr.append (AddrString)
                    elif AddrString.find ("[FV_BASE_ADDRESS]") != -1:
                        AddrKeyFound = True
                AddFileObj.close()

                if FvChildAddr != []:
                    # Update Ffs again
                    for FfsFile in self.FfsList:
                        FileName = FfsFile.GenFfs(MacroDict, FvChildAddr, BaseAddress, IsMakefile=Flag, FvName=self.UiFvName)

                    if GenFdsGlobalVariable.LargeFileInFvFlags[-1]:
                        FFSGuid = GenFdsGlobalVariable.EFI_FIRMWARE_FILE_SYSTEM3_GUID;
                    #Update GenFv again
                    GenFdsGlobalVariable.GenerateFirmwareVolume(
                                                FvOutputFile,
                                                [self.InfFileName],
                                                AddressFile=FvInfoFileName,
                                                FfsList=FfsFileList,
                                                ForceRebase=self.FvForceRebase,
                                                FileSystemGuid=FFSGuid
                                                )

            #
            # Write the Fv contents to Buffer
            #
            if os.path.isfile(FvOutputFile) and os.path.getsize(FvOutputFile) >= 0x48:
                FvFileObj = open(FvOutputFile, 'rb')
                # PI FvHeader is 0x48 byte
                FvHeaderBuffer = FvFileObj.read(0x48)
                Signature = FvHeaderBuffer[0x28:0x32]
                if Signature and Signature.startswith(b'_FVH'):
                    GenFdsGlobalVariable.VerboseLogger("\nGenerate %s FV Successfully" % self.UiFvName)
                    GenFdsGlobalVariable.SharpCounter = 0

                    FvFileObj.seek(0)
                    Buffer.write(FvFileObj.read())
                    # FV alignment position.
                    FvAlignmentValue = 1 << (ord(FvHeaderBuffer[0x2E:0x2F]) & 0x1F)
                    if FvAlignmentValue >= 0x400:
                        if FvAlignmentValue >= 0x100000:
                            if FvAlignmentValue >= 0x1000000:
                            #The max alignment supported by FFS is 16M.
                                self.FvAlignment = "16M"
                            else:
                                self.FvAlignment = str(FvAlignmentValue // 0x100000) + "M"
                        else:
                            self.FvAlignment = str(FvAlignmentValue // 0x400) + "K"
                    else:
                        # FvAlignmentValue is less than 1K
                        self.FvAlignment = str (FvAlignmentValue)
                    FvFileObj.close()
                    GenFdsGlobalVariable.ImageBinDict[self.UiFvName.upper() + 'fv'] = FvOutputFile
                    GenFdsGlobalVariable.LargeFileInFvFlags.pop()
                else:
                    GenFdsGlobalVariable.ErrorLogger("Invalid FV file %s." % self.UiFvName)
            else:
                GenFdsGlobalVariable.ErrorLogger("Failed to generate %s FV file." %self.UiFvName)
        return FvOutputFile

    ## _GetBlockSize()
    #
    #   Calculate FV's block size
    #   Inherit block size from FD if no block size specified in FV
    #
    def _GetBlockSize(self):
        if self.BlockSizeList:
            return True

        for FdObj in GenFdsGlobalVariable.FdfParser.Profile.FdDict.values():
            for RegionObj in FdObj.RegionList:
                if RegionObj.RegionType != BINARY_FILE_TYPE_FV:
                    continue
                for RegionData in RegionObj.RegionDataList:
                    #
                    # Found the FD and region that contain this FV
                    #
                    if self.UiFvName.upper() == RegionData.upper():
                        RegionObj.BlockInfoOfRegion(FdObj.BlockSizeList, self)
                        if self.BlockSizeList:
                            return True
        return False

    ## _InitializeInf()
    #
    #   Initialize the inf file to create FV
    #
    #   @param  self        The object pointer
    #   @param  BaseAddress base address of FV
    #   @param  BlockSize   block size of FV
    #   @param  BlockNum    How many blocks in FV
    #   @param  ErasePolarity      Flash erase polarity
    #
    def _InitializeInf (self, BaseAddress = None, BlockSize= None, BlockNum = None, ErasePloarity='1'):
        #
        # Create FV inf file
        #
        self.InfFileName = os.path.join(GenFdsGlobalVariable.FvDir,
                                   self.UiFvName + '.inf')
        self.FvInfFile = []

        #
        # Add [Options]
        #
        self.FvInfFile.append("[options]" + TAB_LINE_BREAK)
        if BaseAddress is not None:
            self.FvInfFile.append("EFI_BASE_ADDRESS = " + \
                                       BaseAddress          + \
                                       TAB_LINE_BREAK)

        if BlockSize is not None:
            self.FvInfFile.append("EFI_BLOCK_SIZE = " + \
                                      '0x%X' %BlockSize    + \
                                      TAB_LINE_BREAK)
            if BlockNum is not None:
                self.FvInfFile.append("EFI_NUM_BLOCKS   = "  + \
                                      ' 0x%X' %BlockNum    + \
                                      TAB_LINE_BREAK)
        else:
            if self.BlockSizeList == []:
                if not self._GetBlockSize():
                    #set default block size is 1
                    self.FvInfFile.append("EFI_BLOCK_SIZE  = 0x1" + TAB_LINE_BREAK)

            for BlockSize in self.BlockSizeList:
                if BlockSize[0] is not None:
                    self.FvInfFile.append("EFI_BLOCK_SIZE  = "  + \
                                          '0x%X' %BlockSize[0]    + \
                                          TAB_LINE_BREAK)

                if BlockSize[1] is not None:
                    self.FvInfFile.append("EFI_NUM_BLOCKS   = "  + \
                                          ' 0x%X' %BlockSize[1]    + \
                                          TAB_LINE_BREAK)

        if self.BsBaseAddress is not None:
            self.FvInfFile.append('EFI_BOOT_DRIVER_BASE_ADDRESS = ' + \
                                       '0x%X' %self.BsBaseAddress)
        if self.RtBaseAddress is not None:
            self.FvInfFile.append('EFI_RUNTIME_DRIVER_BASE_ADDRESS = ' + \
                                      '0x%X' %self.RtBaseAddress)
        #
        # Add attribute
        #
        self.FvInfFile.append("[attributes]" + TAB_LINE_BREAK)

        self.FvInfFile.append("EFI_ERASE_POLARITY   = "       + \
                                          ' %s' %ErasePloarity    + \
                                          TAB_LINE_BREAK)
        if not (self.FvAttributeDict is None):
            for FvAttribute in self.FvAttributeDict.keys():
                if FvAttribute == "FvUsedSizeEnable":
                    if self.FvAttributeDict[FvAttribute].upper() in ('TRUE', '1'):
                        self.UsedSizeEnable = True
                    continue
                self.FvInfFile.append("EFI_"            + \
                                          FvAttribute       + \
                                          ' = '             + \
                                          self.FvAttributeDict[FvAttribute] + \
                                          TAB_LINE_BREAK )
        if self.FvAlignment is not None:
            self.FvInfFile.append("EFI_FVB2_ALIGNMENT_"     + \
                                       self.FvAlignment.strip() + \
                                       " = TRUE"                + \
                                       TAB_LINE_BREAK)

        #
        # Generate FV extension header file
        #
        if not self.FvNameGuid:
            if len(self.FvExtEntryType) > 0 or self.UsedSizeEnable:
                GenFdsGlobalVariable.ErrorLogger("FV Extension Header Entries declared for %s with no FvNameGuid declaration." % (self.UiFvName))
        else:
            TotalSize = 16 + 4
            Buffer = bytearray()
            if self.UsedSizeEnable:
                TotalSize += (4 + 4)
                ## define EFI_FV_EXT_TYPE_USED_SIZE_TYPE 0x03
                #typedef  struct
                # {
                #    EFI_FIRMWARE_VOLUME_EXT_ENTRY Hdr;
                #    UINT32 UsedSize;
                # } EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE;
                Buffer += pack('HHL', 8, 3, 0)

            if self.FvNameString == 'TRUE':
                #
                # Create EXT entry for FV UI name
                # This GUID is used: A67DF1FA-8DE8-4E98-AF09-4BDF2EFFBC7C
                #
                FvUiLen = len(self.UiFvName)
                TotalSize += (FvUiLen + 16 + 4)
                Guid = FV_UI_EXT_ENTY_GUID.split('-')
                #
                # Layout:
                #   EFI_FIRMWARE_VOLUME_EXT_ENTRY: size 4
                #   GUID: size 16
                #   FV UI name
                #
                Buffer += (pack('HH', (FvUiLen + 16 + 4), 0x0002)
                           + PackGUID(Guid)
                           + self.UiFvName.encode('utf-8'))

            for Index in range (0, len(self.FvExtEntryType)):
                if self.FvExtEntryType[Index] == 'FILE':
                    # check if the path is absolute or relative
                    if os.path.isabs(self.FvExtEntryData[Index]):
                        FileFullPath = os.path.normpath(self.FvExtEntryData[Index])
                    else:
                        FileFullPath = os.path.normpath(os.path.join(GenFdsGlobalVariable.WorkSpaceDir, self.FvExtEntryData[Index]))
                    # check if the file path exists or not
                    if not os.path.isfile(FileFullPath):
                        GenFdsGlobalVariable.ErrorLogger("Error opening FV Extension Header Entry file %s." % (self.FvExtEntryData[Index]))
                    FvExtFile = open (FileFullPath, 'rb')
                    FvExtFile.seek(0, 2)
                    Size = FvExtFile.tell()
                    if Size >= 0x10000:
                        GenFdsGlobalVariable.ErrorLogger("The size of FV Extension Header Entry file %s exceeds 0x10000." % (self.FvExtEntryData[Index]))
                    TotalSize += (Size + 4)
                    FvExtFile.seek(0)
                    Buffer += pack('HH', (Size + 4), int(self.FvExtEntryTypeValue[Index], 16))
                    Buffer += FvExtFile.read()
                    FvExtFile.close()
                if self.FvExtEntryType[Index] == 'DATA':
                    ByteList = self.FvExtEntryData[Index].split(',')
                    Size = len (ByteList)
                    if Size >= 0x10000:
                        GenFdsGlobalVariable.ErrorLogger("The size of FV Extension Header Entry data %s exceeds 0x10000." % (self.FvExtEntryData[Index]))
                    TotalSize += (Size + 4)
                    Buffer += pack('HH', (Size + 4), int(self.FvExtEntryTypeValue[Index], 16))
                    for Index1 in range (0, Size):
                        Buffer += pack('B', int(ByteList[Index1], 16))

            Guid = self.FvNameGuid.split('-')
            Buffer = PackGUID(Guid) + pack('=L', TotalSize) + Buffer

            #
            # Generate FV extension header file if the total size is not zero
            #
            if TotalSize > 0:
                FvExtHeaderFileName = os.path.join(GenFdsGlobalVariable.FvDir, self.UiFvName + '.ext')
                FvExtHeaderFile = BytesIO()
                FvExtHeaderFile.write(Buffer)
                Changed = SaveFileOnChange(FvExtHeaderFileName, FvExtHeaderFile.getvalue(), True)
                FvExtHeaderFile.close()
                if Changed:
                  if os.path.exists (self.InfFileName):
                    os.remove (self.InfFileName)
                self.FvInfFile.append("EFI_FV_EXT_HEADER_FILE_NAME = "      + \
                                           FvExtHeaderFileName                  + \
                                           TAB_LINE_BREAK)

        #
        # Add [Files]
        #
        self.FvInfFile.append("[files]" + TAB_LINE_BREAK)
