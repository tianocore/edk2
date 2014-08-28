## @file
# process rule section generation
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
from struct import *
import Section
from GenFdsGlobalVariable import GenFdsGlobalVariable
import subprocess
from Ffs import Ffs
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import EfiSectionClassObject
from Common import EdkLogger
from Common.BuildToolError import *
from Common.Misc import PeImageClass
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.LongFilePathSupport import CopyLongFilePath

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
    def GenSection(self, OutputPath, ModuleName, SecNum, KeyStringList, FfsInf = None, Dict = {}) :
        
        if self.FileName != None and self.FileName.startswith('PCD('):
            self.FileName = GenFdsGlobalVariable.GetPcdValue(self.FileName)
        """Prepare the parameter of GenSection"""
        if FfsInf != None :
            InfFileName = FfsInf.InfFileName
            SectionType = FfsInf.__ExtendMacro__(self.SectionType)
            Filename = FfsInf.__ExtendMacro__(self.FileName)
            BuildNum = FfsInf.__ExtendMacro__(self.BuildNum)
            StringData = FfsInf.__ExtendMacro__(self.StringData)
            NoStrip = True
            if FfsInf.ModuleType in ('SEC', 'PEI_CORE', 'PEIM') and SectionType in ('TE', 'PE32'):
                if FfsInf.KeepReloc != None:
                    NoStrip = FfsInf.KeepReloc
                elif FfsInf.KeepRelocFromRule != None:
                    NoStrip = FfsInf.KeepRelocFromRule
                elif self.KeepReloc != None:
                    NoStrip = self.KeepReloc
                elif FfsInf.ShadowFromInfFile != None:
                    NoStrip = FfsInf.ShadowFromInfFile
        else:
            EdkLogger.error("GenFds", GENFDS_ERROR, "Module %s apply rule for None!" %ModuleName)

        """If the file name was pointed out, add it in FileList"""
        FileList = []
        if Filename != None:
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
        else:
            FileList, IsSect = Section.Section.GetFileList(FfsInf, self.FileType, self.FileExtension, Dict)
            if IsSect :
                return FileList, self.Alignment

        Index = 0

        """ If Section type is 'VERSION'"""
        OutputFileList = []
        if SectionType == 'VERSION':

            InfOverrideVerString = False
            if FfsInf.Version != None:
                #StringData = FfsInf.Version
                BuildNum = FfsInf.Version
                InfOverrideVerString = True

            if InfOverrideVerString:
                #VerTuple = ('-n', '"' + StringData + '"')
                if BuildNum != None and BuildNum != '':
                    BuildNumTuple = ('-j', BuildNum)
                else:
                    BuildNumTuple = tuple()

                Num = SecNum
                OutputFile = os.path.join( OutputPath, ModuleName + 'SEC' + str(Num) + Ffs.SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_VERSION',
                                                     #Ui=StringData, 
                                                     Ver=BuildNum)
                OutputFileList.append(OutputFile)

            elif FileList != []:
                for File in FileList:
                    Index = Index + 1
                    Num = '%s.%d' %(SecNum , Index)
                    OutputFile = os.path.join(OutputPath, ModuleName + 'SEC' + Num + Ffs.SectionSuffix.get(SectionType))
                    f = open(File, 'r')
                    VerString = f.read()
                    f.close()
                    BuildNum = VerString
                    if BuildNum != None and BuildNum != '':
                        BuildNumTuple = ('-j', BuildNum)
                    GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_VERSION',
                                                         #Ui=VerString, 
                                                         Ver=BuildNum)
                    OutputFileList.append(OutputFile)

            else:
                BuildNum = StringData
                if BuildNum != None and BuildNum != '':
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
                OutputFile = os.path.join( OutputPath, ModuleName + 'SEC' + str(Num) + Ffs.SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_VERSION',
                                                     #Ui=VerString, 
                                                     Ver=BuildNum)
                OutputFileList.append(OutputFile)

        #
        # If Section Type is 'UI'
        #
        elif SectionType == 'UI':

            InfOverrideUiString = False
            if FfsInf.Ui != None:
                StringData = FfsInf.Ui
                InfOverrideUiString = True

            if InfOverrideUiString:
                Num = SecNum
                OutputFile = os.path.join( OutputPath, ModuleName + 'SEC' + str(Num) + Ffs.SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_USER_INTERFACE',
                                                     Ui=StringData)
                OutputFileList.append(OutputFile)

            elif FileList != []:
                for File in FileList:
                    Index = Index + 1
                    Num = '%s.%d' %(SecNum , Index)
                    OutputFile = os.path.join(OutputPath, ModuleName + 'SEC' + Num + Ffs.SectionSuffix.get(SectionType))
                    f = open(File, 'r')
                    UiString = f.read()
                    f.close()
                    GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_USER_INTERFACE',
                                                         Ui=UiString)
                    OutputFileList.append(OutputFile)
            else:
                if StringData != None and len(StringData) > 0:
                    UiTuple = ('-n', '"' + StringData + '"')
                else:
                    UiTuple = tuple()

                    if self.Optional == True :
                        GenFdsGlobalVariable.VerboseLogger( "Optional Section don't exist!")
                        return '', None
                    else:
                        EdkLogger.error("GenFds", GENFDS_ERROR, "File: %s miss UI Section value" %InfFileName)

                Num = SecNum
                OutputFile = os.path.join( OutputPath, ModuleName + 'SEC' + str(Num) + Ffs.SectionSuffix.get(SectionType))
                GenFdsGlobalVariable.GenerateSection(OutputFile, [], 'EFI_SECTION_USER_INTERFACE',
                                                     Ui=StringData)
                OutputFileList.append(OutputFile)


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
                    Num = '%s.%d' %(SecNum , Index)
                    OutputFile = os.path.join( OutputPath, ModuleName + 'SEC' + Num + Ffs.SectionSuffix.get(SectionType))
                    File = GenFdsGlobalVariable.MacroExtend(File, Dict)
                    
                    #Get PE Section alignment when align is set to AUTO
                    if self.Alignment == 'Auto' and (SectionType == 'PE32' or SectionType == 'TE'):
                        ImageObj = PeImageClass (File)
                        if ImageObj.SectionAlignment < 0x400:
                            self.Alignment = str (ImageObj.SectionAlignment)
                        else:
                            self.Alignment = str (ImageObj.SectionAlignment / 0x400) + 'K'

                    if File[(len(File)-4):] == '.efi':
                        MapFile = File.replace('.efi', '.map')
                        if os.path.exists(MapFile):
                            CopyMapFile = os.path.join(OutputPath, ModuleName + '.map')
                            if not os.path.exists(CopyMapFile) or \
                                   (os.path.getmtime(MapFile) > os.path.getmtime(CopyMapFile)):
                                CopyLongFilePath(MapFile, CopyMapFile)

                    if not NoStrip:
                        FileBeforeStrip = os.path.join(OutputPath, ModuleName + '.efi')
                        if not os.path.exists(FileBeforeStrip) or \
                            (os.path.getmtime(File) > os.path.getmtime(FileBeforeStrip)):
                            CopyLongFilePath(File, FileBeforeStrip)
                        StrippedFile = os.path.join(OutputPath, ModuleName + '.stripped')
                        GenFdsGlobalVariable.GenerateFirmwareImage(
                                                StrippedFile,
                                                [File],
                                                Strip=True
                                                )
                        File = StrippedFile
                    
                    """For TE Section call GenFw to generate TE image"""

                    if SectionType == 'TE':
                        TeFile = os.path.join( OutputPath, ModuleName + 'Te.raw')
                        GenFdsGlobalVariable.GenerateFirmwareImage(
                                                TeFile,
                                                [File],
                                                Type='te'
                                                )
                        File = TeFile

                    """Call GenSection"""
                    GenFdsGlobalVariable.GenerateSection(OutputFile,
                                                         [File],
                                                         Section.Section.SectionType.get (SectionType)
                                                         )
                    OutputFileList.append(OutputFile)

        return OutputFileList, self.Alignment
