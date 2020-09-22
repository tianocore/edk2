## @file
# process rule section generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from struct import *
from . import Section
from .GenFdsGlobalVariable import GenFdsGlobalVariable
import subprocess
from .Ffs import SectionSuffix
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import EfiSectionClassObject
from Common import EdkLogger
from Common.BuildToolError import *
from Common.Misc import PeImageClass
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.LongFilePathSupport import CopyLongFilePath
from Common.DataType import *

## generate rule section
#
#
class EfiSection (EfiSectionClassObject):

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
          EfiSectionClassObject.__init__(self)

    ## GenSection() method
    #
    #   Generate rule section
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
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf = None, Dict = None, IsMakefile = False) :

        if self.FileName is not None and self.FileName.startswith('PCD('):
            self.FileName = GenFdsGlobalVariable.GetPcdValue(self.FileName)
        """Prepare the parameter of GenSection"""
        if FfsInf is not None :
            InfFileName = FfsInf.InfFileName
            SectionType = FfsInf.__ExtendMacro__(self.SectionType)
            Filename = FfsInf.__ExtendMacro__(self.FileName)
            BuildNum = FfsInf.__ExtendMacro__(self.BuildNum)
            StringData = FfsInf.__ExtendMacro__(self.StringData)
            ModuleNameStr = FfsInf.__ExtendMacro__('$(MODULE_NAME)')
            NoStrip = True
            if FfsInf.ModuleType in (SUP_MODULE_SEC, SUP_MODULE_PEI_CORE, SUP_MODULE_PEIM, SUP_MODULE_MM_CORE_STANDALONE) and SectionType in (BINARY_FILE_TYPE_TE, BINARY_FILE_TYPE_PE32):
                if FfsInf.KeepReloc is not None:
                    NoStrip = FfsInf.KeepReloc
                elif FfsInf.KeepRelocFromRule is not None:
                    NoStrip = FfsInf.KeepRelocFromRule
                elif self.KeepReloc is not None:
                    NoStrip = self.KeepReloc
                elif FfsInf.ShadowFromInfFile is not None:
                    NoStrip = FfsInf.ShadowFromInfFile
        else:
            EdkLogger.error("GenFds", GENFDS_ERROR, "Module %s apply rule for None!" %ModuleName)

        """If the file name was pointed out, add it in FileList"""
        FileList = []
        if Dict is None:
            Dict = {}
        if Filename is not None:
            Filename = GenFdsGlobalVariable.MacroExtend(Filename, Dict)
            # check if the path is absolute or relative
            if os.path.isabs(Filename):
                Filename = os.path.normpath(Filename)
            else:
                Filename = os.path.normpath(os.path.join(FfsInf.EfiOutputPath, Filename))

            if not self.Optional:
                FileList.append(Filename)
            elif os.path.exists(Filename):
                FileList.append(Filename)
            elif IsMakefile:
                SuffixMap = FfsInf.GetFinalTargetSuffixMap()
                if '.depex' in SuffixMap:
                    FileList.append(Filename)
        else:
            FileList, IsSect = Section.Section.GetFileList(FfsInf, self.FileType, self.FileExtension, Dict, IsMakefile=IsMakefile, SectionType=SectionType)
            if IsSect :
                return FileList, self.Alignment

        Index = 0
        Align = self.Alignment

        """ If Section type is 'VERSION'"""
        OutputFileList = []
        if SectionType == 'VERSION':

            InfOverrideVerString = False
            if FfsInf.Version is not None:
                #StringData = FfsInf.Version
                BuildNum = FfsInf.Version
                InfOverrideVerString = True

            if InfOverrideVerString:
                #VerTuple = ('-n', '"' + StringData + '"')
                if BuildNum is not None and BuildNum != '':
                    BuildNumTuple = ('-j', BuildNum)
                else:
                    BuildNumTuple = tuple()

                Num = SecNum
                OutputFile = os.path.join( OutputPath, ModuleName + SUP_MODULE_SEC + str(Num) + SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_VERSION',
                                                    #Ui=StringData,
                                                    Ver=BuildNum,
                                                    IsMakefile=IsMakefile)
                OutputFileList.append(OutputFile)

            elif FileList != []:
                for File in FileList:
                    Index = Index + 1
                    Num = '%s.%d' %(SecNum, Index)
                    OutputFile = os.path.join(OutputPath, ModuleName + SUP_MODULE_SEC + Num + SectionSuffix.get(SectionType))
                    f = open(File, 'r')
                    VerString = f.read()
                    f.close()
                    BuildNum = VerString
                    if BuildNum is not None and BuildNum != '':
                        BuildNumTuple = ('-j', BuildNum)
                    GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_VERSION',
                                                        #Ui=VerString,
                                                        Ver=BuildNum,
                                                        IsMakefile=IsMakefile)
                    OutputFileList.append(OutputFile)

            else:
                BuildNum = StringData
                if BuildNum is not None and BuildNum != '':
                    BuildNumTuple = ('-j', BuildNum)
                else:
                    BuildNumTuple = tuple()
                BuildNumString = ' ' + ' '.join(BuildNumTuple)

                #if VerString == '' and
                if BuildNumString == '':
                    if self.Optional == True :
                        GenFdsGlobalVariable.VerboseLogger( "Optional Section don't exist!")
                        return [], None
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "File: %s miss Version Section value" %InfFileName)
                Num = SecNum
                OutputFile = os.path.join( OutputPath, ModuleName + SUP_MODULE_SEC + str(Num) + SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_VERSION',
                                                    #Ui=VerString,
                                                    Ver=BuildNum,
                                                    IsMakefile=IsMakefile)
                OutputFileList.append(OutputFile)

        #
        # If Section Type is BINARY_FILE_TYPE_UI
        #
        elif SectionType == BINARY_FILE_TYPE_UI:

            InfOverrideUiString = False
            if FfsInf.Ui is not None:
                StringData = FfsInf.Ui
                InfOverrideUiString = True

            if InfOverrideUiString:
                Num = SecNum
                if IsMakefile and StringData == ModuleNameStr:
                    StringData = "$(MODULE_NAME)"
                OutputFile = os.path.join( OutputPath, ModuleName + SUP_MODULE_SEC + str(Num) + SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_USER_INTERFACE',
                                                     Ui=StringData, IsMakefile=IsMakefile)
                OutputFileList.append(OutputFile)

            elif FileList != []:
                for File in FileList:
                    Index = Index + 1
                    Num = '%s.%d' %(SecNum, Index)
                    OutputFile = os.path.join(OutputPath, ModuleName + SUP_MODULE_SEC + Num + SectionSuffix.get(SectionType))
                    f = open(File, 'r')
                    UiString = f.read()
                    f.close()
                    if IsMakefile and UiString == ModuleNameStr:
                        UiString = "$(MODULE_NAME)"
                    GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_USER_INTERFACE',
                                                        Ui=UiString, IsMakefile=IsMakefile)
                    OutputFileList.append(OutputFile)
            else:
                if StringData is not None and len(StringData) > 0:
                    UiTuple = ('-n', '"' + StringData + '"')
                else:
                    UiTuple = tuple()

                    if self.Optional == True :
                        GenFdsGlobalVariable.VerboseLogger( "Optional Section don't exist!")
                        return '', None
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "File: %s miss UI Section value" %InfFileName)

                Num = SecNum
                if IsMakefile and StringData == ModuleNameStr:
                    StringData = "$(MODULE_NAME)"
                OutputFile = os.path.join( OutputPath, ModuleName + SUP_MODULE_SEC + str(Num) + SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_USER_INTERFACE',
                                                     Ui=StringData, IsMakefile=IsMakefile)
                OutputFileList.append(OutputFile)

        #
        # If Section Type is BINARY_FILE_TYPE_RAW
        #
        elif SectionType == BINARY_FILE_TYPE_RAW:
            """If File List is empty"""
            if FileList == []:
                if self.Optional == True:
                    GenFdsGlobalVariable.VerboseLogger("Optional Section don't exist!")
                    return [], None
                else:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "Output file for %s section could not be found for %s" % (SectionType, InfFileName))

            elif len(FileList) > 1:
                EdkLogger.error("GenFds", GENFDS_ERROR,
                                "Files suffixed with %s are not allowed to have more than one file in %s[Binaries] section" % (
                                self.FileExtension, InfFileName))
            else:
                for File in FileList:
                    File = GenFdsGlobalVariable.MacroExtend(File, Dict)
                    OutputFileList.append(File)

        else:
            """If File List is empty"""
            if FileList == [] :
                if self.Optional == True:
                    GenFdsGlobalVariable.VerboseLogger("Optional Section don't exist!")
                    return [], None
                else:
                    EdkLogger.error("GenFds", GENFDS_ERROR, "Output file for %s section could not be found for %s" % (SectionType, InfFileName))

            else:
                """Convert the File to Section file one by one """
                for File in FileList:
                    """ Copy Map file to FFS output path """
                    Index = Index + 1
                    Num = '%s.%d' %(SecNum, Index)
                    OutputFile = os.path.join( OutputPath, ModuleName + SUP_MODULE_SEC + Num + SectionSuffix.get(SectionType))
                    File = GenFdsGlobalVariable.MacroExtend(File, Dict)

                    #Get PE Section alignment when align is set to AUTO
                    if self.Alignment == 'Auto' and (SectionType == BINARY_FILE_TYPE_PE32 or SectionType == BINARY_FILE_TYPE_TE):
                        Align = "0"
                    if File[(len(File)-4):] == '.efi' and FfsInf.InfModule.BaseName == os.path.basename(File)[:-4]:
                        MapFile = File.replace('.efi', '.map')
                        CopyMapFile = os.path.join(OutputPath, ModuleName + '.map')
                        if IsMakefile:
                            if GenFdsGlobalVariable.CopyList == []:
                                GenFdsGlobalVariable.CopyList = [(MapFile, CopyMapFile)]
                            else:
                                GenFdsGlobalVariable.CopyList.append((MapFile, CopyMapFile))
                        else:
                            if os.path.exists(MapFile):
                                if not os.path.exists(CopyMapFile) or \
                                       (os.path.getmtime(MapFile) > os.path.getmtime(CopyMapFile)):
                                    CopyLongFilePath(MapFile, CopyMapFile)

                    if not NoStrip:
                        FileBeforeStrip = os.path.join(OutputPath, ModuleName + '.efi')
                        if IsMakefile:
                            if GenFdsGlobalVariable.CopyList == []:
                                GenFdsGlobalVariable.CopyList = [(File, FileBeforeStrip)]
                            else:
                                GenFdsGlobalVariable.CopyList.append((File, FileBeforeStrip))
                        else:
                            if not os.path.exists(FileBeforeStrip) or \
                                (os.path.getmtime(File) > os.path.getmtime(FileBeforeStrip)):
                                CopyLongFilePath(File, FileBeforeStrip)
                        StrippedFile = os.path.join(OutputPath, ModuleName + '.stripped')
                        GenFdsGlobalVariable.GenerateFirmwareImage(
                                StrippedFile,
                                [File],
                                Strip=True,
                                IsMakefile = IsMakefile
                            )
                        File = StrippedFile

                    """For TE Section call GenFw to generate TE image"""

                    if SectionType == BINARY_FILE_TYPE_TE:
                        TeFile = os.path.join( OutputPath, ModuleName + 'Te.raw')
                        GenFdsGlobalVariable.GenerateFirmwareImage(
                                TeFile,
                                [File],
                                Type='te',
                                IsMakefile = IsMakefile
                            )
                        File = TeFile

                    """Call GenSection"""
                    GenFdsGlobalVariable.GenerateSection(OutputFile,
                                                        [File],
                                                        Section.Section.SectionType.get (SectionType),
                                                        IsMakefile=IsMakefile
                                                        )
                    OutputFileList.append(OutputFile)

        return OutputFileList, Align
