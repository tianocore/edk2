## @file
# process OptionROM generation
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

from . import OptRomInfStatement
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import OptionRomClassObject
from Common.Misc import SaveFileOnChange
from Common import EdkLogger
from Common.BuildToolError import *

##
#
#
class OPTIONROM (OptionRomClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self, Name = ""):
        OptionRomClassObject.__init__(self)
        self.DriverName = Name

    ## AddToBuffer()
    #
    #   Generate Option ROM
    #
    #   @param  self        The object pointer
    #   @param  Buffer      The buffer generated OptROM data will be put
    #   @retval string      Generated OptROM file path
    #
    def AddToBuffer (self, Buffer, Flag=False) :
        if not Flag:
            GenFdsGlobalVariable.InfLogger( "\nGenerating %s Option ROM ..." %self.DriverName)

        EfiFileList = []
        BinFileList = []

        # Process Modules in FfsList
        for FfsFile in self.FfsList :

            if isinstance(FfsFile, OptRomInfStatement.OptRomInfStatement):
                FilePathNameList = FfsFile.GenFfs(IsMakefile=Flag)
                if len(FilePathNameList) == 0:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "Module %s not produce .efi files, so NO file could be put into option ROM." % (FfsFile.InfFileName))
                if FfsFile.OverrideAttribs is None:
                    EfiFileList.extend(FilePathNameList)
                else:
                    FileName = os.path.basename(FilePathNameList[0])
                    TmpOutputDir = os.path.join(GenFdsGlobalVariable.FvDir, self.DriverName, FfsFile.CurrentArch)
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
                                                           FfsFile.OverrideAttribs.PciVendorId,
                                                           IsMakefile = Flag)
                    BinFileList.append(TmpOutputFile)
            else:
                FilePathName = FfsFile.GenFfs(IsMakefile=Flag)
                if FfsFile.OverrideAttribs is not None:
                    FileName = os.path.basename(FilePathName)
                    TmpOutputDir = os.path.join(GenFdsGlobalVariable.FvDir, self.DriverName, FfsFile.CurrentArch)
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
                                                           FfsFile.OverrideAttribs.PciVendorId,
                                                           IsMakefile=Flag)
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
                                BinFileList,
                                IsMakefile=Flag)

        if not Flag:
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
