## @file
# process Subtype GUIDed section generation
#
#  Copyright (c) 2022, Konstantin Aladyshev <aladyshev22@gmail.com>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Section
import subprocess
from .Ffs import SectionSuffix
import Common.LongFilePathOs as os
from .GenFdsGlobalVariable import GenFdsGlobalVariable
from .GenFdsGlobalVariable import FindExtendTool
from CommonDataClass.FdfClass import SubTypeGuidSectionClassObject
import sys
from Common import EdkLogger
from Common.BuildToolError import *
from .FvImageSection import FvImageSection
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.DataType import *

## generate SubType GUIDed section
#
#
class SubTypeGuidSection(SubTypeGuidSectionClassObject) :

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        SubTypeGuidSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate GUIDed section
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
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf=None, Dict=None, IsMakefile=False):
        #
        # Generate all section
        #
        self.KeyStringList = KeyStringList
        self.CurrentArchList = GenFdsGlobalVariable.ArchList
        if FfsInf is not None:
            self.Alignment = FfsInf.__ExtendMacro__(self.Alignment)
            self.SubTypeGuid = FfsInf.__ExtendMacro__(self.SubTypeGuid)
            self.SectionType = FfsInf.__ExtendMacro__(self.SectionType)
            self.CurrentArchList = [FfsInf.CurrentArch]

        if Dict is None:
            Dict = {}

        self.SectFileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.SectFileName)
        self.SectFileName = GenFdsGlobalVariable.MacroExtend(self.SectFileName, Dict)

        OutputFile = os.path.join(OutputPath, ModuleName + SUP_MODULE_SEC + SecNum + SectionSuffix.get("SUBTYPE_GUID"))
        GenFdsGlobalVariable.GenerateSection(OutputFile, [self.SectFileName], 'EFI_SECTION_FREEFORM_SUBTYPE_GUID', Guid=self.SubTypeGuid, IsMakefile=IsMakefile)

        OutputFileList = []
        OutputFileList.append(OutputFile)
        return OutputFileList, self.Alignment

