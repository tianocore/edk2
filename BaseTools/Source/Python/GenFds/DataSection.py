## @file
# process data section generation
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
from GenFdsGlobalVariable import GenFdsGlobalVariable
import subprocess
from Ffs import Ffs
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import DataSectionClassObject
from Common.Misc import PeImageClass
from Common.LongFilePathSupport import CopyLongFilePath

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
    def GenSection(self, OutputPath, ModuleName, SecNum, keyStringList, FfsFile = None, Dict = {}):
        #
        # Prepare the parameter of GenSection
        #
        if FfsFile != None:
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
            if os.path.exists(MapFile):
                CopyMapFile = os.path.join(OutputPath, ModuleName + '.map')
                if not os.path.exists(CopyMapFile) or (os.path.getmtime(MapFile) > os.path.getmtime(CopyMapFile)):
                    CopyLongFilePath(MapFile, CopyMapFile)

        #Get PE Section alignment when align is set to AUTO
        if self.Alignment == 'Auto' and self.SecType in ('TE', 'PE32'):
            ImageObj = PeImageClass (Filename)
            if ImageObj.SectionAlignment < 0x400:
                self.Alignment = str (ImageObj.SectionAlignment)
            else:
                self.Alignment = str (ImageObj.SectionAlignment / 0x400) + 'K'

        NoStrip = True
        if self.SecType in ('TE', 'PE32'):
            if self.KeepReloc != None:
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
                                    Strip=True
                                    )
            self.SectFileName = StrippedFile

        if self.SecType == 'TE':
            TeFile = os.path.join( OutputPath, ModuleName + 'Te.raw')
            GenFdsGlobalVariable.GenerateFirmwareImage(
                                    TeFile,
                                    [GenFdsGlobalVariable.MacroExtend(self.SectFileName, Dict)],
                                    Type='te'
                                    )
            self.SectFileName = TeFile

        OutputFile = os.path.join (OutputPath, ModuleName + 'SEC' + SecNum + Ffs.SectionSuffix.get(self.SecType))
        OutputFile = os.path.normpath(OutputFile)

        GenFdsGlobalVariable.GenerateSection(OutputFile, [self.SectFileName], Section.Section.SectionType.get(self.SecType))
        FileList = [OutputFile]
        return FileList, self.Alignment
