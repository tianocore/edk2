## @file
# process OptionROM generation
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
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

import OptRomInfStatement
from GenFdsGlobalVariable import GenFdsGlobalVariable
from GenFds import GenFds
from CommonDataClass.FdfClass import OptionRomClassObject
from Common.Misc import SaveFileOnChange
from Common import EdkLogger
from Common.BuildToolError import *

T_CHAR_LF = '\n'

## 
#
#
class OPTIONROM (OptionRomClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        OptionRomClassObject.__init__(self)


    ## AddToBuffer()
    #
    #   Generate Option ROM
    #
    #   @param  self        The object pointer
    #   @param  Buffer      The buffer generated OptROM data will be put
    #   @retval string      Generated OptROM file path
    #
    def AddToBuffer (self, Buffer) :

        GenFdsGlobalVariable.InfLogger( "\nGenerating %s Option ROM ..." %self.DriverName)

        EfiFileList = []
        BinFileList = []

        # Process Modules in FfsList
        for FfsFile in self.FfsList :
            
            if isinstance(FfsFile, OptRomInfStatement.OptRomInfStatement):
                FilePathNameList = FfsFile.GenFfs()
                if len(FilePathNameList) == 0:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "Module %s not produce .efi files, so NO file could be put into option ROM." % (FfsFile.InfFileName))
                if FfsFile.OverrideAttribs == None:
                    EfiFileList.extend(FilePathNameList)
                else:
                    FileName = os.path.basename(FilePathNameList[0])
                    TmpOutputDir = os.path.join(GenFdsGlobalVariable.FvDir, self.DriverName)
                    if not os.path.exists(TmpOutputDir) :
                        os.makedirs(TmpOutputDir)
                    TmpOutputFile = os.path.join(TmpOutputDir, FileName+'.tmp')
                    
                    GenFdsGlobalVariable.GenerateOptionRom(TmpOutputFile, 
                                                           FilePathNameList, 
                                                           [], 
                                                           FfsFile.OverrideAttribs.NeedCompress, 
                                                           FfsFile.OverrideAttribs.PciClassCode, 
                                                           FfsFile.OverrideAttribs.PciRevision, 
                                                           FfsFile.OverrideAttribs.PciDeviceId, 
                                                           FfsFile.OverrideAttribs.PciVendorId)
                    BinFileList.append(TmpOutputFile)
            else:
                FilePathName = FfsFile.GenFfs()
                if FfsFile.OverrideAttribs != None:
                    FileName = os.path.basename(FilePathName)
                    TmpOutputDir = os.path.join(GenFdsGlobalVariable.FvDir, self.DriverName)
                    if not os.path.exists(TmpOutputDir) :
                        os.makedirs(TmpOutputDir)
                    TmpOutputFile = os.path.join(TmpOutputDir, FileName+'.tmp')
                    
                    GenFdsGlobalVariable.GenerateOptionRom(TmpOutputFile, 
                                                           [FilePathName], 
                                                           [], 
                                                           FfsFile.OverrideAttribs.NeedCompress, 
                                                           FfsFile.OverrideAttribs.PciClassCode, 
                                                           FfsFile.OverrideAttribs.PciRevision, 
                                                           FfsFile.OverrideAttribs.PciDeviceId, 
                                                           FfsFile.OverrideAttribs.PciVendorId)
                    BinFileList.append(TmpOutputFile)
                else:
                    if FfsFile.FileType == 'EFI':
                        EfiFileList.append(FilePathName)
                    else:
                        BinFileList.append(FilePathName)
                        
        #
        # Call EfiRom tool
        #
        OutputFile = os.path.join(GenFdsGlobalVariable.FvDir, self.DriverName)
        OutputFile = OutputFile + '.rom'
        
        GenFdsGlobalVariable.GenerateOptionRom(
                                OutputFile,
                                EfiFileList,
                                BinFileList
                                )

        GenFdsGlobalVariable.InfLogger( "\nGenerate %s Option ROM Successfully" %self.DriverName)
        GenFdsGlobalVariable.SharpCounter = 0
        
        return OutputFile

class OverrideAttribs:
        
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        
        self.PciVendorId = None
        self.PciClassCode = None
        self.PciDeviceId = None
        self.PciRevision = None
        self.NeedCompress = None
        
        