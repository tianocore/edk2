## @file
# process FV image section generation
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
import Section
import StringIO
from Ffs import Ffs
import subprocess
from GenFdsGlobalVariable import GenFdsGlobalVariable
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import FvImageSectionClassObject
from Common import EdkLogger
from Common.BuildToolError import *

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
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf = None, Dict = {}):

        OutputFileList = []
        if self.FvFileType != None:
            FileList, IsSect = Section.Section.GetFileList(FfsInf, self.FvFileType, self.FvFileExtension)
            if IsSect :
                return FileList, self.Alignment

            Num = SecNum

            for FileName in FileList:
                OutputFile = os.path.join(OutputPath, ModuleName + 'SEC' + Num + Ffs.SectionSuffix.get("FV_IMAGE"))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [FvFileName], 'EFI_SECTION_FIRMWARE_VOLUME_IMAGE')
                OutputFileList.append(OutputFile)
            return OutputFileList, self.Alignment
        #
        # Generate Fv
        #
        if self.FvName != None:
            Buffer = StringIO.StringIO('')
            Fv = GenFdsGlobalVariable.FdfParser.Profile.FvDict.get(self.FvName)
            if Fv != None:
                self.Fv = Fv
                FvFileName = Fv.AddToBuffer(Buffer, self.FvAddr, MacroDict = Dict)
                if Fv.FvAlignment != None:
                    if self.Alignment == None:
                        self.Alignment = Fv.FvAlignment
                    else:
                        if GenFdsGlobalVariable.GetAlignment (Fv.FvAlignment) > GenFdsGlobalVariable.GetAlignment (self.Alignment):
                            self.Alignment = Fv.FvAlignment
            else:
                if self.FvFileName != None:
                    FvFileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FvFileName)
                else:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "FvImageSection Failed! %s NOT found in FDF" % self.FvName)

            #
            # Prepare the parameter of GenSection
            #
            OutputFile = os.path.join(OutputPath, ModuleName + 'SEC' + SecNum + Ffs.SectionSuffix.get("FV_IMAGE"))
            GenFdsGlobalVariable.GenerateSection(OutputFile, [FvFileName], 'EFI_SECTION_FIRMWARE_VOLUME_IMAGE')
            OutputFileList.append(OutputFile)

            return OutputFileList, self.Alignment
