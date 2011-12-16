## @file
# process FV generation
#
#  Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
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
import os
import shutil
import subprocess
import StringIO
from struct import *

import Ffs
import AprioriSection
from GenFdsGlobalVariable import GenFdsGlobalVariable
from GenFds import GenFds
from CommonDataClass.FdfClass import FvClassObject
from Common.Misc import SaveFileOnChange

T_CHAR_LF = '\n'

## generate FV
#
#
class FV (FvClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FvClassObject.__init__(self)
        self.FvInfFile = None
        self.FvAddressFile = None
        self.BaseAddress = None
        self.InfFileName = None
        self.FvAddressFileName = None
        self.CapsuleName = None
        self.FvBaseAddress = None
        self.FvForceRebase = None
        
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
    #   @param  VtfDict     VTF objects
    #   @param  MacroDict   macro value pair
    #   @retval string      Generated FV file path
    #
    def AddToBuffer (self, Buffer, BaseAddress=None, BlockSize= None, BlockNum=None, ErasePloarity='1', VtfDict=None, MacroDict = {}) :

        if BaseAddress == None and self.UiFvName.upper() + 'fv' in GenFds.ImageBinDict.keys():
            return GenFds.ImageBinDict[self.UiFvName.upper() + 'fv']
        
        #
        # Check whether FV in Capsule is in FD flash region.
        # If yes, return error. Doesn't support FV in Capsule image is also in FD flash region.
        #
        if self.CapsuleName != None:
            for FdName in GenFdsGlobalVariable.FdfParser.Profile.FdDict.keys():
                FdObj = GenFdsGlobalVariable.FdfParser.Profile.FdDict[FdName]
                for RegionObj in FdObj.RegionList:
                    if RegionObj.RegionType == 'FV':
                        for RegionData in RegionObj.RegionDataList:
                            if RegionData.endswith(".fv"):
                                continue
                            elif RegionData.upper() + 'fv' in GenFds.ImageBinDict.keys():
                                continue
                            elif self.UiFvName.upper() == RegionData.upper():
                                GenFdsGlobalVariable.ErrorLogger("Capsule %s in FD region can't contain a FV %s in FD region." % (self.CapsuleName, self.UiFvName.upper()))

        GenFdsGlobalVariable.InfLogger( "\nGenerating %s FV" %self.UiFvName)
        
        if self.FvBaseAddress != None:
            BaseAddress = self.FvBaseAddress

        self.__InitializeInf__(BaseAddress, BlockSize, BlockNum, ErasePloarity, VtfDict)
        #
        # First Process the Apriori section
        #
        MacroDict.update(self.DefineVarDict)

        GenFdsGlobalVariable.VerboseLogger('First generate Apriori file !')
        FfsFileList = []
        for AprSection in self.AprioriSectionList:
            FileName = AprSection.GenFfs (self.UiFvName, MacroDict)
            FfsFileList.append(FileName)
            # Add Apriori file name to Inf file
            self.FvInfFile.writelines("EFI_FILE_NAME = " + \
                                       FileName          + \
                                           T_CHAR_LF)

        # Process Modules in FfsList
        for FfsFile in self.FfsList :
            FileName = FfsFile.GenFfs(MacroDict, FvParentAddr=BaseAddress)
            FfsFileList.append(FileName)
            self.FvInfFile.writelines("EFI_FILE_NAME = " + \
                                       FileName          + \
                                       T_CHAR_LF)

        SaveFileOnChange(self.InfFileName, self.FvInfFile.getvalue(), False)
        self.FvInfFile.close()
        #
        # Call GenFv tool
        #
        FvOutputFile = os.path.join(GenFdsGlobalVariable.FvDir, self.UiFvName)
        FvOutputFile = FvOutputFile + '.Fv'
        # BUGBUG: FvOutputFile could be specified from FDF file (FV section, CreateFile statement)
        if self.CreateFileName != None:
            FvOutputFile = self.CreateFileName

        FvInfoFileName = os.path.join(GenFdsGlobalVariable.FfsDir, self.UiFvName + '.inf')
        shutil.copy(GenFdsGlobalVariable.FvAddressFileName, FvInfoFileName)
        OrigFvInfo = None
        if os.path.exists (FvInfoFileName):
            OrigFvInfo = open(FvInfoFileName, 'r').read()
        GenFdsGlobalVariable.GenerateFirmwareVolume(
                                FvOutputFile,
                                [self.InfFileName],
                                AddressFile=FvInfoFileName,
                                FfsList=FfsFileList,
                                ForceRebase=self.FvForceRebase
                                )

        NewFvInfo = None
        if os.path.exists (FvInfoFileName):
            NewFvInfo = open(FvInfoFileName, 'r').read()
        if NewFvInfo != None and NewFvInfo != OrigFvInfo:
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
                for FfsFile in self.FfsList :
                    FileName = FfsFile.GenFfs(MacroDict, FvChildAddr, BaseAddress)
                
                #Update GenFv again
                GenFdsGlobalVariable.GenerateFirmwareVolume(
                                        FvOutputFile,
                                        [self.InfFileName],
                                        AddressFile=FvInfoFileName,
                                        FfsList=FfsFileList,
                                        ForceRebase=self.FvForceRebase
                                        )

        #
        # Write the Fv contents to Buffer
        #
        FvFileObj = open ( FvOutputFile,'r+b')

        GenFdsGlobalVariable.VerboseLogger( "\nGenerate %s FV Successfully" %self.UiFvName)
        GenFdsGlobalVariable.SharpCounter = 0

        Buffer.write(FvFileObj.read())
        FvFileObj.seek(0)
        # PI FvHeader is 0x48 byte
        FvHeaderBuffer = FvFileObj.read(0x48)
        # FV alignment position.
        FvAlignmentValue = 1 << (ord (FvHeaderBuffer[0x2E]) & 0x1F)
        # FvAlignmentValue is larger than or equal to 1K
        if FvAlignmentValue >= 0x400:
            if FvAlignmentValue >= 0x10000:
                #The max alignment supported by FFS is 64K.
                self.FvAlignment = "64K"
            else:
                self.FvAlignment = str (FvAlignmentValue / 0x400) + "K"
        else:
            # FvAlignmentValue is less than 1K
            self.FvAlignment = str (FvAlignmentValue)
        FvFileObj.close()
        GenFds.ImageBinDict[self.UiFvName.upper() + 'fv'] = FvOutputFile
        return FvOutputFile

    ## __InitializeInf__()
    #
    #   Initilize the inf file to create FV
    #
    #   @param  self        The object pointer
    #   @param  BaseAddress base address of FV
    #   @param  BlockSize   block size of FV
    #   @param  BlockNum    How many blocks in FV
    #   @param  ErasePolarity      Flash erase polarity
    #   @param  VtfDict     VTF objects
    #
    def __InitializeInf__ (self, BaseAddress = None, BlockSize= None, BlockNum = None, ErasePloarity='1', VtfDict=None) :
        #
        # Create FV inf file
        #
        self.InfFileName = os.path.join(GenFdsGlobalVariable.FvDir,
                                   self.UiFvName + '.inf')
        self.FvInfFile = StringIO.StringIO()

        #
        # Add [Options]
        #
        self.FvInfFile.writelines("[options]" + T_CHAR_LF)
        if BaseAddress != None :
            self.FvInfFile.writelines("EFI_BASE_ADDRESS = " + \
                                       BaseAddress          + \
                                       T_CHAR_LF)

        if BlockSize != None:
            self.FvInfFile.writelines("EFI_BLOCK_SIZE = " + \
                                      '0x%X' %BlockSize    + \
                                      T_CHAR_LF)
            if BlockNum != None:
                self.FvInfFile.writelines("EFI_NUM_BLOCKS   = "  + \
                                      ' 0x%X' %BlockNum    + \
                                      T_CHAR_LF)
        else:
            if self.BlockSizeList == []:
                #set default block size is 1
                self.FvInfFile.writelines("EFI_BLOCK_SIZE  = 0x1" + T_CHAR_LF)
            
            for BlockSize in self.BlockSizeList :
                if BlockSize[0] != None:
                    self.FvInfFile.writelines("EFI_BLOCK_SIZE  = "  + \
                                          '0x%X' %BlockSize[0]    + \
                                          T_CHAR_LF)

                if BlockSize[1] != None:
                    self.FvInfFile.writelines("EFI_NUM_BLOCKS   = "  + \
                                          ' 0x%X' %BlockSize[1]    + \
                                          T_CHAR_LF)

        if self.BsBaseAddress != None:
            self.FvInfFile.writelines('EFI_BOOT_DRIVER_BASE_ADDRESS = ' + \
                                       '0x%X' %self.BsBaseAddress)
        if self.RtBaseAddress != None:
            self.FvInfFile.writelines('EFI_RUNTIME_DRIVER_BASE_ADDRESS = ' + \
                                      '0x%X' %self.RtBaseAddress)
        #
        # Add attribute
        #
        self.FvInfFile.writelines("[attributes]" + T_CHAR_LF)

        self.FvInfFile.writelines("EFI_ERASE_POLARITY   = "       + \
                                          ' %s' %ErasePloarity    + \
                                          T_CHAR_LF)
        if not (self.FvAttributeDict == None):
            for FvAttribute in self.FvAttributeDict.keys() :
                self.FvInfFile.writelines("EFI_"            + \
                                          FvAttribute       + \
                                          ' = '             + \
                                          self.FvAttributeDict[FvAttribute] + \
                                          T_CHAR_LF )
        if self.FvAlignment != None:
            self.FvInfFile.writelines("EFI_FVB2_ALIGNMENT_"     + \
                                       self.FvAlignment.strip() + \
                                       " = TRUE"                + \
                                       T_CHAR_LF)
                                       
        #
        # Generate FV extension header file
        #
        if self.FvNameGuid == None or self.FvNameGuid == '':
            if len(self.FvExtEntryType) > 0:
                GenFdsGlobalVariable.ErrorLogger("FV Extension Header Entries declared for %s with no FvNameGuid declaration." % (self.UiFvName))
        
        if self.FvNameGuid <> None and self.FvNameGuid <> '':
            TotalSize = 16 + 4
            Buffer = ''
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
                    FvExtFile = open (FileFullPath,'rb')
                    FvExtFile.seek(0,2)
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
            Buffer = pack('=LHHBBBBBBBBL', 
                        int(Guid[0], 16), 
                        int(Guid[1], 16), 
                        int(Guid[2], 16), 
                        int(Guid[3][-4:-2], 16), 
                        int(Guid[3][-2:], 16),  
                        int(Guid[4][-12:-10], 16),
                        int(Guid[4][-10:-8], 16),
                        int(Guid[4][-8:-6], 16),
                        int(Guid[4][-6:-4], 16),
                        int(Guid[4][-4:-2], 16),
                        int(Guid[4][-2:], 16),
                        TotalSize
                        ) + Buffer

            #
            # Generate FV extension header file if the total size is not zero
            #
            if TotalSize > 0:
                FvExtHeaderFileName = os.path.join(GenFdsGlobalVariable.FvDir, self.UiFvName + '.ext')
                FvExtHeaderFile = StringIO.StringIO()
                FvExtHeaderFile.write(Buffer)
                Changed = SaveFileOnChange(FvExtHeaderFileName, FvExtHeaderFile.getvalue(), True)
                FvExtHeaderFile.close()
                if Changed:
                  if os.path.exists (self.InfFileName):
                    os.remove (self.InfFileName)
                self.FvInfFile.writelines("EFI_FV_EXT_HEADER_FILE_NAME = "      + \
                                           FvExtHeaderFileName                  + \
                                           T_CHAR_LF)

         
        #
        # Add [Files]
        #
        self.FvInfFile.writelines("[files]" + T_CHAR_LF)
        if VtfDict != None and self.UiFvName in VtfDict.keys():
            self.FvInfFile.writelines("EFI_FILE_NAME = "                   + \
                                       VtfDict.get(self.UiFvName)          + \
                                       T_CHAR_LF)
