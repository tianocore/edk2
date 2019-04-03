## @file
# process FV image section generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Section
from io import BytesIO
from .Ffs import SectionSuffix
import subprocess
from .GenFdsGlobalVariable import GenFdsGlobalVariable
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import FvImageSectionClassObject
from Common import EdkLogger
from Common.BuildToolError import *
from Common.DataType import *

## generate FV image section
#
#
class FvImageSection(FvImageSectionClassObject):

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FvImageSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate FV image section
    #
    #   @param  self        The object pointer
    #   @param  OutputPath  Where to place output file
    #   @param  ModuleName  Which module this section belongs to
    #   @param  SecNum      Index of section
    #   @param  KeyStringList  Filter for inputs of section generation
    #   @param  FfsInf      FfsInfStatement object that contains this section data
    #   @param  Dict        dictionary contains macro and its value
    #   @retval tuple       (Generated file name, section alignment)
    #
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf = None, Dict = {}, IsMakefile = False):

        OutputFileList = []
        if self.FvFileType is not None:
            FileList, IsSect = Section.Section.GetFileList(FfsInf, self.FvFileType, self.FvFileExtension)
            if IsSect :
                return FileList, self.Alignment

            Num = SecNum

            MaxFvAlignment = 0
            for FvFileName in FileList:
                FvAlignmentValue = 0
                if os.path.isfile(FvFileName):
                    FvFileObj = open (FvFileName, 'rb')
                    FvFileObj.seek(0)
                    # PI FvHeader is 0x48 byte
                    FvHeaderBuffer = FvFileObj.read(0x48)
                    # FV alignment position.
                    if isinstance(FvHeaderBuffer[0x2E], str):
                        FvAlignmentValue = 1 << (ord(FvHeaderBuffer[0x2E]) & 0x1F)
                    else:
                        FvAlignmentValue = 1 << (FvHeaderBuffer[0x2E] & 0x1F)
                    FvFileObj.close()
                if FvAlignmentValue > MaxFvAlignment:
                    MaxFvAlignment = FvAlignmentValue

                OutputFile = os.path.join(OutputPath, ModuleName + SUP_MODULE_SEC + Num + SectionSuffix.get("FV_IMAGE"))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [FvFileName], 'EFI_SECTION_FIRMWARE_VOLUME_IMAGE', IsMakefile=IsMakefile)
                OutputFileList.append(OutputFile)

            # MaxFvAlignment is larger than or equal to 1K
            if MaxFvAlignment >= 0x400:
                if MaxFvAlignment >= 0x100000:
                    #The max alignment supported by FFS is 16M.
                    if MaxFvAlignment >= 0x1000000:
                        self.Alignment = "16M"
                    else:
                        self.Alignment = str(MaxFvAlignment // 0x100000) + "M"
                else:
                    self.Alignment = str (MaxFvAlignment // 0x400) + "K"
            else:
                # MaxFvAlignment is less than 1K
                self.Alignment = str (MaxFvAlignment)

            return OutputFileList, self.Alignment
        #
        # Generate Fv
        #
        if self.FvName is not None:
            Buffer = BytesIO()
            Fv = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(self.FvName)
            if Fv is not None:
                self.Fv = Fv
                if not self.FvAddr and self.Fv.BaseAddress:
                    self.FvAddr = self.Fv.BaseAddress
                FvFileName = Fv.AddToBuffer(Buffer, self.FvAddr, MacroDict = Dict, Flag=IsMakefile)
                if Fv.FvAlignment is not None:
                    if self.Alignment is None:
                        self.Alignment = Fv.FvAlignment
                    else:
                        if GenFdsGlobalVariable.GetAlignment (Fv.FvAlignment) > GenFdsGlobalVariable.GetAlignment (self.Alignment):
                            self.Alignment = Fv.FvAlignment
            else:
                if self.FvFileName is not None:
                    FvFileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FvFileName)
                    if os.path.isfile(FvFileName):
                        FvFileObj = open (FvFileName, 'rb')
                        FvFileObj.seek(0)
                        # PI FvHeader is 0x48 byte
                        FvHeaderBuffer = FvFileObj.read(0x48)
                        # FV alignment position.
                        if isinstance(FvHeaderBuffer[0x2E], str):
                            FvAlignmentValue = 1 << (ord(FvHeaderBuffer[0x2E]) & 0x1F)
                        else:
                            FvAlignmentValue = 1 << (FvHeaderBuffer[0x2E] & 0x1F)
                        # FvAlignmentValue is larger than or equal to 1K
                        if FvAlignmentValue >= 0x400:
                            if FvAlignmentValue >= 0x100000:
                                #The max alignment supported by FFS is 16M.
                                if FvAlignmentValue >= 0x1000000:
                                    self.Alignment = "16M"
                                else:
                                    self.Alignment = str(FvAlignmentValue // 0x100000) + "M"
                            else:
                                self.Alignment = str (FvAlignmentValue // 0x400) + "K"
                        else:
                            # FvAlignmentValue is less than 1K
                            self.Alignment = str (FvAlignmentValue)
                        FvFileObj.close()
                    else:
                        if len (mws.getPkgPath()) == 0:
                            EdkLogger.error("GenFds", FILE_NOT_FOUND, "%s is not found in WORKSPACE: %s" % self.FvFileName, GenFdsGlobalVariable.WorkSpaceDir)
                        else:
                            EdkLogger.error("GenFds", FILE_NOT_FOUND, "%s is not found in packages path:\n\t%s" % (self.FvFileName, '\n\t'.join(mws.getPkgPath())))

                else:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "FvImageSection Failed! %s NOT found in FDF" % self.FvName)

            #
            # Prepare the parameter of GenSection
            #
            OutputFile = os.path.join(OutputPath, ModuleName + SUP_MODULE_SEC + SecNum + SectionSuffix.get("FV_IMAGE"))
            GenFdsGlobalVariable.GenerateSection(OutputFile, [FvFileName], 'EFI_SECTION_FIRMWARE_VOLUME_IMAGE', IsMakefile=IsMakefile)
            OutputFileList.append(OutputFile)

            return OutputFileList, self.Alignment
