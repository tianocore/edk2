## @file
# This file is used to define each component of DSC file
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
import EdkLogger as EdkLogger
import Database
from String import *
from Parsing import *
from DataType import *
from Identification import *
from Dictionary import *
from CommonDataClass.PlatformClass import *
from CommonDataClass.CommonClass import SkuInfoClass
from BuildToolError import *
from Misc import sdict
import GlobalData
from Table.TableDsc import TableDsc

#
# Global variable
#
Section = {TAB_UNKNOWN.upper() : MODEL_UNKNOWN,
           TAB_DSC_DEFINES.upper() : MODEL_META_DATA_HEADER,
           TAB_BUILD_OPTIONS.upper() : MODEL_META_DATA_BUILD_OPTION,
           TAB_SKUIDS.upper() : MODEL_EFI_SKU_ID,
           TAB_LIBRARIES.upper() : MODEL_EFI_LIBRARY_INSTANCE,
           TAB_LIBRARY_CLASSES.upper() : MODEL_EFI_LIBRARY_CLASS,
           TAB_PCDS_FIXED_AT_BUILD_NULL.upper() : MODEL_PCD_FIXED_AT_BUILD,
           TAB_PCDS_PATCHABLE_IN_MODULE_NULL.upper() : MODEL_PCD_PATCHABLE_IN_MODULE,
           TAB_PCDS_FEATURE_FLAG_NULL.upper() : MODEL_PCD_FEATURE_FLAG,
           TAB_PCDS_DYNAMIC_EX_NULL.upper() : MODEL_PCD_DYNAMIC_EX,
           TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL.upper() :  MODEL_PCD_DYNAMIC_EX_DEFAULT,
           TAB_PCDS_DYNAMIC_EX_VPD_NULL.upper() : MODEL_PCD_DYNAMIC_EX_VPD,
           TAB_PCDS_DYNAMIC_EX_HII_NULL.upper() : MODEL_PCD_DYNAMIC_EX_HII,
           TAB_PCDS_DYNAMIC_NULL.upper() : MODEL_PCD_DYNAMIC,
           TAB_PCDS_DYNAMIC_DEFAULT_NULL.upper() : MODEL_PCD_DYNAMIC_DEFAULT,
           TAB_PCDS_DYNAMIC_VPD_NULL.upper() : MODEL_PCD_DYNAMIC_VPD,
           TAB_PCDS_DYNAMIC_HII_NULL.upper() : MODEL_PCD_DYNAMIC_HII,
           TAB_COMPONENTS.upper() : MODEL_META_DATA_COMPONENT,
           TAB_USER_EXTENSIONS.upper() : MODEL_META_DATA_USER_EXTENSION
           }

## DscObject
#
# This class defined basic Dsc object which is used by inheriting
#
# @param object:       Inherited from object class
#
class DscObject(object):
    def __init__(self):
        object.__init__()

## Dsc
#
# This class defined the structure used in Dsc object
#
# @param DscObject:         Inherited from InfObject class
# @param Ffilename:         Input value for Ffilename of Inf file, default is None
# @param IsMergeAllArches:  Input value for IsMergeAllArches
#                           True is to merge all arches
#                           Fales is not to merge all arches
#                           default is False
# @param IsToPlatform:      Input value for IsToPlatform
#                           True is to transfer to ModuleObject automatically
#                           False is not to transfer to ModuleObject automatically
#                           default is False
# @param WorkspaceDir:      Input value for current workspace directory, default is None
#
# @var _NullClassIndex:     To store value for _NullClassIndex, default is 0
# @var Identification:      To store value for Identification, it is a structure as Identification
# @var Defines:             To store value for Defines, it is a structure as DscDefines
# @var Contents:            To store value for Contents, it is a structure as DscContents
# @var UserExtensions:      To store value for UserExtensions
# @var Platform:            To store value for Platform, it is a structure as PlatformClass
# @var WorkspaceDir:        To store value for WorkspaceDir
# @var KeyList:             To store value for KeyList, a list for all Keys used in Dec
#
class Dsc(DscObject):
    _NullClassIndex = 0

    def __init__(self, Filename=None, IsToDatabase=False, IsToPlatform=False, WorkspaceDir=None, Database=None):
        self.Identification = Identification()
        self.Platform = PlatformClass()
        self.UserExtensions = ''
        self.WorkspaceDir = WorkspaceDir
        self.IsToDatabase = IsToDatabase

        self.Cur = Database.Cur
        self.TblFile = Database.TblFile
        self.TblDsc = Database.TblDsc


        self.KeyList = [
            TAB_SKUIDS, TAB_LIBRARIES, TAB_LIBRARY_CLASSES, TAB_BUILD_OPTIONS, TAB_PCDS_FIXED_AT_BUILD_NULL, \
            TAB_PCDS_PATCHABLE_IN_MODULE_NULL, TAB_PCDS_FEATURE_FLAG_NULL, \
            TAB_PCDS_DYNAMIC_DEFAULT_NULL, TAB_PCDS_DYNAMIC_HII_NULL, TAB_PCDS_DYNAMIC_VPD_NULL, \
            TAB_PCDS_DYNAMIC_EX_DEFAULT_NULL, TAB_PCDS_DYNAMIC_EX_HII_NULL, TAB_PCDS_DYNAMIC_EX_VPD_NULL, \
            TAB_COMPONENTS, TAB_DSC_DEFINES
        ]

        self.PcdToken = {}

        #
        # Upper all KEYs to ignore case sensitive when parsing
        #
        self.KeyList = map(lambda c: c.upper(), self.KeyList)

        #
        # Init RecordSet
        #
#        self.RecordSet = {}
#        for Key in self.KeyList:
#            self.RecordSet[Section[Key]] = []

        #
        # Load Dsc file if filename is not None
        #
        if Filename != None:
            self.LoadDscFile(Filename)

        #
        # Transfer to Platform Object if IsToPlatform is True
        #
        if IsToPlatform:
            self.DscToPlatform()

    ## Transfer to Platform Object
    #
    # Transfer all contents of an Inf file to a standard Module Object
    #
    def DscToPlatform(self):
        #
        # Init global information for the file
        #
        ContainerFile = self.Identification.FileFullPath

        #
        # Generate Platform Header
        #
        self.GenPlatformHeader(ContainerFile)

        #
        # Generate BuildOptions
        #
        self.GenBuildOptions(ContainerFile)

        #
        # Generate SkuInfos
        #
        self.GenSkuInfos(ContainerFile)

        #
        # Generate Libraries
        #
        self.GenLibraries(ContainerFile)

        #
        # Generate LibraryClasses
        #
        self.GenLibraryClasses(ContainerFile)

        #
        # Generate Pcds
        #
        self.GenPcds(DataType.TAB_PCDS_FIXED_AT_BUILD, ContainerFile)
        self.GenPcds(DataType.TAB_PCDS_PATCHABLE_IN_MODULE, ContainerFile)
        self.GenFeatureFlagPcds(DataType.TAB_PCDS_FEATURE_FLAG, ContainerFile)
        self.GenDynamicDefaultPcds(DataType.TAB_PCDS_DYNAMIC_DEFAULT, ContainerFile)
        self.GenDynamicDefaultPcds(DataType.TAB_PCDS_DYNAMIC_EX_DEFAULT, ContainerFile)
        self.GenDynamicHiiPcds(DataType.TAB_PCDS_DYNAMIC_HII, ContainerFile)
        self.GenDynamicHiiPcds(DataType.TAB_PCDS_DYNAMIC_EX_HII, ContainerFile)
        self.GenDynamicVpdPcds(DataType.TAB_PCDS_DYNAMIC_VPD, ContainerFile)
        self.GenDynamicVpdPcds(DataType.TAB_PCDS_DYNAMIC_EX_VPD, ContainerFile)

        #
        # Generate Components
        #
        self.GenComponents(ContainerFile)

        #
        # Update to database
        #
        if self.IsToDatabase:
            for Key in self.PcdToken.keys():
                SqlCommand = """update %s set Value2 = '%s' where ID = %s""" % (self.TblDsc.Table, ".".join((self.PcdToken[Key][0], self.PcdToken[Key][1])), Key)
                self.TblDsc.Exec(SqlCommand)
    #End of DscToPlatform

    ## Get Platform Header
    #
    # Gen Platform Header of Dsc as <Key> = <Value>
    #
    # @param ContainerFile: The Dsc file full path
    #
    def GenPlatformHeader(self, ContainerFile):
        EdkLogger.debug(2, "Generate PlatformHeader ...")
        #
        # Update all defines item in database
        #
        SqlCommand = """select ID, Value1, Arch, StartLine from %s
                        where Model = %s
                        and BelongsToFile = %s
                        and Enabled > -1""" % (self.TblDsc.Table, MODEL_META_DATA_HEADER, self.FileID)
        RecordSet = self.TblDsc.Exec(SqlCommand)
        for Record in RecordSet:
            ValueList = GetSplitValueList(Record[1], TAB_EQUAL_SPLIT)
            if len(ValueList) != 2:
                RaiseParserError(Record[1], 'Defines', ContainerFile, '<Key> = <Value>', Record[3])
            ID, Value1, Value2, Arch = Record[0], ValueList[0], ValueList[1], Record[2]
            SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                            where ID = %s""" % (self.TblDsc.Table, ConvertToSqlString2(Value1), ConvertToSqlString2(Value2), ID)
            self.TblDsc.Exec(SqlCommand)

        #
        # Get detailed information
        #
        for Arch in DataType.ARCH_LIST:
            PlatformHeader = PlatformHeaderClass()

            PlatformHeader.Name = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_PLATFORM_NAME, Arch, self.FileID)[0]
            PlatformHeader.Guid = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_PLATFORM_GUID, Arch, self.FileID)[0]
            PlatformHeader.Version = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_PLATFORM_VERSION, Arch, self.FileID)[0]
            PlatformHeader.FileName = self.Identification.FileName
            PlatformHeader.FullPath = self.Identification.FileFullPath
            PlatformHeader.DscSpecification = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_DSC_SPECIFICATION, Arch, self.FileID)[0]

            PlatformHeader.SkuIdName = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_SKUID_IDENTIFIER, Arch, self.FileID)
            PlatformHeader.SupArchList = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_SUPPORTED_ARCHITECTURES, Arch, self.FileID)
            PlatformHeader.BuildTargets = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_BUILD_TARGETS, Arch, self.FileID)
            PlatformHeader.OutputDirectory = NormPath(QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_OUTPUT_DIRECTORY, Arch, self.FileID)[0])
            PlatformHeader.BuildNumber = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_BUILD_NUMBER, Arch, self.FileID)[0]
            PlatformHeader.MakefileName = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_MAKEFILE_NAME, Arch, self.FileID)[0]

            PlatformHeader.BsBaseAddress = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_BS_BASE_ADDRESS, Arch, self.FileID)[0]
            PlatformHeader.RtBaseAddress = QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_RT_BASE_ADDRESS, Arch, self.FileID)[0]

            self.Platform.Header[Arch] = PlatformHeader
            Fdf = PlatformFlashDefinitionFileClass()
            Fdf.FilePath = NormPath(QueryDefinesItem(self.TblDsc, TAB_DSC_DEFINES_FLASH_DEFINITION, Arch, self.FileID)[0])
            self.Platform.FlashDefinitionFile = Fdf

    ## GenBuildOptions
    #
    # Gen BuildOptions of Dsc
    # [<Family>:]<ToolFlag>=Flag
    #
    # @param ContainerFile: The Dsc file full path
    #
    def GenBuildOptions(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_BUILD_OPTIONS)
        BuildOptions = {}
        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, MODEL_META_DATA_BUILD_OPTION, self.FileID)

        #
        # Get all BuildOptions
        #
        RecordSet = QueryDscItem(self.TblDsc, MODEL_META_DATA_BUILD_OPTION, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, TAB_BUILD_OPTIONS, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        (Family, ToolChain, Flag) = GetBuildOption(NewItem, Filename, -1)
                        MergeArches(BuildOptions, (Family, ToolChain, Flag), Arch)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (Family, ToolChain, Flag) = GetBuildOption(Record[0], ContainerFile, Record[2])
                    MergeArches(BuildOptions, (Family, ToolChain, Flag), Arch)
                    #
                    # Update to Database
                    #
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s'
                                        where ID = %s""" % (self.TblDsc.Table, ConvertToSqlString2(Family), ConvertToSqlString2(ToolChain), ConvertToSqlString2(Flag), Record[3])
                        self.TblDsc.Exec(SqlCommand)

        for Key in BuildOptions.keys():
            BuildOption = BuildOptionClass(Key[0], Key[1], Key[2])
            BuildOption.SupArchList = BuildOptions[Key]
            self.Platform.BuildOptions.BuildOptionList.append(BuildOption)

    ## GenSkuInfos
    #
    # Gen SkuInfos of Dsc
    # <Integer>|<UiName>
    #
    # @param ContainerFile: The Dsc file full path
    #
    def GenSkuInfos(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_SKUIDS)
        #
        # SkuIds
        # <Integer>|<UiName>
        #
        self.Platform.SkuInfos.SkuInfoList['DEFAULT'] = '0'

        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, MODEL_EFI_SKU_ID, self.FileID)

        #
        # Get all SkuInfos
        #
        RecordSet = QueryDscItem(self.TblDsc, MODEL_EFI_SKU_ID, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, TAB_SKUIDS, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        List = GetSplitValueList(NewItem)
                        if len(List) != 2:
                            RaiseParserError(NewItem, TAB_SKUIDS, Filename, '<Integer>|<UiName>')
                        else:
                            self.Platform.SkuInfos.SkuInfoList[List[1]] = List[0]

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    List = GetSplitValueList(Record[0])
                    if len(List) != 2:
                        RaiseParserError(Record[0], TAB_SKUIDS, ContainerFile, '<Integer>|<UiName>')
                    else:
                        self.Platform.SkuInfos.SkuInfoList[List[1]] = List[0]
                        #
                        # Update to Database
                        #
                        if self.IsToDatabase:
                            SqlCommand = """update %s set Value1 = '%s', Value2 = '%s'
                                            where ID = %s""" % (self.TblDsc.Table, ConvertToSqlString2(List[0]), ConvertToSqlString2(List[1]), Record[3])
                            self.TblDsc.Exec(SqlCommand)

    ## GenLibraries
    #
    # Gen Libraries of Dsc
    # <PathAndFilename>
    #
    # @param ContainerFile: The Dsc file full path
    #
    def GenLibraries(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_LIBRARIES)
        Libraries = {}
        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, MODEL_EFI_LIBRARY_INSTANCE, self.FileID)

        #
        # Get all Libraries
        #
        RecordSet = QueryDscItem(self.TblDsc, MODEL_EFI_LIBRARY_INSTANCE, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, TAB_LIBRARIES, '', IncludeFile[2])
                    if os.path.exists(Filename):
                        for NewItem in open(Filename, 'r').readlines():
                            if CleanString(NewItem) == '':
                                continue
                            MergeArches(Libraries, NewItem, Arch)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    MergeArches(Libraries, Record[0], Arch)

        for Key in Libraries.keys():
            Library = PlatformLibraryClass()
            Library.FilePath = NormPath(Key)
            Library.SupArchList = Libraries[Key]
            self.Platform.Libraries.LibraryList.append(Library)

    ## GenLibraryClasses
    #
    # Get LibraryClasses of Dsc
    # <LibraryClassKeyWord>|<LibraryInstance>
    #
    # @param ContainerFile: The Dsc file full path
    #
    def GenLibraryClasses(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_LIBRARY_CLASSES)
        LibraryClasses = {}
        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, MODEL_EFI_LIBRARY_CLASS, self.FileID)

        #
        # Get all LibraryClasses
        #
        RecordSet = QueryDscItem(self.TblDsc, MODEL_EFI_LIBRARY_CLASS, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, TAB_LIBRARY_CLASSES, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        MergeArches(LibraryClasses, GetLibraryClass([NewItem, IncludeFile[4]], Filename, self.WorkspaceDir, -1), Arch)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (LibClassName, LibClassIns, SupModelList) = GetLibraryClass([Record[0], Record[4]], ContainerFile, self.WorkspaceDir, Record[2])
                    MergeArches(LibraryClasses, (LibClassName, LibClassIns, SupModelList), Arch)
                    #
                    # Update to Database
                    #
                    if self.IsToDatabase:
                        SqlCommand = """update %s set Value1 = '%s', Value2 = '%s', Value3 = '%s'
                                        where ID = %s""" % (self.TblDsc.Table, ConvertToSqlString2(LibClassName), ConvertToSqlString2(LibClassIns), ConvertToSqlString2(SupModelList), Record[3])
                        self.TblDsc.Exec(SqlCommand)

        for Key in LibraryClasses.keys():
            Library = PlatformLibraryClass()
            Library.Name = Key[0]
            Library.FilePath = NormPath(Key[1])
            Library.SupModuleList = GetSplitValueList(Key[2])
            Library.SupArchList = LibraryClasses[Key]
            self.Platform.LibraryClasses.LibraryList.append(Library)

    ## Gen Pcds
    #
    # Gen Pcd of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<Type>|<MaximumDatumSize>]
    #
    # @param Type:           The type of Pcd
    # @param ContainerFile:  The file which describes the pcd, used for error report
    #
    def GenPcds(self, Type='', ContainerFile=''):
        Pcds = {}
        if Type == DataType.TAB_PCDS_PATCHABLE_IN_MODULE:
            Model = MODEL_PCD_PATCHABLE_IN_MODULE
        elif Type == DataType.TAB_PCDS_FIXED_AT_BUILD:
            Model = MODEL_PCD_FIXED_AT_BUILD
        else:
            pass
        EdkLogger.debug(2, "Generate %s ..." % Type)

        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, Model, self.FileID)

        #
        # Get all Pcds
        #
        RecordSet = QueryDscItem(self.TblDsc, Model, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, Type, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        (TokenName, TokenGuidCName, Value, DatumType, MaxDatumSize, Type) = GetPcd(NewItem, Type, Filename, -1)
                        MergeArches(Pcds, (TokenName, TokenGuidCName, Value, DatumType, MaxDatumSize, Type), Arch)
                        self.PcdToken[Record[3]] = (TokenGuidCName, TokenName)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (TokenName, TokenGuidCName, Value, DatumType, MaxDatumSize, Type) = GetPcd(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenName, TokenGuidCName, Value, DatumType, MaxDatumSize, Type), Arch)
                    self.PcdToken[Record[3]] = (TokenGuidCName, TokenName)

        for Key in Pcds:
            Pcd = PcdClass(Key[0], '', Key[1], Key[3], Key[4], Key[2], Key[5], [], {}, [])
            Pcd.SupArchList = Pcds[Key]
            self.Platform.DynamicPcdBuildDefinitions.append(Pcd)

    ## Gen FeatureFlagPcds
    #
    # Gen FeatureFlagPcds of Dsc file as <PcdTokenSpaceGuidCName>.<TokenCName>|TRUE/FALSE
    #
    # @param Type:           The type of Pcd
    # @param ContainerFile:  The file which describes the pcd, used for error report
    #
    def GenFeatureFlagPcds(self, Type='', ContainerFile=''):
        Pcds = {}
        if Type == DataType.TAB_PCDS_FEATURE_FLAG:
            Model = MODEL_PCD_FEATURE_FLAG
        else:
            pass
        EdkLogger.debug(2, "Generate %s ..." % Type)

        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, Model, self.FileID)

        #
        # Get all FeatureFlagPcds
        #
        RecordSet = QueryDscItem(self.TblDsc, Model, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, Type, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        (TokenName, TokenGuidCName, Value, Type) = GetFeatureFlagPcd(NewItem, Type, Filename, -1)
                        MergeArches(Pcds, (TokenName, TokenGuidCName, Value, Type), Arch)
                        self.PcdToken[Record[3]] = (TokenGuidCName, TokenName)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (TokenName, TokenGuidCName, Value, Type) = GetFeatureFlagPcd(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Pcds, (TokenName, TokenGuidCName, Value, Type), Arch)
                    self.PcdToken[Record[3]] = (TokenGuidCName, TokenName)

        for Key in Pcds:
            Pcd = PcdClass(Key[0], '', Key[1], '', '', Key[2], Key[3], [], {}, [])
            Pcd.SupArchList = Pcds[Key]
            self.Platform.DynamicPcdBuildDefinitions.append(Pcd)

    ## Gen DynamicDefaultPcds
    #
    # Gen DynamicDefaultPcds of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<Value>[|<DatumTyp>[|<MaxDatumSize>]]
    #
    # @param Type:           The type of Pcd
    # @param ContainerFile:  The file which describes the pcd, used for error report
    #
    def GenDynamicDefaultPcds(self, Type='', ContainerFile=''):
        Pcds = {}
        SkuInfoList = {}
        if Type == DataType.TAB_PCDS_DYNAMIC_DEFAULT:
            Model = MODEL_PCD_DYNAMIC_DEFAULT
        elif Type == DataType.TAB_PCDS_DYNAMIC_EX_DEFAULT:
            Model = MODEL_PCD_DYNAMIC_EX_DEFAULT
        else:
            pass
        EdkLogger.debug(2, "Generate %s ..." % Type)

        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, Model, self.FileID)

        #
        # Get all DynamicDefaultPcds
        #
        RecordSet = QueryDscItem(self.TblDsc, Model, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, Type, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        (K1, K2, K3, K4, K5, K6) = GetDynamicDefaultPcd(NewItem, Type, Filename, -1)
                        MergeArches(Pcds, (K1, K2, K3, K4, K5, K6, IncludeFile[4]), Arch)
                        self.PcdToken[Record[3]] = (K2, K1)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (K1, K2, K3, K4, K5, K6) = GetDynamicDefaultPcd(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Pcds, (K1, K2, K3, K4, K5, K6, Record[4]), Arch)
                    self.PcdToken[Record[3]] = (K2, K1)

        for Key in Pcds:
            (Status, SkuInfoList) = self.GenSkuInfoList(Key[6], self.Platform.SkuInfos.SkuInfoList, '', '', '', '', '', Key[2])
            if Status == False:
                ErrorMsg = "The SKUID '%s' used in section '%s' is not defined in section [SkuIds]" % (SkuInfoList, Type)
                EdkLogger.error("DSC File Parser", PARSER_ERROR, ErrorMsg, ContainerFile, RaiseError=EdkLogger.IsRaiseError)
            Pcd = PcdClass(Key[0], '', Key[1], Key[3], Key[4], Key[2], Key[5], [], SkuInfoList, [])
            Pcd.SupArchList = Pcds[Key]
            self.Platform.DynamicPcdBuildDefinitions.append(Pcd)

    ## Gen DynamicHiiPcds
    #
    # Gen DynamicHiiPcds of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<String>|<VariableGuidCName>|<VariableOffset>[|<DefaultValue>[|<MaximumDatumSize>]]
    #
    # @param Type:           The type of Pcd
    # @param ContainerFile:  The file which describes the pcd, used for error report
    #
    def GenDynamicHiiPcds(self, Type='', ContainerFile=''):
        Pcds = {}
        SkuInfoList = {}
        if Type == DataType.TAB_PCDS_DYNAMIC_HII:
            Model = MODEL_PCD_DYNAMIC_HII
        elif Type == DataType.TAB_PCDS_DYNAMIC_EX_HII:
            Model = MODEL_PCD_DYNAMIC_EX_HII
        else:
            pass
        EdkLogger.debug(2, "Generate %s ..." % Type)

        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, Model, self.FileID)

        #
        # Get all DynamicHiiPcds
        #
        RecordSet = QueryDscItem(self.TblDsc, Model, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, Type, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        (K1, K2, K3, K4, K5, K6, K7, K8) = GetDynamicHiiPcd(NewItem, Type, Filename, -1)
                        MergeArches(Pcds, (K1, K2, K3, K4, K5, K6, K7, K8, IncludeFile[4]), Arch)
                        self.PcdToken[Record[3]] = (K2, K1)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (K1, K2, K3, K4, K5, K6, K7, K8) = GetDynamicHiiPcd(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Pcds, (K1, K2, K3, K4, K5, K6, K7, K8, Record[4]), Arch)
                    self.PcdToken[Record[3]] = (K2, K1)

        for Key in Pcds:
            (Status, SkuInfoList) = self.GenSkuInfoList(Key[8], self.Platform.SkuInfos.SkuInfoList, Key[2], Key[3], Key[4], Key[5], '', '')
            if Status == False:
                ErrorMsg = "The SKUID '%s' used in section '%s' is not defined in section [SkuIds]" % (SkuInfoList, Type)
                EdkLogger.error("DSC File Parser", PARSER_ERROR, ErrorMsg, ContainerFile, RaiseError=EdkLogger.IsRaiseError)
            Pcd = PcdClass(Key[0], '', Key[1], '', Key[6], Key[5], Key[7], [], SkuInfoList, [])
            Pcd.SupArchList = Pcds[Key]
            self.Platform.DynamicPcdBuildDefinitions.append(Pcd)

    ## Gen DynamicVpdPcds
    #
    # Gen DynamicVpdPcds of Dsc as <PcdTokenSpaceGuidCName>.<TokenCName>|<VpdOffset>[|<MaximumDatumSize>]
    #
    # @param Type:           The type of Pcd
    # @param ContainerFile:  The file which describes the pcd, used for error report
    #
    def GenDynamicVpdPcds(self, Type='', ContainerFile=''):
        Pcds = {}
        SkuInfoList = {}
        if Type == DataType.TAB_PCDS_DYNAMIC_VPD:
            Model = MODEL_PCD_DYNAMIC_VPD
        elif Type == DataType.TAB_PCDS_DYNAMIC_EX_VPD:
            Model = MODEL_PCD_DYNAMIC_EX_VPD
        else:
            pass
        EdkLogger.debug(2, "Generate %s ..." % Type)

        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, Model, self.FileID)

        #
        # Get all DynamicVpdPcds
        #
        RecordSet = QueryDscItem(self.TblDsc, Model, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, Type, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        (K1, K2, K3, K4, K5) = GetDynamicVpdPcd(NewItem, Type, Filename, -1)
                        MergeArches(Pcds, (K1, K2, K3, K4, K5, IncludeFile[4]), Arch)
                        self.PcdToken[Record[3]] = (K2, K1)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    (K1, K2, K3, K4, K5) = GetDynamicVpdPcd(Record[0], Type, ContainerFile, Record[2])
                    MergeArches(Pcds, (K1, K2, K3, K4, K5, Record[4]), Arch)
                    self.PcdToken[Record[3]] = (K2, K1)

        for Key in Pcds:
            (Status, SkuInfoList) = self.GenSkuInfoList(Key[5], self.Platform.SkuInfos.SkuInfoList, '', '', '', '', Key[2], '')
            if Status == False:
                ErrorMsg = "The SKUID '%s' used in section '%s' is not defined in section [SkuIds]" % (SkuInfoList, Type)
                EdkLogger.error("DSC File Parser", PARSER_ERROR, ErrorMsg, ContainerFile, RaiseError=EdkLogger.IsRaiseError)
            Pcd = PcdClass(Key[0], '', Key[1], '', Key[3], '', Key[4], [], SkuInfoList, [])
            Pcd.SupArchList = Pcds[Key]
            self.Platform.DynamicPcdBuildDefinitions.append(Pcd)


    ## Get Component
    #
    # Get Component section defined in Dsc file
    #
    # @param ContainerFile:  The file which describes the Components, used for error report
    #
    # @retval PlatformModuleClass() A instance for PlatformModuleClass
    #
    def GenComponents(self, ContainerFile):
        EdkLogger.debug(2, "Generate %s ..." % TAB_COMPONENTS)
        Components = sdict()
        #
        # Get all include files
        #
        IncludeFiles = QueryDscItem(self.TblDsc, MODEL_META_DATA_INCLUDE, MODEL_META_DATA_COMPONENT, self.FileID)

        #
        # Get all Components
        #
        RecordSet = QueryDscItem(self.TblDsc, MODEL_META_DATA_COMPONENT, -1, self.FileID)

        #
        # Go through each arch
        #
        for Arch in DataType.ARCH_LIST:
            for IncludeFile in IncludeFiles:
                if IncludeFile[1] == Arch or IncludeFile[1] == TAB_ARCH_COMMON.upper():
                    Filename = CheckFileExist(self.WorkspaceDir, IncludeFile[0], ContainerFile, TAB_COMPONENTS, '', IncludeFile[2])
                    for NewItem in open(Filename, 'r').readlines():
                        if CleanString(NewItem) == '':
                            continue
                        NewItems = []
                        GetComponents(open(Filename, 'r').read(), TAB_COMPONENTS, NewItems, TAB_COMMENT_SPLIT)
                        for NewComponent in NewItems:
                            MergeArches(Components, self.GenComponent(NewComponent, Filename), Arch)

            for Record in RecordSet:
                if Record[1] == Arch or Record[1] == TAB_ARCH_COMMON.upper():
                    Lib, Bo, Pcd = [], [], []

                    SubLibSet = QueryDscItem(self.TblDsc, MODEL_EFI_LIBRARY_CLASS, Record[3], self.FileID)
                    for SubLib in SubLibSet:
                        Lib.append(TAB_VALUE_SPLIT.join([SubLib[0], SubLib[4]]))

                    SubBoSet = QueryDscItem(self.TblDsc, MODEL_META_DATA_BUILD_OPTION, Record[3], self.FileID)
                    for SubBo in SubBoSet:
                        Bo.append(SubBo[0])

                    SubPcdSet1 = QueryDscItem(self.TblDsc, MODEL_PCD_FIXED_AT_BUILD, Record[3], self.FileID)
                    SubPcdSet2 = QueryDscItem(self.TblDsc, MODEL_PCD_PATCHABLE_IN_MODULE, Record[3], self.FileID)
                    SubPcdSet3 = QueryDscItem(self.TblDsc, MODEL_PCD_FEATURE_FLAG, Record[3], self.FileID)
                    SubPcdSet4 = QueryDscItem(self.TblDsc, MODEL_PCD_DYNAMIC_EX_DEFAULT, Record[3], self.FileID)
                    SubPcdSet5 = QueryDscItem(self.TblDsc, MODEL_PCD_DYNAMIC_DEFAULT, Record[3], self.FileID)
                    for SubPcd in SubPcdSet1:
                        Pcd.append([DataType.TAB_PCDS_FIXED_AT_BUILD, SubPcd[0], SubPcd[3]])
                    for SubPcd in SubPcdSet2:
                        Pcd.append([DataType.TAB_PCDS_PATCHABLE_IN_MODULE, SubPcd[0], SubPcd[3]])
                    for SubPcd in SubPcdSet3:
                        Pcd.append([DataType.TAB_PCDS_FEATURE_FLAG, SubPcd[0], SubPcd[3]])
                    for SubPcd in SubPcdSet4:
                        Pcd.append([DataType.TAB_PCDS_DYNAMIC_EX, SubPcd[0], SubPcd[3]])
                    for SubPcd in SubPcdSet5:
                        Pcd.append([DataType.TAB_PCDS_DYNAMIC, SubPcd[0], SubPcd[3]])
                    Item = [Record[0], Lib, Bo, Pcd]
                    MergeArches(Components, self.GenComponent(Item, ContainerFile), Arch)

        for Key in Components.keys():
            Key.SupArchList = Components[Key]
            self.Platform.Modules.ModuleList.append(Key)

    ## Get Component
    #
    # Get Component section defined in Dsc file
    #
    # @param Item:           Contents includes a component block
    # @param ContainerFile:  The file which describes the library class, used for error report
    #
    # @retval PlatformModuleClass() A instance for PlatformModuleClass
    #
    def GenComponent(self, Item, ContainerFile, LineNo= -1):
        (InfFilename, ExecFilename) = GetExec(Item[0])
        LibraryClasses = Item[1]
        BuildOptions = Item[2]
        Pcds = Item[3]
        Component = PlatformModuleClass()
        Component.FilePath = NormPath(InfFilename)
        Component.ExecFilePath = NormPath(ExecFilename)
        CheckFileType(Component.FilePath, '.Inf', ContainerFile, 'component name', Item[0], LineNo)
        CheckFileExist(self.WorkspaceDir, Component.FilePath, ContainerFile, 'component', Item[0], LineNo)
        for Lib in LibraryClasses:
            List = GetSplitValueList(Lib)
            if len(List) != 2:
                RaiseParserError(Lib, 'LibraryClasses', ContainerFile, '<ClassName>|<InfFilename>')
            LibName = List[0]
            LibFile = NormPath(List[1])
            if LibName == "" or LibName == "NULL":
                LibName = "NULL%d" % self._NullClassIndex
                self._NullClassIndex += 1
            CheckFileType(List[1], '.Inf', ContainerFile, 'library instance of component ', Lib, LineNo)
            CheckFileExist(self.WorkspaceDir, LibFile, ContainerFile, 'library instance of component', Lib, LineNo)
            Component.LibraryClasses.LibraryList.append(PlatformLibraryClass(LibName, LibFile))
        for BuildOption in BuildOptions:
            Key = GetBuildOption(BuildOption, ContainerFile)
            Component.ModuleSaBuildOption.BuildOptionList.append(BuildOptionClass(Key[0], Key[1], Key[2]))
        for Pcd in Pcds:
            Type = Pcd[0]
            List = GetSplitValueList(Pcd[1])
            PcdId = Pcd[2]

            TokenInfo = None
            #
            # For FeatureFlag
            #
            if Type == DataType.TAB_PCDS_FEATURE_FLAG:
                if len(List) != 2:
                    RaiseParserError(Pcd[1], 'Components', ContainerFile, '<PcdTokenSpaceGuidCName>.<PcdTokenName>|TRUE/FALSE')

                CheckPcdTokenInfo(List[0], 'Components', ContainerFile)
                TokenInfo = GetSplitValueList(List[0], DataType.TAB_SPLIT)
                Component.PcdBuildDefinitions.append(PcdClass(TokenInfo[1], '', TokenInfo[0], '', '', List[1], Type, [], {}, []))
            #
            # For FixedAtBuild or PatchableInModule
            #
            if Type == DataType.TAB_PCDS_FIXED_AT_BUILD or Type == DataType.TAB_PCDS_PATCHABLE_IN_MODULE:
                List.append('')
                if len(List) != 3 and len(List) != 4:
                    RaiseParserError(Pcd[1], 'Components', ContainerFile, '<PcdTokenSpaceGuidCName>.<PcdTokenName>|<Value>[|<MaxDatumSize>]')

                CheckPcdTokenInfo(List[0], 'Components', ContainerFile)
                TokenInfo = GetSplitValueList(List[0], DataType.TAB_SPLIT)
                Component.PcdBuildDefinitions.append(PcdClass(TokenInfo[1], '', TokenInfo[0], '', List[2], List[1], Type, [], {}, []))

            #
            # For Dynamic or DynamicEx
            #
            if Type == DataType.TAB_PCDS_DYNAMIC or Type == DataType.TAB_PCDS_DYNAMIC_EX:
                if len(List) != 1:
                    RaiseParserError(Pcd[1], 'Components', ContainerFile, '<PcdTokenSpaceGuidCName>.<PcdTokenName>')

                CheckPcdTokenInfo(List[0], 'Components', ContainerFile)
                TokenInfo = GetSplitValueList(List[0], DataType.TAB_SPLIT)
                Component.PcdBuildDefinitions.append(PcdClass(TokenInfo[1], '', TokenInfo[0], '', '', '', Type, [], {}, []))

            #
            # Add to PcdToken
            #
            self.PcdToken[PcdId] = (TokenInfo[0], TokenInfo[1])

        return Component
    #End of GenComponent

    ## Gen SkuInfoList
    #
    # Gen SkuInfoList section defined in Dsc file
    #
    # @param SkuNameList:      Input value for SkuNameList
    # @param SkuInfo:          Input value for SkuInfo
    # @param VariableName:     Input value for VariableName
    # @param VariableGuid:     Input value for VariableGuid
    # @param VariableOffset:   Input value for VariableOffset
    # @param HiiDefaultValue:  Input value for HiiDefaultValue
    # @param VpdOffset:        Input value for VpdOffset
    # @param DefaultValue:     Input value for DefaultValue
    #
    # @retval (False, SkuName)     Not found in section SkuId Dsc file
    # @retval (True, SkuInfoList)  Found in section SkuId of Dsc file
    #
    def GenSkuInfoList(self, SkuNameList, SkuInfo, VariableName='', VariableGuid='', VariableOffset='', HiiDefaultValue='', VpdOffset='', DefaultValue=''):
        SkuNameList = GetSplitValueList(SkuNameList)
        if SkuNameList == None or SkuNameList == [] or SkuNameList == ['']:
            SkuNameList = ['DEFAULT']
        SkuInfoList = {}
        for Item in SkuNameList:
            if Item not in SkuInfo:
                return False, Item
            Sku = SkuInfoClass(Item, SkuInfo[Item], VariableName, VariableGuid, VariableOffset, HiiDefaultValue, VpdOffset, DefaultValue)
            SkuInfoList[Item] = Sku

        return True, SkuInfoList

    ## Parse Include statement
    #
    # Get include file path
    #
    # 1. Insert a record into TblFile ???
    # 2. Insert a record into TblDsc
    # Value1: IncludeFilePath
    #
    # @param LineValue:  The line of incude statement
    def ParseInclude(self, LineValue, StartLine, Table, FileID, Filename, SectionName, Model, Arch):
        EdkLogger.debug(EdkLogger.DEBUG_2, "!include statement '%s' found in section %s" % (LineValue, SectionName))
        SectionModel = Section[SectionName.upper()]
        IncludeFile = CleanString(LineValue[LineValue.upper().find(DataType.TAB_INCLUDE.upper() + ' ') + len(DataType.TAB_INCLUDE + ' ') : ])
        Table.Insert(Model, IncludeFile, '', '', Arch, SectionModel, FileID, StartLine, -1, StartLine, -1, 0)

    ## Parse DEFINE statement
    #
    # Get DEFINE macros
    #
    # 1. Insert a record into TblDsc
    # Value1: Macro Name
    # Value2: Macro Value
    #
    def ParseDefine(self, LineValue, StartLine, Table, FileID, Filename, SectionName, Model, Arch):
        EdkLogger.debug(EdkLogger.DEBUG_2, "DEFINE statement '%s' found in section %s" % (LineValue, SectionName))
        SectionModel = Section[SectionName.upper()]
        Define = GetSplitValueList(CleanString(LineValue[LineValue.upper().find(DataType.TAB_DEFINE.upper() + ' ') + len(DataType.TAB_DEFINE + ' ') : ]), TAB_EQUAL_SPLIT, 1)
        Table.Insert(Model, Define[0], Define[1], '', Arch, SectionModel, FileID, StartLine, -1, StartLine, -1, 0)

    ## Parse Defines section
    #
    # Get one item in defines section
    #
    # Value1: Item Name
    # Value2: Item Value
    #
    def ParseDefinesSection(self, LineValue, StartLine, Table, FileID, Filename, SectionName, Model, Arch):
        EdkLogger.debug(EdkLogger.DEBUG_2, "Parse '%s' found in section %s" % (LineValue, SectionName))
        Defines = GetSplitValueList(LineValue, TAB_EQUAL_SPLIT, 1)
        if len(Defines) != 2:
            RaiseParserError(LineValue, SectionName, Filename, '', StartLine)
        self.TblDsc.Insert(Model, Defines[0], Defines[1], '', Arch, -1, FileID, StartLine, -1, StartLine, -1, 0)

    ## Insert conditional statements
    #
    # Pop an item from IfDefList
    # Insert conditional statements to database
    #
    # @param Filename:   Path of parsing file
    # @param IfDefList:  A list stored current conditional statements
    # @param EndLine:    The end line no
    # @param ArchList:   Support arch list
    #
    def InsertConditionalStatement(self, Filename, FileID, BelongsToItem, IfDefList, EndLine, ArchList):
        (Value1, Value2, Value3, Model, StartColumn, EndColumn, Enabled) = ('', '', '', -1, -1, -1, 0)
        if IfDefList == []:
            ErrorMsg = 'Not suited conditional statement in file %s' % Filename
            EdkLogger.error("DSC File Parser", PARSER_ERROR, ErrorMsg, Filename, RaiseError=EdkLogger.IsRaiseError)
        else:
            #
            # Get New Dsc item ID
            #
            DscID = self.TblDsc.GetCount() + 1

            #
            # Pop the conditional statements which is closed
            #
            PreviousIf = IfDefList.pop()
            EdkLogger.debug(EdkLogger.DEBUG_5, 'Previous IfDef: ' + str(PreviousIf))

            #
            # !ifdef and !ifndef
            #
            if PreviousIf[2] in (MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF, MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF):
                Value1 = PreviousIf[0]
                Model = PreviousIf[2]
                self.TblDsc.Insert(Model, Value1, Value2, Value3, ArchList, BelongsToItem, self.FileID, PreviousIf[1], StartColumn, EndLine, EndColumn, Enabled)
            #
            # !if and !elseif
            #
            elif PreviousIf[2] in (MODEL_META_DATA_CONDITIONAL_STATEMENT_IF, Model):
                List = PreviousIf[0].split(' ')
                Value1, Value2, Value3 = '', '==', '0'
                if len(List) == 3:
                    Value1 = List[0]
                    Value2 = List[1]
                    Value3 = List[2]
                    Value3 = SplitString(Value3)
                if len(List) == 1:
                    Value1 = List[0]
                Model = PreviousIf[2]
                self.TblDsc.Insert(Model, Value1, Value2, Value3, ArchList, BelongsToItem, self.FileID, PreviousIf[1], StartColumn, EndLine, EndColumn, Enabled)
            #
            # !else
            #
            elif PreviousIf[2] in (MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE, Model):
                Value1 = PreviousIf[0].strip()
                Model = PreviousIf[2]
                self.TblDsc.Insert(Model, Value1, Value2, Value3, ArchList, BelongsToItem, self.FileID, PreviousIf[1], StartColumn, EndLine, EndColumn, Enabled)

    ## Load Dsc file
    #
    # Load the file if it exists
    #
    # @param Filename:  Input value for filename of Dsc file
    #
    def LoadDscFile(self, Filename):
        #
        # Insert a record for file
        #
        Filename = NormPath(Filename)
        self.Identification.FileFullPath = Filename
        (self.Identification.FileRelativePath, self.Identification.FileName) = os.path.split(Filename)
        self.FileID = self.TblFile.InsertFile(Filename, MODEL_FILE_DSC)

        #
        # Init DscTable
        #
        #self.TblDsc.Table = "Dsc%s" % FileID
        #self.TblDsc.Create()

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
                #
                # Insert items data of previous section
                #
                self.InsertSectionItemsIntoDatabase(self.FileID, Filename, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList)
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
        self.InsertSectionItemsIntoDatabase(self.FileID, Filename, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList)

        #
        # Parse conditional statements
        #
        self.ParseConditionalStatement()

        #
        # Replace all DEFINE macros with its actual values
        #
        #ParseDefineMacro2(self.TblDsc, self.RecordSet, GlobalData.gGlobalDefines)
        ParseDefineMacro(self.TblDsc, GlobalData.gGlobalDefines)


    ## ParseConditionalStatement
    #
    # Search all conditional statement and disable no match records
    #
    def ParseConditionalStatement(self):
        #
        # Disabled all !if/!elif/!ifdef statements without DEFINE
        #
        SqlCommand = """select A.StartLine, A.EndLine from %s as A
                        where A.Model in (%s, %s, %s)
                        and A.Enabled = 0
                        and A.BelongsToFile = %s
                        and A.Value1 not in (select B.Value1 from %s as B
                                             where B.Model = %s
                                             and B.Enabled = 0
                                             and A.StartLine > B.StartLine
                                             and A.Arch = B.Arch
                                             and A.BelongsToItem = B.BelongsToItem
                                             and A.BelongsToFile = B.BelongsToFile) """ % \
                        (self.TblDsc.Table, \
                         MODEL_META_DATA_CONDITIONAL_STATEMENT_IF, MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE, MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF, \
                         self.FileID, \
                         self.TblDsc.Table, \
                         MODEL_META_DATA_DEFINE)
        RecordSet = self.TblDsc.Exec(SqlCommand)
        for Record in RecordSet:
            SqlCommand = """Update %s set Enabled = -1 where StartLine >= %s and EndLine <= %s""" % (self.TblDsc.Table, Record[0], Record[1])
            self.TblDsc.Exec(SqlCommand)

        #
        # Disabled !ifndef with DEFINE
        #
        SqlCommand = """select A.StartLine, A.EndLine from %s as A
                        where A.Model = %s
                        and A.Enabled = 0
                        and A.BelongsToFile = %s
                        and A.Value1 in (select B.Value1 from %s as B
                                         where B.Model = %s
                                         and B.Enabled = 0
                                         and A.StartLine > B.StartLine
                                         and A.Arch = B.Arch
                                         and A.BelongsToItem = B.BelongsToItem
                                         and A.BelongsToFile = B.BelongsToFile)""" % \
                        (self.TblDsc.Table, \
                         MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF, \
                         self.FileID, \
                         self.TblDsc.Table, \
                         MODEL_META_DATA_DEFINE)
        RecordSet = self.TblDsc.Exec(SqlCommand)
        for Record in RecordSet:
            SqlCommand = """Update %s set Enabled = -1 where StartLine >= %s and EndLine <= %s""" % (self.TblDsc.Table, Record[0], Record[1])
            EdkLogger.debug(4, "SqlCommand: %s" % SqlCommand)
            self.Cur.execute(SqlCommand)

        #
        # Disabled !if, !elif and !else with un-match value
        #
        SqlCommand = """select A.Model, A.Value1, A.Value2, A.Value3, A.StartLine, A.EndLine, B.Value2 from %s as A join %s as B
                        where A.Model in (%s, %s)
                        and A.Enabled = 0
                        and A.BelongsToFile = %s
                        and B.Enabled = 0
                        and B.Model = %s
                        and A.Value1 = B.Value1
                        and A.StartLine > B.StartLine
                        and A.BelongsToItem = B.BelongsToItem
                        and A.BelongsToFile = B.BelongsToFile""" % \
                        (self.TblDsc.Table, self.TblDsc.Table, \
                         MODEL_META_DATA_CONDITIONAL_STATEMENT_IF, MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE, \
                         self.FileID, MODEL_META_DATA_DEFINE)
        RecordSet = self.TblDsc.Exec(SqlCommand)
        DisabledList = []
        for Record in RecordSet:
            if Record[0] == MODEL_META_DATA_CONDITIONAL_STATEMENT_IF:
                if not self.Compare(Record[6], Record[2], Record[3]):
                    SqlCommand = """Update %s set Enabled = -1 where StartLine >= %s and EndLine <= %s""" % (self.TblDsc.Table, Record[4], Record[5])
                    self.TblDsc.Exec(SqlCommand)
                else:
                    DisabledList.append(Record[1])
                continue
            if Record[0] == MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE and Record[1] in DisabledList:
                SqlCommand = """Update %s set Enabled = -1 where StartLine >= %s and EndLine <= %s""" % (self.TblDsc.Table, Record[4], Record[5])
                self.TblDsc.Exec(SqlCommand)

    ## Compare
    #
    # Compare two values
    # @param Value1:
    # @param CompareType:
    # @param Value2:
    #
    def Compare(self, Value1, CompareType, Value2):
        Command = """Value1 %s Value2""" % CompareType
        return eval(Command)

    ## First time to insert records to database
    #
    # Insert item data of a section to database
    # @param FileID:           The ID of belonging file
    # @param Filename:         The name of belonging file
    # @param CurrentSection:   The name of currect section
    # @param SectionItemList:  A list of items of the section
    # @param ArchList:         A list of arches
    # @param ThirdList:        A list of third parameters, ModuleType for LibraryClass and SkuId for Dynamic Pcds
    # @param IfDefList:        A list of all conditional statements
    #
    def InsertSectionItemsIntoDatabase(self, FileID, Filename, CurrentSection, SectionItemList, ArchList, ThirdList, IfDefList):
        #
        # Insert each item data of a section
        #
        for Index in range(0, len(ArchList)):
            Arch = ArchList[Index]
            Third = ThirdList[Index]
            if Arch == '':
                Arch = TAB_ARCH_COMMON.upper()

            Model = Section[CurrentSection.upper()]
            #Records = self.RecordSet[Model]

            for SectionItem in SectionItemList:
                BelongsToItem, EndLine, EndColumn = -1, -1, -1
                LineValue, StartLine, EndLine = SectionItem[0], SectionItem[1], SectionItem[1]


                EdkLogger.debug(4, "Parsing %s ..." % LineValue)
                #
                # Parse '!ifdef'
                #
                if LineValue.upper().find(TAB_IF_DEF.upper()) > -1:
                    IfDefList.append((LineValue[len(TAB_IF_N_DEF):].strip(), StartLine, MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF))
                    continue

                #
                # Parse '!ifndef'
                #
                if LineValue.upper().find(TAB_IF_N_DEF.upper()) > -1:
                    IfDefList.append((LineValue[len(TAB_IF_N_DEF):].strip(), StartLine, MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF))
                    continue

                #
                # Parse '!endif'
                #
                if LineValue.upper().find(TAB_END_IF.upper()) > -1:
                    self.InsertConditionalStatement(Filename, FileID, Model, IfDefList, StartLine, Arch)
                    continue
                #
                # Parse '!if'
                #
                if LineValue.upper().find(TAB_IF.upper()) > -1:
                    IfDefList.append((LineValue[len(TAB_IF):].strip(), StartLine, MODEL_META_DATA_CONDITIONAL_STATEMENT_IF))
                    continue

                #
                # Parse '!elseif'
                #
                if LineValue.upper().find(TAB_ELSE_IF.upper()) > -1:
                    self.InsertConditionalStatement(Filename, FileID, Model, IfDefList, StartLine - 1, Arch)
                    IfDefList.append((LineValue[len(TAB_ELSE_IF):].strip(), StartLine, MODEL_META_DATA_CONDITIONAL_STATEMENT_IF))
                    continue

                #
                # Parse '!else'
                #
                if LineValue.upper().find(TAB_ELSE.upper()) > -1:
                    Key = IfDefList[-1][0].split(' ' , 1)[0].strip()
                    self.InsertConditionalStatement(Filename, FileID, Model, IfDefList, StartLine, Arch)
                    IfDefList.append((Key, StartLine, MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE))
                    continue

                #
                # Parse !include statement first
                #
                if LineValue.upper().find(DataType.TAB_INCLUDE.upper() + ' ') > -1:
                    self.ParseInclude(LineValue, StartLine, self.TblDsc, FileID, Filename, CurrentSection, MODEL_META_DATA_INCLUDE, Arch)
                    continue

                #
                # And then parse DEFINE statement
                #
                if LineValue.upper().find(DataType.TAB_DEFINE.upper() + ' ') > -1:
                    self.ParseDefine(LineValue, StartLine, self.TblDsc, FileID, Filename, CurrentSection, MODEL_META_DATA_DEFINE, Arch)
                    continue

                #
                # At last parse other sections
                #
                if CurrentSection == TAB_LIBRARY_CLASSES or CurrentSection in TAB_PCD_DYNAMIC_TYPE_LIST or CurrentSection in TAB_PCD_DYNAMIC_EX_TYPE_LIST:
                    ID = self.TblDsc.Insert(Model, LineValue, Third, '', Arch, -1, FileID, StartLine, -1, StartLine, -1, 0)
                    #Records.append([LineValue, Arch, StartLine, ID, Third])
                    continue
                elif CurrentSection != TAB_COMPONENTS:
                    ID = self.TblDsc.Insert(Model, LineValue, '', '', Arch, -1, FileID, StartLine, -1, StartLine, -1, 0)
                    #Records.append([LineValue, Arch, StartLine, ID, Third])
                    continue

            #
            # Parse COMPONENT section
            #
            if CurrentSection == TAB_COMPONENTS:
                Components = []
                GetComponent(SectionItemList, Components)
                for Component in Components:
                    EdkLogger.debug(4, "Parsing component %s ..." % Component)
                    DscItmeID = self.TblDsc.Insert(MODEL_META_DATA_COMPONENT, Component[0], '', '', Arch, -1, FileID, StartLine, -1, StartLine, -1, 0)
                    for Item in Component[1]:
                        List = GetSplitValueList(Item, MaxSplit=2)
                        LibName, LibIns = '', ''
                        if len(List) == 2:
                            LibName = List[0]
                            LibIns = List[1]
                        else:
                            LibName = List[0]
                        self.TblDsc.Insert(MODEL_EFI_LIBRARY_CLASS, LibName, LibIns, '', Arch, DscItmeID, FileID, StartLine, -1, StartLine, -1, 0)
                    for Item in Component[2]:
                        self.TblDsc.Insert(MODEL_META_DATA_BUILD_OPTION, Item, '', '', Arch, DscItmeID, FileID, StartLine, -1, StartLine, -1, 0)
                    for Item in Component[3]:
                        Model = Section[Item[0].upper()]
                        self.TblDsc.Insert(Model, Item[1], '', '', Arch, DscItmeID, FileID, StartLine, -1, StartLine, -1, 0)

    ## Show detailed information of Dsc
    #
    # Print all members and their values of Dsc class
    #
    def ShowDsc(self):
        print TAB_SECTION_START + TAB_INF_DEFINES + TAB_SECTION_END
        printDict(self.Defines.DefinesDictionary)

        for Key in self.KeyList:
            for Arch in DataType.ARCH_LIST_FULL:
                Command = "printList(TAB_SECTION_START + '" + \
                                    Key + DataType.TAB_SPLIT + Arch + \
                                    "' + TAB_SECTION_END, self.Contents[arch]." + Key + ')'
                eval(Command)

    ## Show detailed information of Platform
    #
    # Print all members and their values of Platform class
    #
    def ShowPlatform(self):
        M = self.Platform
        for Arch in M.Header.keys():
            print '\nArch =', Arch
            print 'Filename =', M.Header[Arch].FileName
            print 'FullPath =', M.Header[Arch].FullPath
            print 'BaseName =', M.Header[Arch].Name
            print 'Guid =', M.Header[Arch].Guid
            print 'Version =', M.Header[Arch].Version
            print 'DscSpecification =', M.Header[Arch].DscSpecification
            print 'SkuId =', M.Header[Arch].SkuIdName
            print 'SupArchList =', M.Header[Arch].SupArchList
            print 'BuildTargets =', M.Header[Arch].BuildTargets
            print 'OutputDirectory =', M.Header[Arch].OutputDirectory
            print 'BuildNumber =', M.Header[Arch].BuildNumber
            print 'MakefileName =', M.Header[Arch].MakefileName
            print 'BsBaseAddress =', M.Header[Arch].BsBaseAddress
            print 'RtBaseAddress =', M.Header[Arch].RtBaseAddress
            print 'Define =', M.Header[Arch].Define
        print 'Fdf =', M.FlashDefinitionFile.FilePath
        print '\nBuildOptions =', M.BuildOptions, M.BuildOptions.IncludeFiles
        for Item in M.BuildOptions.BuildOptionList:
            print '\t', 'ToolChainFamily =', Item.ToolChainFamily, 'ToolChain =', Item.ToolChain, 'Option =', Item.Option, 'Arch =', Item.SupArchList
        print '\nSkuIds =', M.SkuInfos.SkuInfoList, M.SkuInfos.IncludeFiles
        print '\nLibraries =', M.Libraries, M.Libraries.IncludeFiles
        for Item in M.Libraries.LibraryList:
            print '\t', Item.FilePath, Item.SupArchList, Item.Define
        print '\nLibraryClasses =', M.LibraryClasses, M.LibraryClasses.IncludeFiles
        for Item in M.LibraryClasses.LibraryList:
            print '\t', Item.Name, Item.FilePath, Item.SupModuleList, Item.SupArchList, Item.Define
        print '\nPcds =', M.DynamicPcdBuildDefinitions
        for Item in M.DynamicPcdBuildDefinitions:
            print '\tCname=', Item.CName, 'TSG=', Item.TokenSpaceGuidCName, 'Value=', Item.DefaultValue, 'Token=', Item.Token, 'Type=', Item.ItemType, 'Datum=', Item.DatumType, 'Size=', Item.MaxDatumSize, 'Arch=', Item.SupArchList, Item.SkuInfoList
            for Sku in Item.SkuInfoList.values():
                print '\t\t', str(Sku)
        print '\nComponents =', M.Modules.ModuleList, M.Modules.IncludeFiles
        for Item in M.Modules.ModuleList:
            print '\t', Item.FilePath, Item.ExecFilePath, Item.SupArchList
            for Lib in Item.LibraryClasses.LibraryList:
                print '\t\tLib:', Lib.Name, Lib.FilePath
            for Bo in Item.ModuleSaBuildOption.BuildOptionList:
                print '\t\tBuildOption:', Bo.ToolChainFamily, Bo.ToolChain, Bo.Option
            for Pcd in Item.PcdBuildDefinitions:
                print '\t\tPcd:', Pcd.CName, Pcd.TokenSpaceGuidCName, Pcd.MaxDatumSize, Pcd.DefaultValue, Pcd.ItemType

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.DEBUG_0)

    W = os.getenv('WORKSPACE')
    F = os.path.join(W, 'Nt32Pkg/Nt32Pkg.dsc')

    Db = Database.Database('Dsc.db')
    Db.InitDatabase()

    P = Dsc(os.path.normpath(F), True, True, W, Db)
    P.ShowPlatform()

    Db.Close()
