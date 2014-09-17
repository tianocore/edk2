## @file
# This file is used to define each component of INF file
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
import Common.LongFilePathOs as os
import re
import EdkLogger
from CommonDataClass.CommonClass import LibraryClassClass
from CommonDataClass.ModuleClass import *
from String import *
from DataType import *
from Identification import *
from Dictionary import *
from BuildToolError import *
from Misc import sdict
import GlobalData
from Table.TableInf import TableInf
import Database
from Parsing import *
from Common.LongFilePathSupport import OpenLongFilePath as open

#
# Global variable
#
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

gNmakeFlagPattern = re.compile("(?:EBC_)?([A-Z]+)_(?:STD_|PROJ_|ARCH_)?FLAGS(?:_DLL|_ASL|_EXE)?", re.UNICODE)
gNmakeFlagName2ToolCode = {
    "C"         :   "CC",
    "LIB"       :   "SLINK",
    "LINK"      :   "DLINK",
}

class InfHeader(ModuleHeaderClass):
    _Mapping_ = {
        #
        # Required Fields
        #
        TAB_INF_DEFINES_BASE_NAME                   : "Name",
        TAB_INF_DEFINES_FILE_GUID                   : "Guid",
        TAB_INF_DEFINES_MODULE_TYPE                 : "ModuleType",
        TAB_INF_DEFINES_EFI_SPECIFICATION_VERSION   : "UefiSpecificationVersion",
        TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION  : "UefiSpecificationVersion",
        TAB_INF_DEFINES_EDK_RELEASE_VERSION         : "EdkReleaseVersion",
        #
        # Optional Fields
        #
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
#       TAB_INF_DEFINES_LIBRARY_CLASS               : "LibraryClass",
#        TAB_INF_DEFINES_ENTRY_POINT                 : "ExternImages",
#        TAB_INF_DEFINES_UNLOAD_IMAGE                : "ExternImages",
#        TAB_INF_DEFINES_CONSTRUCTOR                 : ,
#        TAB_INF_DEFINES_DESTRUCTOR                  : ,
#        TAB_INF_DEFINES_DEFINE                      : "Define",
#        TAB_INF_DEFINES_SPEC                        : "Specification",
#        TAB_INF_DEFINES_CUSTOM_MAKEFILE             : "CustomMakefile",
#        TAB_INF_DEFINES_MACRO                       :
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
    def __init__(self, Filename=None, IsToDatabase=False, IsToModule=False, WorkspaceDir=None, Database=None, SupArchList=DataType.ARCH_LIST):
        self.Identification = Identification()
        self.Module = ModuleClass()
        self.UserExtensions = ''
        self.WorkspaceDir = WorkspaceDir
        self.SupArchList = SupArchList
        self.IsToDatabase = IsToDatabase

        self.Cur = Database.Cur
        self.TblFile = Database.TblFile
        self.TblInf = Database.TblInf
        self.FileID = -1
        #self.TblInf = TableInf(Database.Cur)

        self.KeyList = [
            TAB_SOURCES, TAB_BUILD_OPTIONS, TAB_BINARIES, TAB_INCLUDES, TAB_GUIDS,
            TAB_PROTOCOLS, TAB_PPIS, TAB_LIBRARY_CLASSES, TAB_PACKAGES, TAB_LIBRARIES,
            TAB_INF_FIXED_PCD, TAB_INF_PATCH_PCD, TAB_INF_FEATURE_PCD, TAB_INF_PCD,
            TAB_INF_PCD_EX, TAB_DEPEX, TAB_NMAKE, TAB_INF_DEFINES
        ]
        #
        # Upper all KEYs to ignore case sensitive when parsing
        #
        self.KeyList = map(lambda c: c.upper(), self.KeyList)

        #
        # Init RecordSet
        #
        self.RecordSet = {}
        for Key in self.KeyList:
            self.RecordSet[Section[Key]] = []

        #
        # Load Inf file if filename is not None
        #
        if Filename != None:
            self.LoadInfFile(Filename)

        #
        # Transfer to Module Object if IsToModule is True
        #
        if IsToModule:
            self.InfToModule()

    ## Transfer to Module Object
    #
    # Transfer all contents of an Inf file to a standard Module Object
    #
    def InfToModule(self):
        #
        # Init global information for the file
        #
        ContainerFile = self.Identification.FileFullPath

        #
        # Generate Package Header
        #
        self.GenModuleHeader(ContainerFile)

        #
        # Generate BuildOptions
        #
        self.GenBuildOptions(ContainerFile)

        #
        # Generate Includes
        #
        self.GenIncludes(ContainerFile)

        #
        # Generate Libraries
        #
        self.GenLibraries(ContainerFile)

        #
        # Generate LibraryClasses
        #
        self.GenLibraryClasses(ContainerFile)

        #
        # Generate Packages
        #
        self.GenPackages(ContainerFile)

        #
        # Generate Nmakes
        #
        self.GenNmakes(ContainerFile)

        #
        # Generate Pcds
        #
        self.GenPcds(ContainerFile)

        #
        # Generate Sources
        #
        self.GenSources(ContainerFile)

        #
        # Generate UserExtensions
        #
        self.GenUserExtensions(ContainerFile)

        #
        # Generate Guids
        #
        self.GenGuidProtocolPpis(DataType.TAB_GUIDS, ContainerFile)

        #
        # Generate Protocols
        #
        self.GenGuidProtocolPpis(DataType.TAB_PROTOCOLS, ContainerFile)

        #
        # Generate Ppis
        #
        self.GenGuidProtocolPpis(DataType.TAB_PPIS, ContainerFile)

        #
        # Generate Depexes
        #
        self.GenDepexes(ContainerFile)

        #
        # Generate Binaries
        #
        self.GenBinaries(ContainerFile)

    ## Parse [Defines] section
    #
    # Parse [Defines] section into InfDefines object
    #
    # @param InfFile    The path of the INF file
    # @param Section    The title of "Defines" section
    # @param Lines      The content of "Defines" section
    #
    def ParseDefines(self, InfFile, Section, Lines):
        TokenList = Section.split(TAB_SPLIT)
        if len(TokenList) == 3:
            RaiseParserError(Section, "Defines", InfFile, "[xx.yy.%s] format (with platform) is not supported")
        if len(TokenList) == 2:
            Arch = TokenList[1].upper()
        else:
            Arch = TAB_ARCH_COMMON

        if Arch not in self.Defines:
            self.Defines[Arch] = InfDefines()
        GetSingleValueOfKeyFromLines(Lines, self.Defines[Arch].DefinesDictionary,
                                     TAB_COMMENT_SPLIT, TAB_EQUAL_SPLIT, False, None)

    ## Load Inf file
    #
    # Load the file if it exists
    #
    # @param Filename:  Input value for filename of Inf file
    #
    def LoadInfFile(self, Filename):
        #
        # Insert a record for file
        #
        Filename = NormPath(Filename)
        self.Identification.FileFullPath = Filename
        (self.Identification.FileRelativePath, self.Identification.FileName) = os.path.split(Filename)
        self.FileID = self.TblFile.InsertFile(Filename, MODEL_FILE_INF)

        #
        # Init InfTable
        #
        #self.TblInf.Table = "Inf%s" % self.FileID
        #self.TblInf.Create()

        #
        # Init common datas
        #
        IfDefList, SectionItemList, CurrentSection, ArchList, ThirdList, IncludeFiles = \
        [], [], TAB_UNKNOWN, [], [], []
        LineNo = 0

        #
        # Parse file content
        #
        IsFindBlockComment = False
        ReservedLine = ''
        for Line in open(Filename, 'r'):
            LineNo = LineNo + 1
            #
            # Remove comment block
            #
            if Line.find(TAB_COMMENT_EDK_START) > -1:
                ReservedLine = GetSplitList(Line, TAB_COMMENT_EDK_START, 1)[0]
                IsFindBlockComment = True
            if Line.find(TAB_COMMENT_EDK_END) > -1:
                Line = ReservedLine + GetSplitList(Line, TAB_COMMENT_EDK_END, 1)[1]
                ReservedLine = ''
                IsFindBlockComment = False
            if IsFindBlockComment:
                continue

            #
            # Remove comments at tail and remove spaces again
            #
            Line = CleanString(Line)
            if Line == '':
                continue

            #
            # Find a new section tab
            # First insert previous section items
            # And then parse the content of the new section
            #
            if Line.startswith(TAB_SECTION_START) and Line.endswith(TAB_SECTION_END):
                if Line[1:3] == "--":
                    continue
                Model = Section[CurrentSection.upper()]
                #
                # Insert items data of previous section
                #
                InsertSectionItemsIntoDatabase(self.TblInf, self.FileID, Filename, Model, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList, self.RecordSet)
                #
                # Parse the new section
                #
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
                        CurrentSection = TAB_UNKNOWN
                        continue
                    ItemList.append('')
                    ItemList.append('')
                    if len(ItemList) > 5:
                        RaiseParserError(Line, CurrentSection, Filename, '', LineNo)
                    else:
                        if ItemList[1] != '' and ItemList[1].upper() not in ARCH_LIST_FULL:
                            EdkLogger.error("Parser", PARSER_ERROR, "Invalid Arch definition '%s' found" % ItemList[1], File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)
                        ArchList.append(ItemList[1].upper())
                        ThirdList.append(ItemList[2])

                continue

            #
            # Not in any defined section
            #
            if CurrentSection == TAB_UNKNOWN:
                ErrorMsg = "%s is not in any defined section" % Line
                EdkLogger.error("Parser", PARSER_ERROR, ErrorMsg, File=Filename, Line=LineNo, RaiseError=EdkLogger.IsRaiseError)

            #
            # Add a section item
            #
            SectionItemList.append([Line, LineNo])
            # End of parse
        #End of For

        #
        # Insert items data of last section
        #
        Model = Section[CurrentSection.upper()]
        InsertSectionItemsIntoDatabase(self.TblInf, self.FileID, Filename, Model, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList, self.RecordSet)

        #
        # Replace all DEFINE macros with its actual values
        #
        ParseDefineMacro2(self.TblInf, self.RecordSet, GlobalData.gGlobalDefines)

    ## Show detailed information of Module
    #
    # Print all members and their values of Module class
    #
    def ShowModule(self):
        M = self.Module
        for Arch in M.Header.keys():
            print '\nArch =', Arch
            print 'Filename =', M.Header[Arch].FileName
            print 'FullPath =', M.Header[Arch].FullPath
            print 'BaseName =', M.Header[Arch].Name
            print 'Guid =', M.Header[Arch].Guid
            print 'Version =', M.Header[Arch].Version
            print 'InfVersion =', M.Header[Arch].InfVersion
            print 'UefiSpecificationVersion =', M.Header[Arch].UefiSpecificationVersion
            print 'EdkReleaseVersion =', M.Header[Arch].EdkReleaseVersion
            print 'ModuleType =', M.Header[Arch].ModuleType
            print 'BinaryModule =', M.Header[Arch].BinaryModule
            print 'ComponentType =', M.Header[Arch].ComponentType
            print 'MakefileName =', M.Header[Arch].MakefileName
            print 'BuildNumber =', M.Header[Arch].BuildNumber
            print 'BuildType =', M.Header[Arch].BuildType
            print 'FfsExt =', M.Header[Arch].FfsExt
            print 'FvExt =', M.Header[Arch].FvExt
            print 'SourceFv =', M.Header[Arch].SourceFv
            print 'PcdIsDriver =', M.Header[Arch].PcdIsDriver
            print 'TianoEdkFlashMap_h =', M.Header[Arch].TianoEdkFlashMap_h
            print 'Shadow =', M.Header[Arch].Shadow
            print 'LibraryClass =', M.Header[Arch].LibraryClass
            for Item in M.Header[Arch].LibraryClass:
                print Item.LibraryClass, DataType.TAB_VALUE_SPLIT.join(Item.SupModuleList)
            print 'CustomMakefile =', M.Header[Arch].CustomMakefile
            print 'Define =', M.Header[Arch].Define
            print 'Specification =', M.Header[Arch].Specification
        for Item in self.Module.ExternImages:
            print '\nEntry_Point = %s, UnloadImage = %s' % (Item.ModuleEntryPoint, Item.ModuleUnloadImage)
        for Item in self.Module.ExternLibraries:
            print 'Constructor = %s, Destructor = %s' % (Item.Constructor, Item.Destructor)
        print '\nBuildOptions =', M.BuildOptions
        for Item in M.BuildOptions:
            print Item.ToolChainFamily, Item.ToolChain, Item.Option, Item.SupArchList
        print '\nIncludes =', M.Includes
        for Item in M.Includes:
            print Item.FilePath, Item.SupArchList
        print '\nLibraries =', M.Libraries
        for Item in M.Libraries:
            print Item.Library, Item.SupArchList
        print '\nLibraryClasses =', M.LibraryClasses
        for Item in M.LibraryClasses:
            print Item.LibraryClass, Item.RecommendedInstance, Item.FeatureFlag, Item.SupModuleList, Item.SupArchList, Item.Define
        print '\nPackageDependencies =', M.PackageDependencies
        for Item in M.PackageDependencies:
            print Item.FilePath, Item.SupArchList, Item.FeatureFlag
        print '\nNmake =', M.Nmake
        for Item in M.Nmake:
            print Item.Name, Item.Value, Item.SupArchList
        print '\nPcds =', M.PcdCodes
        for Item in M.PcdCodes:
            print '\tCName=', Item.CName, 'TokenSpaceGuidCName=', Item.TokenSpaceGuidCName, 'DefaultValue=', Item.DefaultValue, 'ItemType=', Item.ItemType, Item.SupArchList
        print '\nSources =', M.Sources
        for Source in M.Sources:
            print Source.SourceFile, 'Fam=', Source.ToolChainFamily, 'Pcd=', Source.FeatureFlag, 'Tag=', Source.TagName, 'ToolCode=', Source.ToolCode, Source.SupArchList
        print '\nUserExtensions =', M.UserExtensions
        for UserExtension in M.UserExtensions:
            print UserExtension.UserID, UserExtension.Identifier, UserExtension.Content
        print '\nGuids =', M.Guids
        for Item in M.Guids:
            print Item.CName, Item.SupArchList, Item.FeatureFlag
        print '\nProtocols =', M.Protocols
        for Item in M.Protocols:
            print Item.CName, Item.SupArchList, Item.FeatureFlag
        print '\nPpis =', M.Ppis
        for Item in M.Ppis:
            print Item.CName, Item.SupArchList, Item.FeatureFlag
        print '\nDepex =', M.Depex
        for Item in M.Depex:
            print Item.Depex, Item.SupArchList, Item.Define
        print '\nBinaries =', M.Binaries
        for Binary in M.Binaries:
            print 'Type=', Binary.FileType, 'Target=', Binary.Target, 'Name=', Binary.BinaryFile, 'FeatureFlag=', Binary.FeatureFlag, 'SupArchList=', Binary.SupArchList

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
        File = self.Identification.FileFullPath
        #
        # Update all defines item in database
        #
        RecordSet = self.RecordSet[MODEL_META_DATA_HEADER]
        for Record in RecordSet:
            ValueList = GetSplitValueList(Record[0], TAB_EQUAL_SPLIT)
            if len(ValueList) != 2:
                RaiseParserError(Record[0], 'Defines', ContainerFile, '<Key> = <Value>', Record[2])
            ID, Value1, Value2, Arch, LineNo = Record[3], ValueList[0], ValueList[1], Record[1], Record[2]
            SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                            where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(Value1), ConvertToSqlString2(Value2), ID)
            self.TblInf.Exec(SqlCommand)

        for Arch in DataType.ARCH_LIST:
            ModuleHeader = InfHeader()
            ModuleHeader.FileName = self.Identification.FileName
            ModuleHeader.FullPath = self.Identification.FileFullPath
            DefineList = QueryDefinesItem2(self.TblInf, Arch, self.FileID)

            NotProcessedDefineList = []
            for D in DefineList:
                if D[0] in ModuleHeader:
                    ModuleHeader[D[0]] = GetSplitValueList(D[1])[0]
                else:
                    NotProcessedDefineList.append(D)

            if ModuleHeader.ComponentType == "LIBRARY":
                Lib = LibraryClassClass()
                Lib.LibraryClass = ModuleHeader.Name
                Lib.SupModuleList = DataType.SUP_MODULE_LIST
                ModuleHeader.LibraryClass.append(Lib)

            # we need to make some key defines resolved first
            for D in NotProcessedDefineList:
                if D[0] == TAB_INF_DEFINES_LIBRARY_CLASS:
                    List = GetSplitValueList(D[1], DataType.TAB_VALUE_SPLIT, 1)
                    Lib = LibraryClassClass()
                    Lib.LibraryClass = CleanString(List[0])
                    if len(List) == 1:
                        Lib.SupModuleList = DataType.SUP_MODULE_LIST
                    elif len(List) == 2:
                        Lib.SupModuleList = GetSplitValueList(CleanString(List[1]), ' ')
                    ModuleHeader.LibraryClass.append(Lib)
                elif D[0] == TAB_INF_DEFINES_CUSTOM_MAKEFILE:
                    List = D[1].split(DataType.TAB_VALUE_SPLIT)
                    if len(List) == 2:
                        ModuleHeader.CustomMakefile[CleanString(List[0])] = CleanString(List[1])
                    else:
                        RaiseParserError(D[1], 'CUSTOM_MAKEFILE of Defines', File, 'CUSTOM_MAKEFILE=<Family>|<Filename>', D[2])
                elif D[0] == TAB_INF_DEFINES_ENTRY_POINT:
                    Image = ModuleExternImageClass()
                    Image.ModuleEntryPoint = CleanString(D[1])
                    self.Module.ExternImages.append(Image)
                elif D[0] == TAB_INF_DEFINES_UNLOAD_IMAGE:
                    Image = ModuleExternImageClass()
                    Image.ModuleUnloadImage = CleanString(D[1])
                    self.Module.ExternImages.append(Image)
                elif D[0] == TAB_INF_DEFINES_CONSTRUCTOR:
                    LibraryClass = ModuleExternLibraryClass()
                    LibraryClass.Constructor = CleanString(D[1])
                    self.Module.ExternLibraries.append(LibraryClass)
                elif D[0] == TAB_INF_DEFINES_DESTRUCTOR:
                    LibraryClass = ModuleExternLibraryClass()
                    LibraryClass.Destructor = CleanString(D[1])
                    self.Module.ExternLibraries.append(LibraryClass)
                elif D[0] == TAB_INF_DEFINES_DEFINE:
                    List = D[1].split(DataType.TAB_EQUAL_SPLIT)
                    if len(List) != 2:
                        RaiseParserError(Item, 'DEFINE of Defines', File, 'DEFINE <Word> = <Word>', D[2])
                    else:
                        ModuleHeader.Define[CleanString(List[0])] = CleanString(List[1])
                elif D[0] == TAB_INF_DEFINES_SPEC:
                    List = D[1].split(DataType.TAB_EQUAL_SPLIT)
                    if len(List) != 2:
                        RaiseParserError(Item, 'SPEC of Defines', File, 'SPEC <Word> = <Version>', D[2])
                    else:
                        ModuleHeader.Specification[CleanString(List[0])] = CleanString(List[1])

            #
            # Get version of INF
            #
            if ModuleHeader.InfVersion != "":
                # EdkII inf
                VersionNumber = ModuleHeader.VersionNumber
                VersionString = ModuleHeader.VersionString
                if len(VersionNumber) > 0 and len(VersionString) == 0:
                    EdkLogger.warn(2000, 'VERSION_NUMBER depricated; INF file %s should be modified to use VERSION_STRING instead.' % self.Identification.FileFullPath)
                    ModuleHeader.Version = VersionNumber
                if len(VersionString) > 0:
                    if len(VersionNumber) > 0:
                        EdkLogger.warn(2001, 'INF file %s defines both VERSION_NUMBER and VERSION_STRING, using VERSION_STRING' % self.Identification.FileFullPath)
                    ModuleHeader.Version = VersionString
            else:
                # Edk inf
                ModuleHeader.InfVersion = "0x00010000"
                if ModuleHeader.ComponentType in gComponentType2ModuleType:
                    ModuleHeader.ModuleType = gComponentType2ModuleType[ModuleHeader.ComponentType]
                elif ModuleHeader.ComponentType != '':
                    EdkLogger.error("Parser", PARSER_ERROR, "Unsupported Edk component type [%s]" % ModuleHeader.ComponentType, ExtraData=File, RaiseError=EdkLogger.IsRaiseError)

            self.Module.Header[Arch] = ModuleHeader


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
        #
        # Get all BuildOptions
        #
        RecordSet = self.RecordSet[MODEL_META_DATA_BUILD_OPTION]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (Family, ToolChain, Flag) = GetBuildOption(Record[0], ContainerFile, Record[2])
                    MergeArches(BuildOptions, (Family, ToolChain, Flag), Arch)
                    #
                    # Update to Database
                    #
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s'
                                        where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(Family), ConvertToSqlString2(ToolChain), ConvertToSqlString2(Flag), Record[3])
                        self.TblInf.Exec(SqlCommand)

        for Key in BuildOptions.keys():
            BuildOption = BuildOptionClass(Key[0], Key[1], Key[2])
            BuildOption.SupArchList = BuildOptions[Key]
            self.Module.BuildOptions.append(BuildOption)

    ## GenIncludes
    #
    # Gen Includes of Inf
    #
    #
    # @param ContainerFile: The Inf file full path
    #
    def GenIncludes(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_INCLUDES)
        Includes = sdict()
        #
        # Get all Includes
        #
        RecordSet = self.RecordSet[MODEL_EFI_INCLUDE]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    MergeArches(Includes, Record[0], Arch)

        for Key in Includes.keys():
            Include = IncludeClass()
            Include.FilePath = NormPath(Key)
            Include.SupArchList = Includes[Key]
            self.Module.Includes.append(Include)

    ## GenLibraries
    #
    # Gen Libraries of Inf
    #
    #
    # @param ContainerFile: The Inf file full path
    #
    def GenLibraries(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_LIBRARIES)
        Libraries = sdict()
        #
        # Get all Includes
        #
        RecordSet = self.RecordSet[MODEL_EFI_LIBRARY_INSTANCE]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    MergeArches(Libraries, Record[0], Arch)

        for Key in Libraries.keys():
            Library = ModuleLibraryClass()
            # replace macro and remove file extension
            Library.Library = Key.rsplit('.', 1)[0]
            Library.SupArchList = Libraries[Key]
            self.Module.Libraries.append(Library)

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
        #
        # Get all LibraryClasses
        #
        RecordSet = self.RecordSet[MODEL_EFI_LIBRARY_CLASS]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (LibClassName, LibClassIns, Pcd, SupModelList) = GetLibraryClassOfInf([Record[0], Record[4]], ContainerFile, self.WorkspaceDir, Record[2])
                    MergeArches(LibraryClasses, (LibClassName, LibClassIns, Pcd, SupModelList), Arch)
                    #
                    # Update to Database
                    #
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s'
                                        where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(LibClassName), ConvertToSqlString2(LibClassIns), ConvertToSqlString2(SupModelList), Record[3])
                        self.TblInf.Exec(SqlCommand)

        for Key in LibraryClasses.keys():
            KeyList = Key[0].split(DataType.TAB_VALUE_SPLIT)
            LibraryClass = LibraryClassClass()
            LibraryClass.LibraryClass = Key[0]
            LibraryClass.RecommendedInstance = NormPath(Key[1])
            LibraryClass.FeatureFlag = Key[2]
            LibraryClass.SupArchList = LibraryClasses[Key]
            LibraryClass.SupModuleList = GetSplitValueList(Key[3])
            self.Module.LibraryClasses.append(LibraryClass)

    ## GenPackages
    #
    # Gen Packages of Inf
    #
    #
    # @param ContainerFile: The Inf file full path
    #
    def GenPackages(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_PACKAGES)
        Packages = {}
        #
        # Get all Packages
        #
        RecordSet = self.RecordSet[MODEL_META_DATA_PACKAGE]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (Package, Pcd) = GetPackage(Record[0], ContainerFile, self.WorkspaceDir, Record[2])
                    MergeArches(Packages, (Package, Pcd), Arch)
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                                        where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(Package), ConvertToSqlString2(Pcd), Record[3])
                        self.TblInf.Exec(SqlCommand)


        for Key in Packages.keys():
            Package = ModulePackageDependencyClass()
            Package.FilePath = NormPath(Key[0])
            Package.SupArchList = Packages[Key]
            Package.FeatureFlag = Key[1]
            self.Module.PackageDependencies.append(Package)

    ## GenNmakes
    #
    # Gen Nmakes of Inf
    #
    #
    # @param ContainerFile: The Inf file full path
    #
    def GenNmakes(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_NMAKE)
        Nmakes = sdict()
        #
        # Get all Nmakes
        #
        RecordSet = self.RecordSet[MODEL_META_DATA_NMAKE]


        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    MergeArches(Nmakes, Record[0], Arch)

        for Key in Nmakes.keys():
            List = GetSplitValueList(Key, DataType.TAB_EQUAL_SPLIT, MaxSplit=1)
            if len(List) != 2:
                RaiseParserError(Key, 'Nmake', ContainerFile, '<MacroName> = <Value>')
                continue
            Nmake = ModuleNmakeClass()
            Nmake.Name = List[0]
            Nmake.Value = List[1]
            Nmake.SupArchList = Nmakes[Key]
            self.Module.Nmake.append(Nmake)

            # convert Edk format to EdkII format
            if Nmake.Name == "IMAGE_ENTRY_POINT":
                Image = ModuleExternImageClass()
                Image.ModuleEntryPoint = Nmake.Value
                self.Module.ExternImages.append(Image)
            elif Nmake.Name == "DPX_SOURCE":
                Source = ModuleSourceFileClass(NormPath(Nmake.Value), "", "", "", "", Nmake.SupArchList)
                self.Module.Sources.append(Source)
            else:
                ToolList = gNmakeFlagPattern.findall(Nmake.Name)
                if len(ToolList) == 0 or len(ToolList) != 1:
                    EdkLogger.warn("\nParser", "Don't know how to do with MACRO: %s" % Nmake.Name,
                                   ExtraData=ContainerFile)
                else:
                    if ToolList[0] in gNmakeFlagName2ToolCode:
                        Tool = gNmakeFlagName2ToolCode[ToolList[0]]
                    else:
                        Tool = ToolList[0]
                    BuildOption = BuildOptionClass("MSFT", "*_*_*_%s_FLAGS" % Tool, Nmake.Value)
                    BuildOption.SupArchList = Nmake.SupArchList
                    self.Module.BuildOptions.append(BuildOption)

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

        #
        # Get all Guids
        #
        RecordSet1 = self.RecordSet[MODEL_PCD_FIXED_AT_BUILD]
        RecordSet2 = self.RecordSet[MODEL_PCD_PATCHABLE_IN_MODULE]
        RecordSet3 = self.RecordSet[MODEL_PCD_FEATURE_FLAG]
        RecordSet4 = self.RecordSet[MODEL_PCD_DYNAMIC_EX]
        RecordSet5 = self.RecordSet[MODEL_PCD_DYNAMIC]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet1:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    if self.Module.Header[Arch].LibraryClass != {}:
                        pass
                    (TokenGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_FIXED_AT_BUILD, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet2:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_PATCHABLE_IN_MODULE, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet3:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_FEATURE_FLAG, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet4:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], TAB_PCDS_DYNAMIC_EX, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
            for Record in RecordSet5:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (TokenGuidCName, TokenName, Value, Type) = GetPcdOfInf(Record[0], "", ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenGuidCName, TokenName, Value, Type), Arch)
                    PcdToken[Record[3]] = (TokenGuidCName, TokenName)
        #
        # Update to database
        #
        if self.IsToDatabase:
            for Key in PcdToken.keys():
                SqlCommand = """update %s set Value2 = '%s' where ID = %s""" % (self.TblInf.Table, ".".join((PcdToken[Key][0], PcdToken[Key][1])), Key)
                self.TblInf.Exec(SqlCommand)

        for Key in Pcds.keys():
            Pcd = PcdClass()
            Pcd.CName = Key[1]
            Pcd.TokenSpaceGuidCName = Key[0]
            Pcd.DefaultValue = Key[2]
            Pcd.ItemType = Key[3]
            Pcd.SupArchList = Pcds[Key]
            self.Module.PcdCodes.append(Pcd)

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

        #
        # Get all Nmakes
        #
        RecordSet = self.RecordSet[MODEL_EFI_SOURCE_FILE]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (Filename, Family, TagName, ToolCode, Pcd) = GetSource(Record[0], ContainerFile, self.Identification.FileRelativePath, Record[2])
                    MergeArches(Sources, (Filename, Family, TagName, ToolCode, Pcd), Arch)
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s', Value4 = '%s', Value5 = '%s'
                                        where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(Filename), ConvertToSqlString2(Family), ConvertToSqlString2(TagName), ConvertToSqlString2(ToolCode), ConvertToSqlString2(Pcd), Record[3])
                        self.TblInf.Exec(SqlCommand)

        for Key in Sources.keys():
            Source = ModuleSourceFileClass(Key[0], Key[2], Key[3], Key[1], Key[4], Sources[Key])
            self.Module.Sources.append(Source)

    ## GenUserExtensions
    #
    # Gen UserExtensions of Inf
    #
    def GenUserExtensions(self, ContainerFile):
#        #
#        # UserExtensions
#        #
#        if self.UserExtensions != '':
#            UserExtension = UserExtensionsClass()
#            Lines = self.UserExtensions.splitlines()
#            List = GetSplitValueList(Lines[0], DataType.TAB_SPLIT, 2)
#            if len(List) != 3:
#                RaiseParserError(Lines[0], 'UserExtensions', File, "UserExtensions.UserId.'Identifier'")
#            else:
#                UserExtension.UserID = List[1]
#                UserExtension.Identifier = List[2][0:-1].replace("'", '').replace('\"', '')
#                for Line in Lines[1:]:
#                    UserExtension.Content = UserExtension.Content + CleanString(Line) + '\n'
#            self.Module.UserExtensions.append(UserExtension)
        pass

    ## GenDepexes
    #
    # Gen Depex of Inf
    #
    # @param ContainerFile: The Inf file full path
    #
    def GenDepexes(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_DEPEX)
        Depex = {}
        #
        # Get all Depexes
        #
        RecordSet = self.RecordSet[MODEL_EFI_DEPEX]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            Line = ''
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    Line = Line + Record[0] + ' '
            if Line != '':
                MergeArches(Depex, Line, Arch)

        for Key in Depex.keys():
            Dep = ModuleDepexClass()
            Dep.Depex = Key
            Dep.SupArchList = Depex[Key]
            self.Module.Depex.append(Dep)

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

        #
        # Get all Guids
        #
        RecordSet = self.RecordSet[MODEL_EFI_BINARY_FILE]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (FileType, Filename, Target, Pcd) = GetBinary(Record[0], ContainerFile, self.Identification.FileRelativePath, Record[2])
                    MergeArches(Binaries, (FileType, Filename, Target, Pcd), Arch)
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s', Value4 = '%s'
                                        where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(FileType), ConvertToSqlString2(Filename), ConvertToSqlString2(Target), ConvertToSqlString2(Pcd), Record[3])
                        self.TblInf.Exec(SqlCommand)

        for Key in Binaries.keys():
            Binary = ModuleBinaryFileClass(NormPath(Key[1]), Key[0], Key[2], Key[3], Binaries[Key])
            self.Module.Binaries.append(Binary)

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
        #
        # Get all Items
        #
        RecordSet = self.RecordSet[Section[Type.upper()]]

        #
        # Go through each arch
        #
        for Arch in self.SupArchList:
            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON:
                    (Name, Value) = GetGuidsProtocolsPpisOfInf(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Lists, (Name, Value), Arch)
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                                        where ID = %s""" % (self.TblInf.Table, ConvertToSqlString2(Name), ConvertToSqlString2(Value), Record[3])
                        self.TblInf.Exec(SqlCommand)

        ListMember = None
        if Type == TAB_GUIDS:
            ListMember = self.Module.Guids
        elif Type == TAB_PROTOCOLS:
            ListMember = self.Module.Protocols
        elif Type == TAB_PPIS:
            ListMember = self.Module.Ppis

        for Key in Lists.keys():
            ListClass = GuidProtocolPpiCommonClass()
            ListClass.CName = Key[0]
            ListClass.SupArchList = Lists[Key]
            ListClass.FeatureFlag = Key[1]
            ListMember.append(ListClass)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)

    W = os.getenv('WORKSPACE')
    F = os.path.join(W, 'MdeModulePkg/Application/HelloWorld/HelloWorld.inf')

    Db = Database.Database('Inf.db')
    Db.InitDatabase()

    P = Inf(os.path.normpath(F), True, True, W, Db)
    P.ShowModule()

    Db.Close()
