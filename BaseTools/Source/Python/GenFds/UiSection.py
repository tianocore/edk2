## @file
# process UI section generation
#
#  Copyright (c) 2007 - 2017, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Section
from .Ffs import SectionSuffix
import subprocess
import Common.LongFilePathOs as os
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from CommonDataClass.FdfClass import UiSectionClassObject
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.DataType import *

## generate UI section
#
#
class UiSection (UiSectionClassObject):

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        UiSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate UI section
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
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf=None, Dict={}, IsMakefile = False):
        #
        # Prepare the parameter of GenSection
        #
        if FfsInf is not None:
            self.Alignment = FfsInf.__ExtendMacro__(self.Alignment)
            self.StringData = FfsInf.__ExtendMacro__(self.StringData)
            self.FileName = FfsInf.__ExtendMacro__(self.FileName)

        OutputFile = os.path.join(OutputPath, ModuleName + SUP_MODULE_SEC + SecNum + SectionSuffix.get(BINARY_FILE_TYPE_UI))

        if self.StringData is not None :
            NameString = self.StringData
        elif self.FileName is not None:
            FileNameStr = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.FileName)
            FileNameStr = GenFdsGlobalVariable.MacroExtend(FileNameStr, Dict)
            FileObj = open(FileNameStr, 'r')
            NameString = FileObj.read()
            FileObj.close()
        else:
            NameString = ''
        GenFdsGlobalVariable.GenerateSection(OutputFile, None, 'EFI_SECTION_USER_INTERFACE', Ui=NameString, IsMakefile=IsMakefile)

        OutputFileList = []
        OutputFileList.append(OutputFile)
        return OutputFileList, self.Alignment
