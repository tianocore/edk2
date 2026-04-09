## @file
# process data section generation
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
from .GenFdsGlobalVariable import GenFdsGlobalVariable
import subprocess
from .Ffs import SectionSuffix
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import DataSectionClassObject
from Common.Misc import PeImageClass
from Common.LongFilePathSupport import CopyLongFilePath
from Common.DataType import *

## generate data section
#
#
class DataSection (DataSectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        DataSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate compressed section
    #
    #   @param  self        The object pointer
    #   @param  OutputPath  Where to place output file
    #   @param  ModuleName  Which module this section belongs to
    #   @param  SecNum      Index of section
    #   @param  KeyStringList  Filter for inputs of section generation
    #   @param  FfsInf      FfsInfStatement object that contains this section data
    #   @param  Dict        dictionary contains macro and its value
    #   @retval tuple       (Generated file name list, section alignment)
    #
    def GenSection(self, OutputPath, ModuleName, SecNum, keyStringList, FfsFile = None, Dict = None, IsMakefile = False):
        #
        # Prepare the parameter of GenSection
        #
        if Dict is None:
            Dict = {}
        if FfsFile is not None:
            self.SectFileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.SectFileName)
            self.SectFileName = GenFdsGlobalVariable.MacroExtend(self.SectFileName, Dict, FfsFile.CurrentArch)
        else:
            self.SectFileName = GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.SectFileName)
            self.SectFileName = GenFdsGlobalVariable.MacroExtend(self.SectFileName, Dict)

        """Check Section file exist or not !"""

        if not os.path.exists(self.SectFileName):
            self.SectFileName = os.path.join (GenFdsGlobalVariable.WorkSpaceDir,
                                              self.SectFileName)

        """Copy Map file to Ffs output"""
        Filename = GenFdsGlobalVariable.MacroExtend(self.SectFileName)
        if Filename[(len(Filename)-4):] == '.efi':
            MapFile = Filename.replace('.efi', '.map')
            CopyMapFile = os.path.join(OutputPath, ModuleName + '.map')
            if IsMakefile:
                if GenFdsGlobalVariable.CopyList == []:
                    GenFdsGlobalVariable.CopyList = [(MapFile, CopyMapFile)]
                else:
                    GenFdsGlobalVariable.CopyList.append((MapFile, CopyMapFile))
            else:
                if os.path.exists(MapFile):
                    if not os.path.exists(CopyMapFile) or (os.path.getmtime(MapFile) > os.path.getmtime(CopyMapFile)):
                        CopyLongFilePath(MapFile, CopyMapFile)

        #Get PE Section alignment when align is set to AUTO
        if self.Alignment == 'Auto' and self.SecType in (BINARY_FILE_TYPE_TE, BINARY_FILE_TYPE_PE32):
            self.Alignment = "0"
        NoStrip = True
        if self.SecType in (BINARY_FILE_TYPE_TE, BINARY_FILE_TYPE_PE32):
            if self.KeepReloc is not None:
                NoStrip = self.KeepReloc

        if not NoStrip:
            FileBeforeStrip = os.path.join(OutputPath, ModuleName + '.efi')
            if not os.path.exists(FileBeforeStrip) or \
                (os.path.getmtime(self.SectFileName) > os.path.getmtime(FileBeforeStrip)):
                CopyLongFilePath(self.SectFileName, FileBeforeStrip)
            StrippedFile = os.path.join(OutputPath, ModuleName + '.stripped')
            GenFdsGlobalVariable.GenerateFirmwareImage(
                    StrippedFile,
                    [GenFdsGlobalVariable.MacroExtend(self.SectFileName, Dict)],
                    Strip=True,
                    IsMakefile = IsMakefile
                )
            self.SectFileName = StrippedFile

        if self.SecType == BINARY_FILE_TYPE_TE:
            TeFile = os.path.join( OutputPath, ModuleName + 'Te.raw')
            GenFdsGlobalVariable.GenerateFirmwareImage(
                    TeFile,
                    [GenFdsGlobalVariable.MacroExtend(self.SectFileName, Dict)],
                    Type='te',
                    IsMakefile = IsMakefile
                )
            self.SectFileName = TeFile

        OutputFile = os.path.join (OutputPath, ModuleName + SUP_MODULE_SEC + SecNum + SectionSuffix.get(self.SecType))
        OutputFile = os.path.normpath(OutputFile)
        GenFdsGlobalVariable.GenerateSection(OutputFile, [self.SectFileName], Section.Section.SectionType.get(self.SecType), IsMakefile = IsMakefile)
        FileList = [OutputFile]
        return FileList, self.Alignment
