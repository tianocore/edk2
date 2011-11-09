## @file
# This file is used to define each component of INF file
#
# Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os
import re
import EdkLogger

from CommonDataClass.ModuleClass import *
from CommonDataClass import CommonClass
from String import *
from DataType import *
from BuildToolError import *
from Misc import sdict
from Misc import GetFiles
from Parsing import *

# Global variable
Section = {TAB_UNKNOWN.upper() : MODEL_UNKNOWN,
           TAB_INF_DEFINES.upper() : MODEL_META_DATA_HEADER,
           TAB_BUILD_OPTIONS.upper() : MODEL_META_DATA_BUILD_OPTION,
           TAB_INCLUDES.upper() : MODEL_EFI_INCLUDE,
           TAB_LIBRARIES.upper() : MODEL_EFI_LIBRARY_INSTANCE,
           TAB_LIBRARY_CLASSES.upper() : MODEL_EFI_LIBRARY_CLASS,
           TAB_PACKAGES.upper() : MODEL_META_DATA_PACKAGE,
           TAB_NMAKE.upper() : MODEL_META_DATA_NMAKE,
           TAB_INF_FIXED_PCD.upper() : MODEL_PCD_FIXED_AT_BUILD,
           TAB_INF_PATCH_PCD.upper() : MODEL_PCD_PATCHABLE_IN_MODULE,
           TAB_INF_FEATURE_PCD.upper() : MODEL_PCD_FEATURE_FLAG,
           TAB_INF_PCD_EX.upper() : MODEL_PCD_DYNAMIC_EX,
           TAB_INF_PCD.upper() : MODEL_PCD_DYNAMIC,
           TAB_SOURCES.upper() : MODEL_EFI_SOURCE_FILE,
           TAB_GUIDS.upper() : MODEL_EFI_GUID,
           TAB_PROTOCOLS.upper() : MODEL_EFI_PROTOCOL,
           TAB_PPIS.upper() : MODEL_EFI_PPI,
           TAB_DEPEX.upper() : MODEL_EFI_DEPEX,
           TAB_BINARIES.upper() : MODEL_EFI_BINARY_FILE,
           TAB_USER_EXTENSIONS.upper() : MODEL_META_DATA_USER_EXTENSION
           }

gComponentType2ModuleType = {
    "LIBRARY"               :   "BASE",
    "SECURITY_CORE"         :   "SEC",
    "PEI_CORE"              :   "PEI_CORE",
    "COMBINED_PEIM_DRIVER"  :   "PEIM",
    "PIC_PEIM"              :   "PEIM",
    "RELOCATABLE_PEIM"      :   "PEIM",
    "PE32_PEIM"             :   "PEIM",
    "BS_DRIVER"             :   "DXE_DRIVER",
    "RT_DRIVER"             :   "DXE_RUNTIME_DRIVER",
    "SAL_RT_DRIVER"         :   "DXE_SAL_DRIVER",
    "APPLICATION"           :   "UEFI_APPLICATION",
    "LOGO"                  :   "BASE",
}

class InfHeader(ModuleHeaderClass):
    _Mapping_ = {
        # Required Fields
        TAB_INF_DEFINES_BASE_NAME                   : "Name",
        TAB_INF_DEFINES_FILE_GUID                   : "Guid",
        TAB_INF_DEFINES_MODULE_TYPE                 : "ModuleType",
        TAB_INF_DEFINES_EFI_SPECIFICATION_VERSION   : "UefiSpecificationVersion",
        TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION  : "UefiSpecificationVersion",
        TAB_INF_DEFINES_EDK_RELEASE_VERSION         : "EdkReleaseVersion",

        # Optional Fields
        TAB_INF_DEFINES_INF_VERSION                 : "InfVersion",
        TAB_INF_DEFINES_BINARY_MODULE               : "BinaryModule",
        TAB_INF_DEFINES_COMPONENT_TYPE              : "ComponentType",
        TAB_INF_DEFINES_MAKEFILE_NAME               : "MakefileName",
        TAB_INF_DEFINES_BUILD_NUMBER                : "BuildNumber",
        TAB_INF_DEFINES_BUILD_TYPE                  : "BuildType",
        TAB_INF_DEFINES_FFS_EXT                     : "FfsExt",
        TAB_INF_DEFINES_FV_EXT                      : "FvExt",
        TAB_INF_DEFINES_SOURCE_FV                   : "SourceFv",
        TAB_INF_DEFINES_VERSION_NUMBER              : "VersionNumber",
        TAB_INF_DEFINES_VERSION_STRING              : "VersionString",
        TAB_INF_DEFINES_VERSION                     : "Version",
        TAB_INF_DEFINES_PCD_IS_DRIVER               : "PcdIsDriver",
        TAB_INF_DEFINES_TIANO_EDK_FLASHMAP_H         : "TianoEdkFlashMap_h",
        TAB_INF_DEFINES_SHADOW                      : "Shadow",
    }

    def __init__(self):
        ModuleHeaderClass.__init__(self)
        self.VersionNumber = ''
        self.VersionString = ''
        #print self.__dict__
    def __setitem__(self, key, value):
        self.__dict__[self._Mapping_[key]] = value
    def __getitem__(self, key):
        return self.__dict__[self._Mapping_[key]]
    ## "in" test support
    def __contains__(self, key):
        return key in self._Mapping_

## InfObject
#
# This class defined basic Inf object which is used by inheriting
# 
# @param object:       Inherited from object class
#
class InfObject(object):
    def __init__(self):
        object.__init__()

## Inf
#
# This class defined the structure used in Inf object
# 
# @param InfObject:         Inherited from InfObject class
# @param Ffilename:         Input value for Ffilename of Inf file, default is None
# @param IsMergeAllArches:  Input value for IsMergeAllArches
#                           True is to merge all arches
#                           Fales is not to merge all arches
#                           default is False
# @param IsToModule:        Input value for IsToModule
#                           True is to transfer to ModuleObject automatically
#                           False is not to transfer to ModuleObject automatically
#                           default is False
# @param WorkspaceDir:      Input value for current workspace directory, default is None
#
# @var Identification:      To store value for Identification, it is a structure as Identification
# @var UserExtensions:      To store value for UserExtensions
# @var Module:              To store value for Module, it is a structure as ModuleClass
# @var WorkspaceDir:        To store value for WorkspaceDir
# @var KeyList:             To store value for KeyList, a list for all Keys used in Inf
#
class Inf(InfObject):
    def __init__(self, Filename=None, IsToModule=False, WorkspaceDir=None, PackageDir=None, SupArchList=DataType.ARCH_LIST):
        self.Identification = IdentificationClass()
        self.Module = ModuleClass()
        self.WorkspaceDir = WorkspaceDir
        self.PackageDir = PackageDir
        self.SupArchList = SupArchList

        self.KeyList = [
            TAB_SOURCES, TAB_BUILD_OPTIONS, TAB_BINARIES, TAB_INCLUDES, TAB_GUIDS,
            TAB_PROTOCOLS, TAB_PPIS, TAB_LIBRARY_CLASSES, TAB_PACKAGES, TAB_INF_FIXED_PCD,
            TAB_INF_PATCH_PCD, TAB_INF_FEATURE_PCD, TAB_INF_PCD, TAB_INF_PCD_EX,
            TAB_DEPEX, TAB_INF_DEFINES
        ]
        # Upper all KEYs to ignore case sensitive when parsing
        self.KeyList = map(lambda c: c.upper(), self.KeyList)

        # Init RecordSet
        self.RecordSet = {}
        for Key in self.KeyList:
            self.RecordSet[Section[Key]] = []

        # Init Comment
        self.SectionHeaderCommentDict = {}

        # Load Inf file if filename is not None
        if Filename != None:
            self.LoadInfFile(Filename)

        # Transfer to Module Object if IsToModule is True
        if IsToModule:
            self.InfToModule()

    ## Module Object to INF file
    def ModuleToInf(self, Module):
        Inf = ''
        InfList = sdict()
        SectionHeaderCommentDict = {}
        if Module == None:
            return Inf

        ModuleHeader = Module.ModuleHeader
        TmpList = []
        # Common define items
        if ModuleHeader.Name:
            TmpList.append(TAB_INF_DEFINES_BASE_NAME + ' = ' + ModuleHeader.Name)
        if ModuleHeader.Guid:
            TmpList.append(TAB_INF_DEFINES_FILE_GUID + ' = ' + ModuleHeader.Guid)
        if ModuleHeader.Version:
            TmpList.append(TAB_INF_DEFINES_VERSION_STRING + ' = ' + ModuleHeader.Version)
        if ModuleHeader.ModuleType:
            TmpList.append(TAB_INF_DEFINES_MODULE_TYPE + ' = ' + ModuleHeader.ModuleType)
        if ModuleHeader.PcdIsDriver:
            TmpList.append(TAB_INF_DEFINES_PCD_IS_DRIVER + ' = ' + ModuleHeader.PcdIsDriver)
        # Externs
        for Item in Module.Externs:
            if Item.EntryPoint:
                TmpList.append(TAB_INF_DEFINES_ENTRY_POINT + ' = ' + Item.EntryPoint)
            if Item.UnloadImage:
                TmpList.append(TAB_INF_DEFINES_UNLOAD_IMAGE + ' = ' + Item.UnloadImage)
            if Item.Constructor:
                TmpList.append(TAB_INF_DEFINES_CONSTRUCTOR + ' = ' + Item.Constructor)
            if Item.Destructor:
                TmpList.append(TAB_INF_DEFINES_DESTRUCTOR + ' = ' + Item.Destructor)
        # Other define items
        if Module.UserExtensions != None:
            for Item in Module.UserExtensions.Defines:
                TmpList.append(Item)
        InfList['Defines'] = TmpList
        if ModuleHeader.Description != '':
            SectionHeaderCommentDict['Defines'] = ModuleHeader.Description

        if Module.UserExtensions != None:
            InfList['BuildOptions'] = Module.UserExtensions.BuildOptions

        for Item in Module.Includes:
            Key = 'Includes.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            Value.append(Item.FilePath)
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.LibraryClasses:
            Key = 'LibraryClasses.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            NewValue = Item.LibraryClass
            if Item.RecommendedInstance:
                NewValue = NewValue + '|' + Item.RecommendedInstance
            if Item.FeatureFlag:
                NewValue = NewValue + '|' + Item.FeatureFlag
            Value.append(NewValue)
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.PackageDependencies:
            Key = 'Packages.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            Value.append(Item.FilePath)
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.PcdCodes:
            Key = 'Pcds' + Item.ItemType + '.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            NewValue = Item.TokenSpaceGuidCName + '.' + Item.CName
            if Item.DefaultValue != '':
                NewValue = NewValue + '|' + Item.DefaultValue
            Value.append(NewValue)
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.Sources:
            Key = 'Sources.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            NewValue = Item.SourceFile
            if Item.ToolChainFamily != '':
                NewValue = NewValue + '|' + Item.ToolChainFamily
            if Item.TagName != '':
                NewValue = NewValue + '|' + Item.TagName
            if Item.ToolCode != '':
                NewValue = NewValue + '|' + Item.ToolCode
            if Item.FeatureFlag != '':
                NewValue = NewValue + '|' + Item.FeatureFlag
            Value.append(NewValue)
            if Item.HelpText != '':
                SectionHeaderCommentDict[Key] = Item.HelpText
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.Guids:
            Key = 'Guids.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            Value.append(Item.CName)
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.Protocols:
            Key = 'Protocols.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            Value.append(Item.CName)
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.Ppis:
            Key = 'Ppis.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            Value.append(Item.CName)
            GenMetaDatSectionItem(Key, Value, InfList)

        if Module.PeiDepex:
            Key = 'Depex'
            Value = Module.PeiDepex.Depex
            GenMetaDatSectionItem(Key, Value, InfList)

        if Module.DxeDepex:
            Key = 'Depex'
            Value = Module.DxeDepex.Depex
            GenMetaDatSectionItem(Key, Value, InfList)

        if Module.SmmDepex:
            Key = 'Depex'
            Value = Module.SmmDepex.Depex
            GenMetaDatSectionItem(Key, Value, InfList)

        for Item in Module.Binaries:
            Key = 'Binaries.' + GetStringOfList(Item.SupArchList)
            Value = GetHelpTextList(Item.HelpTextList)
            NewValue = Item.FileType + '|' + Item.BinaryFile + '|' + Item.Target
            if Item.FeatureFlag != '':
                NewValue = NewValue + '|' + Item.FeatureFlag
            Value.append(NewValue)
            GenMetaDatSectionItem(Key, Value, InfList)

        # Transfer Module to Inf
        for Key in InfList:
            if Key in SectionHeaderCommentDict:
                List = SectionHeaderCommentDict[Key].split('\r')
                for Item in List:
                    Inf = Inf + Item + '\n'
            Inf = Inf + '[' + Key + ']' + '\n'
            for Value in InfList[Key]:
                if type(Value) == type([]):
                    for SubValue in Value:
                        Inf = Inf + '  ' + SubValue + '\n'
                else:
                    Inf = Inf + '  ' + Value + '\n'
            Inf = Inf + '\n'

        return Inf


    ## Transfer to Module Object
    # 
    # Transfer all contents of an Inf file to a standard Module Object
    #
    def InfToModule(self):
        # Init global information for the file
        ContainerFile = self.Identification.FullPath

        # Generate Module Header
        self.GenModuleHeader(ContainerFile)

        # Generate BuildOptions
        self.GenBuildOptions(ContainerFile)

        # Generate Includes
        self.GenIncludes(ContainerFile)

        # Generate LibraryClasses
        self.GenLibraryClasses(ContainerFile)

        # Generate Packages
        self.GenPackages(ContainerFile)

        # Generate Pcds
        self.GenPcds(ContainerFile)

        # Generate Sources
        self.GenSources(ContainerFile)

        # Generate Guids
        self.GenGuidProtocolPpis(DataType.TAB_GUIDS, ContainerFile)

        # Generate Protocols
        self.GenGuidProtocolPpis(DataType.TAB_PROTOCOLS, ContainerFile)

        # Generate Ppis
        self.GenGuidProtocolPpis(DataType.TAB_PPIS, ContainerFile)

        # Generate Depexes
        self.GenDepexes(ContainerFile)

        # Generate Binaries
        self.GenBinaries(ContainerFile)

        # Init MiscFiles
        self.GenMiscFiles(ContainerFile)

    ## GenMiscFiles
    #
    def GenMiscFiles(self, ContainerFile):
        MiscFiles = MiscFileClass()
        MiscFiles.Name = 'ModuleFiles'
        for Item in GetFiles(os.path.dirname(ContainerFile), ['CVS', '.svn'], False):
            File = CommonClass.FileClass()
            File.Filename = Item
            MiscFiles.Files.append(File)
        self.Module.MiscFiles = MiscFiles

    ## Load Inf file
    #
    # Load the file if it exists
    #
    # @param Filename:  Input value for filename of Inf file
    #
    def LoadInfFile(self, Filename):
        # Insert a record for file
        Filename = NormPath(Filename)

        self.Identification.FullPath = Filename
        (self.Identification.RelaPath, self.Identification.FileName) = os.path.split(Filename)
        if self.Identification.FullPath.find(self.WorkspaceDir) > -1:
            self.Identification.ModulePath = os.path.dirname(self.Identification.FullPath[len(self.WorkspaceDir) + 1:])
        if self.PackageDir:
            self.Identification.PackagePath = self.PackageDir
            if self.Identification.ModulePath.find(self.PackageDir) == 0:
                self.Identification.ModulePath = self.Identification.ModulePath[len(self.PackageDir) + 1:]

        # Init common datas
        IfDefList, SectionItemList, CurrentSection, ArchList, ThirdList, IncludeFiles = \
        [], [], TAB_UNKNOWN, [], [], []
        LineNo = 0

        # Parse file content
        IsFindBlockComment = False
        ReservedLine = ''
        Comment = ''
        for Line in open(Filename, 'r'):
            LineNo = LineNo + 1
            # Remove comment block
            if Line.find(TAB_COMMENT_EDK_START) > -1:
                ReservedLine = GetSplitList(Line, TAB_COMMENT_EDK_START, 1)[0]
                if ReservedLine.strip().startswith(TAB_COMMENT_SPLIT):
                    Comment = Comment + Line.strip() + '\n'
                    ReservedLine = ''
                else:
                    Comment = Comment + Line[len(ReservedLine):] + '\n'
                IsFindBlockComment = True
                if not ReservedLine:
                    continue
            if Line.find(TAB_COMMENT_EDK_END) > -1:
                Comment = Comment + Line[:Line.find(TAB_COMMENT_EDK_END) + len(TAB_COMMENT_EDK_END)] + '\n'
                Line = ReservedLine + GetSplitList(Line, TAB_COMMENT_EDK_END, 1)[1]
                ReservedLine = ''
                IsFindBlockComment = False
            if IsFindBlockComment:
                Comment = Comment + Line.strip() + '\n'
                continue

            # Remove comments at tail and remove spaces again
            if Line.strip().startswith(TAB_COMMENT_SPLIT) or Line.strip().startswith('--/'):
                Comment = Comment + Line.strip() + '\n'
            Line = CleanString(Line)
            if Line == '':
                continue

            ## Find a new section tab
            # First insert previous section items
            # And then parse the content of the new section
            if Line.startswith(TAB_SECTION_START) and Line.endswith(TAB_SECTION_END):
                if Line[1:3] == "--":
                    continue
                Model = Section[CurrentSection.upper()]
                # Insert items data of previous section
                InsertSectionItems(Model, CurrentSection, SectionItemList, ArchList, ThirdList, self.RecordSet)

                # Parse the new section
                SectionItemList = []
                ArchList = []
                ThirdList = []

                CurrentSection = ''
                LineList = GetSplitValueList(Line[len(TAB_SECTION_START):len(Line) - len(TAB_SECTION_END)], TAB_COMMA_SPLIT)
                for Item in LineList:
                    ItemList = GetSplitValueList(Item, TAB_SPLIT)
                    if CurrentSection == '':
                        CurrentSection = ItemList[0]
                    else:
                        if CurrentSection != ItemList[0]:
                            EdkLogger.error("Parser", PARSER_ERROR, "Different section names '%s' and '%s' are found in one section definition, this is not allowed." % (CurrentSection, ItemList[0]), File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)
                    if CurrentSection.upper() not in self.KeyList:
                        RaiseParserError(Line, CurrentSection, Filename, '', LineNo)
                    ItemList.append('')
                    ItemList.append('')
                    if len(ItemList) > 5:
                        RaiseParserError(Line, CurrentSection, Filename, '', LineNo)
                    else:
                        if ItemList[1] != '' and ItemList[1].upper() not in ARCH_LIST_FULL:
                            EdkLogger.error("Parser", PARSER_ERROR, "Invalid Arch definition '%s' found" % ItemList[1], File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)
                        ArchList.append(ItemList[1].upper())
                        ThirdList.append(ItemList[2])

                if Comment:
                    if Comment.endswith('\n'):
                        Comment = Comment[:len(Comment) - len('\n')]
                    self.SectionHeaderCommentDict[Section[CurrentSection.upper()]] = Comment
                    Comment = ''
                continue

            # Not in any defined section
            if CurrentSection == TAB_UNKNOWN:
                ErrorMsg = "%s is not in any defined section" % Line
                EdkLogger.error("Parser", PARSER_ERROR, ErrorMsg, File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)

            # Add a section item
            SectionItemList.append([Line, LineNo, Comment])
            Comment = ''
            # End of parse
        #End of For

        # Insert items data of last section
        Model = Section[CurrentSection.upper()]
        InsertSectionItems(Model, CurrentSection, SectionItemList, ArchList, ThirdList, self.RecordSet)
        if Comment != '':
            self.SectionHeaderCommentDict[Model] = Comment
            Comment = ''

    ## Show detailed information of Module
    #
    # Print all members and their values of Module class
    #
    def ShowModule(self):
        M = self.Module
        print 'Filename =', M.ModuleHeader.FileName
        print 'FullPath =', M.ModuleHeader.FullPath
        print 'RelaPath =', M.ModuleHeader.RelaPath
        print 'PackagePath =', M.ModuleHeader.PackagePath
        print 'ModulePath =', M.ModuleHeader.ModulePath
        print 'CombinePath =', M.ModuleHeader.CombinePath

        print 'BaseName =', M.ModuleHeader.Name
        print 'Guid =', M.ModuleHeader.Guid
        print 'Version =', M.ModuleHeader.Version

        print '\nIncludes ='
        for Item in M.Includes:
            print Item.FilePath, Item.SupArchList
        print '\nLibraryClasses ='
        for Item in M.LibraryClasses:
            print Item.LibraryClass, Item.RecommendedInstance, Item.RecommendedInstanceGuid, Item.RecommendedInstanceVersion, Item.FeatureFlag, Item.SupModuleList, Item.SupArchList, Item.Define
        print '\nPackageDependencies ='
        for Item in M.PackageDependencies:
            print Item.FilePath, Item.SupArchList, Item.FeatureFlag
        print '\nPcds ='
        for Item in M.PcdCodes:
            print '\tCName=', Item.CName, 'TokenSpaceGuidCName=', Item.TokenSpaceGuidCName, 'DefaultValue=', Item.DefaultValue, 'ItemType=', Item.ItemType, Item.SupArchList
        print '\nSources ='
        for Source in M.Sources:
            print Source.SourceFile, 'Fam=', Source.ToolChainFamily, 'Pcd=', Source.FeatureFlag, 'Tag=', Source.TagName, 'ToolCode=', Source.ToolCode, Source.SupArchList
        print '\nGuids ='
        for Item in M.Guids:
            print Item.CName, Item.SupArchList, Item.FeatureFlag
        print '\nProtocols ='
        for Item in M.Protocols:
            print Item.CName, Item.SupArchList, Item.FeatureFlag
        print '\nPpis ='
        for Item in M.Ppis:
            print Item.CName, Item.SupArchList, Item.FeatureFlag
        print '\nDepex ='
        for Item in M.Depex:
            print Item.Depex, Item.SupArchList, Item.Define
        print '\nBinaries ='
        for Binary in M.Binaries:
            print 'Type=', Binary.FileType, 'Target=', Binary.Target, 'Name=', Binary.BinaryFile, 'FeatureFlag=', Binary.FeatureFlag, 'SupArchList=', Binary.SupArchList
        print '\n*** FileList ***'
        for Item in M.MiscFiles.Files:
            print Item.Filename
        print '****************\n'

    ## Convert [Defines] section content to ModuleHeaderClass
    #
    # Convert [Defines] section content to ModuleHeaderClass
    #
    # @param Defines        The content under [Defines] section
    # @param ModuleHeader   An object of ModuleHeaderClass
    # @param Arch           The supported ARCH
    #
    def GenModuleHeader(self, ContainerFile):
        EdkLogger.debug(2, "Generate ModuleHeader ...")
        # Update all defines item in database
        RecordSet = self.RecordSet[MODEL_META_DATA_HEADER]

        ModuleHeader = ModuleHeaderClass()
        ModuleExtern = ModuleExternClass()
        OtherDefines = []
        for Record in RecordSet:
            ValueList = GetSplitValueList(Record[0], TAB_EQUAL_SPLIT)
            if len(ValueList) != 2:
                OtherDefines.append(Record[0])
            else:
                Name = ValueList[0]
                Value = ValueList[1]
                if Name == TAB_INF_DEFINES_BASE_NAME:
                    ModuleHeader.Name = Value
                    ModuleHeader.BaseName = Value
                elif Name == TAB_INF_DEFINES_FILE_GUID:
                    ModuleHeader.Guid = Value
                elif Name == TAB_INF_DEFINES_VERSION_STRING:
                    ModuleHeader.Version = Value
                elif Name == TAB_INF_DEFINES_PCD_IS_DRIVER:
                    ModuleHeader.PcdIsDriver = Value
                elif Name == TAB_INF_DEFINES_MODULE_TYPE:
                    ModuleHeader.ModuleType = Value
                elif Name in (TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION, TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION):
                    ModuleHeader.UefiSpecificationVersion = Value
                elif Name == TAB_INF_DEFINES_PI_SPECIFICATION_VERSION:
                    ModuleHeader.PiSpecificationVersion = Value
                elif Name == TAB_INF_DEFINES_ENTRY_POINT:
                    ModuleExtern.EntryPoint = Value
                elif Name == TAB_INF_DEFINES_UNLOAD_IMAGE:
                    ModuleExtern.UnloadImage = Value
                elif Name == TAB_INF_DEFINES_CONSTRUCTOR:
                    ModuleExtern.Constructor = Value
                elif Name == TAB_INF_DEFINES_DESTRUCTOR:
                    ModuleExtern.Destructor = Value
                else:
                    OtherDefines.append(Record[0])
        ModuleHeader.FileName = self.Identification.FileName
        ModuleHeader.FullPath = self.Identification.FullPath
        ModuleHeader.RelaPath = self.Identification.RelaPath
        ModuleHeader.PackagePath = self.Identification.PackagePath
        ModuleHeader.ModulePath = self.Identification.ModulePath
        ModuleHeader.CombinePath = os.path.normpath(os.path.join(ModuleHeader.PackagePath, ModuleHeader.ModulePath, ModuleHeader.FileName))

        if MODEL_META_DATA_HEADER in self.SectionHeaderCommentDict:
            ModuleHeader.Description = self.SectionHeaderCommentDict[MODEL_META_DATA_HEADER]
        self.Module.ModuleHeader = ModuleHeader
        self.Module.Externs.append(ModuleExtern)
        UE = self.Module.UserExtensions
        if UE == None:
            UE = UserExtensionsClass()
        UE.Defines = OtherDefines
        self.Module.UserExtensions = UE

    ## GenBuildOptions
    #
    # Gen BuildOptions of Inf
    # [<Family>:]<ToolFlag>=Flag
    #
    # @param ContainerFile: The Inf file full path 
    #
    def GenBuildOptions(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_BUILD_OPTIONS)
        BuildOptions = {}
        # Get all BuildOptions
        RecordSet = self.RecordSet[MODEL_META_DATA_BUILD_OPTION]
        UE = self.Module.UserExtensions
        if UE == None:
            UE = UserExtensionsClass()
        for Record in RecordSet:
            UE.BuildOptions.append(Record[0])
        self.Module.UserExtensions = UE

    ## GenIncludes
    #
    # Gen Includes of Inf
    # 
    # @param ContainerFile: The Inf file full path 
    #
    def GenIncludes(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_INCLUDES)
        Includes = sdict()
        # Get all Includes
        RecordSet = self.RecordSet[MODEL_EFI_INCLUDE]
        for Record in RecordSet:
            Include = IncludeClass()
            Include.FilePath = Record[0]
            Include.SupArchList = Record[1]
            if GenerateHelpText(Record[5], ''):
                Include.HelpTextList.append(GenerateHelpText(Record[5], ''))
            self.Module.Includes.append(Include)
            #self.Module.FileList.extend(GetFiles(os.path.normpath(os.path.join(self.Identification.FileRelativePath, Include.FilePath)), ['CVS', '.svn']))

    ## GenLibraryClasses
    #
    # Get LibraryClass of Inf
    # <LibraryClassKeyWord>|<LibraryInstance>
    #
    # @param ContainerFile: The Inf file full path 
    #
    def GenLibraryClasses(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_LIBRARY_CLASSES)
        LibraryClasses = {}
        # Get all LibraryClasses
        RecordSet = self.RecordSet[MODEL_EFI_LIBRARY_CLASS]
        for Record in RecordSet:
            (LibClassName, LibClassIns, Pcd, SupModelList) = GetLibraryClassOfInf([Record[0], Record[4]], ContainerFile, self.WorkspaceDir, Record[2])
            LibraryClass = CommonClass.LibraryClassClass()
            LibraryClass.LibraryClass = LibClassName
            LibraryClass.RecommendedInstance = LibClassIns
            LibraryClass.FeatureFlag = Pcd
            LibraryClass.SupArchList = Record[1]
            LibraryClass.SupModuleList = Record[4]
            if GenerateHelpText(Record[5], ''):
                LibraryClass.HelpTextList.append(GenerateHelpText(Record[5], ''))
            self.Module.LibraryClasses.append(LibraryClass)

    ## GenPackages
    #
    # Gen Packages of Inf
    # 
    # @param ContainerFile: The Inf file full path 
    #
    def GenPackages(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_PACKAGES)
        Packages = {}
        # Get all Packages
        RecordSet = self.RecordSet[MODEL_META_DATA_PACKAGE]
        for Record in RecordSet:
            (PackagePath, Pcd) = GetPackage(Record[0], ContainerFile, self.WorkspaceDir, Record[2])
            Package = ModulePackageDependencyClass()
            Package.FilePath = NormPath(PackagePath)
            Package.SupArchList = Record[1]
            Package.FeatureFlag = Pcd
            if GenerateHelpText(Record[5], ''):
                Package.HelpTextList.append(GenerateHelpText(Record[5], ''))
            self.Module.PackageDependencies.append(Package)

    def AddPcd(self, CName, TokenSpaceGuidCName, DefaultValue, ItemType, Arch, HelpTextList):
        Pcd = PcdClass()
        Pcd.CName = CName
        Pcd.TokenSpaceGuidCName = TokenSpaceGuidCName
        Pcd.DefaultValue = DefaultValue
        Pcd.ItemType = ItemType
        Pcd.SupArchList = Arch
        if GenerateHelpText(HelpTextList, ''):
            Pcd.HelpTextList.append(GenerateHelpText(HelpTextList, ''))
        self.Module.PcdCodes.append(Pcd)

    ## GenPcds
    #
    # Gen Pcds of Inf
    # <TokenSpaceGuidCName>.<PcdCName>[|<Value>]
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenPcds(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_PCDS)
        Pcds = {}
        PcdToken = {}

        # Get all Pcds
        RecordSet1 = self.RecordSet[MODEL_PCD_FIXED_AT_BUILD]
        RecordSet2 = self.RecordSet[MODEL_PCD_PATCHABLE_IN_MODULE]
        RecordSet3 = self.RecordSet[MODEL_PCD_FEATURE_FLAG]
        RecordSet4 = self.RecordSet[MODEL_PCD_DYNAMIC_EX]
        RecordSet5 = self.RecordSet[MODEL_PCD_DYNAMIC]

        # Go through each arch
        for Record in RecordSet1:
            (TokenSpaceGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_FIXED_AT_BUILD, ContainerFile, Record[2])
            self.AddPcd(TokenName, TokenSpaceGuidCName, Value, Type, Record[1], Record[5])
        for Record in RecordSet2:
            (TokenSpaceGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_PATCHABLE_IN_MODULE, ContainerFile, Record[2])
            self.AddPcd(TokenName, TokenSpaceGuidCName, Value, Type, Record[1], Record[5])
        for Record in RecordSet3:
            (TokenSpaceGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_FEATURE_FLAG, ContainerFile, Record[2])
            self.AddPcd(TokenName, TokenSpaceGuidCName, Value, Type, Record[1], Record[5])
        for Record in RecordSet4:
            (TokenSpaceGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_DYNAMIC_EX, ContainerFile, Record[2])
            self.AddPcd(TokenName, TokenSpaceGuidCName, Value, Type, Record[1], Record[5])
        for Record in RecordSet5:
            (TokenSpaceGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], '', ContainerFile, Record[2])
            self.AddPcd(TokenName, TokenSpaceGuidCName, Value, Type, Record[1], Record[5])

    ## GenSources
    #
    # Gen Sources of Inf
    # <Filename>[|<Family>[|<TagName>[|<ToolCode>[|<PcdFeatureFlag>]]]]
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenSources(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_SOURCES)
        Sources = {}

        # Get all Sources
        RecordSet = self.RecordSet[MODEL_EFI_SOURCE_FILE]
        for Record in RecordSet:
            (Filename, Family, TagName, ToolCode, Pcd) = GetSource(Record[0], ContainerFile, self.Identification.RelaPath, Record[2])
            Source = ModuleSourceFileClass(Filename, TagName, ToolCode, Family, Pcd, Record[1])
            if GenerateHelpText(Record[5], ''):
                Source.HelpTextList.append(GenerateHelpText(Record[5], ''))
            if MODEL_EFI_SOURCE_FILE in self.SectionHeaderCommentDict:
                Source.HelpText = self.SectionHeaderCommentDict[MODEL_EFI_SOURCE_FILE]
            self.Module.Sources.append(Source)
            #self.Module.FileList.append(os.path.normpath(os.path.join(self.Identification.RelaPath, Filename)))

    ## GenDepexes
    #
    # Gen Depex of Inf
    #
    # @param ContainerFile: The Inf file full path 
    #
    def GenDepexes(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_DEPEX)
        Depex = {}
        # Get all Depexes
        RecordSet = self.RecordSet[MODEL_EFI_DEPEX]
        DepexString = ''
        for Record in RecordSet:
            DepexString = DepexString + Record[0] + '\n'
        Dep = ModuleDepexClass()
        if DepexString.endswith('\n'):
            DepexString = DepexString[:len(DepexString) - len('\n')]
        Dep.Depex = DepexString
        if self.Module.ModuleHeader.ModuleType in ['DXE_SMM_DRIVER']:
            self.Module.SmmDepex = Dep
        elif self.Module.ModuleHeader.ModuleType in ['PEI_CORE', 'PEIM']:
            self.Module.PeiDepex = Dep
        else:
            self.Module.DxeDepex = Dep
#        for Record in RecordSet:
#            
#            Dep = ModuleDepexClass()
#            Dep.Depex = Record[0]
#            Dep.SupArchList = Record[1]
#            if GenerateHelpText(Record[5], ''):
#                Dep.HelpTextList.append(GenerateHelpText(Record[5], ''))
#            DepexString = DepexString + Dep
#            List.append(Dep)
#        self.Module.Depex = List
#        if self.Module.ModuleHeader.ModuleType in ['DXE_SMM_DRIVER']:
#            self.Module.SmmDepex = List
#        elif self.Module.ModuleHeader.ModuleType in ['PEI_CORE', 'PEIM']:
#            self.Module.PeiDepex = List
#        else:
#            self.Module.DxeDepex = List

    ## GenBinaries
    #
    # Gen Binary of Inf
    # <FileType>|<Filename>|<Target>[|<TokenSpaceGuidCName>.<PcdCName>]
    #
    # @param ContainerFile: The Dec file full path 
    #
    def GenBinaries(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_BINARIES)
        Binaries = {}

        # Get all Guids
        RecordSet = self.RecordSet[MODEL_EFI_BINARY_FILE]
        for Record in RecordSet:
            (FileType, Filename, Target, Pcd) = GetBinary(Record[0], ContainerFile, self.Identification.RelaPath, Record[2])
            Binary = ModuleBinaryFileClass(Filename, FileType, Target, Pcd, Record[1])
            if GenerateHelpText(Record[5], ''):
                Binary.HelpTextList.append(GenerateHelpText(Record[5], ''))
            self.Module.Binaries.append(Binary)
            #self.Module.FileList.append(os.path.normpath(os.path.join(self.Identification.RelaPath, Filename)))

    ## GenGuids
    #
    # Gen Guids of Inf
    # <CName>=<GuidValue>
    #
    # @param ContainerFile: The Inf file full path 
    #
    def GenGuidProtocolPpis(self, Type, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % Type)
        Lists = {}
        # Get all Items
        if Type == TAB_GUIDS:
            ListMember = self.Module.Guids
        elif Type == TAB_PROTOCOLS:
            ListMember = self.Module.Protocols
        elif Type == TAB_PPIS:
            ListMember = self.Module.Ppis

        RecordSet = self.RecordSet[Section[Type.upper()]]
        for Record in RecordSet:
            (Name, Value) = GetGuidsProtocolsPpisOfInf(Record[0], Type, ContainerFile, Record[2])
            ListClass = GuidProtocolPpiCommonClass()
            ListClass.CName = Name
            ListClass.SupArchList = Record[1]
            ListClass.FeatureFlag = Value
            if GenerateHelpText(Record[5], ''):
                ListClass.HelpTextList.append(GenerateHelpText(Record[5], ''))
            ListMember.append(ListClass)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.QUIET)

    W = os.getenv('WORKSPACE')
    F = os.path.join(W, 'MdeModulePkg/Application/HelloWorld/HelloWorld.inf')

    P = Inf(os.path.normpath(F), True, W, 'MdeModulePkg')
    P.ShowModule()
    print P.ModuleToInf(P.Module)
