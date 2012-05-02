## @file
# This file is used to define each component of the build database
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
import os, string, copy, pdb, copy
import EdkLogger
import DataType
from InfClassObject import *
from DecClassObject import *
from DscClassObject import *
from String import *
from BuildToolError import *
from Misc import sdict
import Database as Database
import time as time

## PcdClassObject
#
# This Class is used for PcdObject
# 
# @param object:             Inherited from object class
# @param Name:               Input value for Name of Pcd, default is None
# @param Guid:               Input value for Guid of Pcd, default is None
# @param Type:               Input value for Type of Pcd, default is None
# @param DatumType:          Input value for DatumType of Pcd, default is None
# @param Value:              Input value for Value of Pcd, default is None
# @param Token:              Input value for Token of Pcd, default is None
# @param MaxDatumSize:       Input value for MaxDatumSize of Pcd, default is None
# @param SkuInfoList:        Input value for SkuInfoList of Pcd, default is {}
# @param IsOverrided:        Input value for IsOverrided of Pcd, default is False
#
# @var TokenCName:           To store value for TokenCName
# @var TokenSpaceGuidCName:  To store value for TokenSpaceGuidCName
# @var Type:                 To store value for Type
# @var DatumType:            To store value for DatumType
# @var TokenValue:           To store value for TokenValue
# @var MaxDatumSize:         To store value for MaxDatumSize
# @var SkuInfoList:          To store value for SkuInfoList
# @var IsOverrided:          To store value for IsOverrided
# @var Phase:                To store value for Phase, default is "DXE"
#
class PcdClassObject(object):
    def __init__(self, Name = None, Guid = None, Type = None, DatumType = None, Value = None, Token = None, MaxDatumSize = None, SkuInfoList = {}, IsOverrided = False):
        self.TokenCName = Name
        self.TokenSpaceGuidCName = Guid
        self.Type = Type
        self.DatumType = DatumType
        self.DefaultValue = Value
        self.TokenValue = Token
        self.MaxDatumSize = MaxDatumSize
        self.SkuInfoList = SkuInfoList
        self.IsOverrided = IsOverrided
        self.Phase = "DXE"

    ## Convert the class to a string
    #
    #  Convert each member of the class to string
    #  Organize to a signle line format string
    #
    #  @retval Rtn Formatted String
    #
    def __str__(self):
        Rtn = '\tTokenCName=' + str(self.TokenCName) + ', ' + \
              'TokenSpaceGuidCName=' + str(self.TokenSpaceGuidCName) + ', ' + \
              'Type=' + str(self.Type) + ', ' + \
              'DatumType=' + str(self.DatumType) + ', ' + \
              'DefaultValue=' + str(self.DefaultValue) + ', ' + \
              'TokenValue=' + str(self.TokenValue) + ', ' + \
              'MaxDatumSize=' + str(self.MaxDatumSize) + ', '
        for Item in self.SkuInfoList.values():
            Rtn = Rtn + 'SkuId=' + Item.SkuId + ', ' + 'SkuIdName=' + Item.SkuIdName
        Rtn = Rtn + str(self.IsOverrided)

        return Rtn

    ## Override __eq__ function
    #
    # Check whether pcds are the same
    #
    # @retval False The two pcds are different
    # @retval True  The two pcds are the same
    #
    def __eq__(self, Other):
        return Other != None and self.TokenCName == Other.TokenCName and self.TokenSpaceGuidCName == Other.TokenSpaceGuidCName

    ## Override __hash__ function
    #
    # Use (TokenCName, TokenSpaceGuidCName) as key in hash table
    #
    # @retval truple() Key for hash table
    #
    def __hash__(self):
        return hash((self.TokenCName, self.TokenSpaceGuidCName))

## LibraryClassObject
#
# This Class defines LibraryClassObject used in BuildDatabase
# 
# @param object:      Inherited from object class
# @param Name:        Input value for LibraryClassName, default is None
# @param SupModList:  Input value for SupModList, default is []
# @param Type:        Input value for Type, default is None
#
# @var LibraryClass:  To store value for LibraryClass
# @var SupModList:    To store value for SupModList
# @var Type:          To store value for Type
#
class LibraryClassObject(object):
    def __init__(self, Name = None, SupModList = [], Type = None):
        self.LibraryClass = Name
        self.SupModList = SupModList
        if Type != None:
            self.SupModList = CleanString(Type).split(DataType.TAB_SPACE_SPLIT)

## ModuleBuildClassObject
#
# This Class defines ModuleBuildClass
# 
# @param object:               Inherited from object class
#
# @var DescFilePath:           To store value for DescFilePath
# @var BaseName:               To store value for BaseName
# @var ModuleType:             To store value for ModuleType
# @var Guid:                   To store value for Guid
# @var Version:                To store value for Version
# @var PcdIsDriver:            To store value for PcdIsDriver
# @var BinaryModule:           To store value for BinaryModule
# @var CustomMakefile:         To store value for CustomMakefile
# @var Specification:          To store value for Specification
# @var Shadow                  To store value for Shadow
# @var LibraryClass:           To store value for LibraryClass, it is a list structure as
#                              [ LibraryClassObject, ...]
# @var ModuleEntryPointList:   To store value for ModuleEntryPointList
# @var ModuleUnloadImageList:  To store value for ModuleUnloadImageList
# @var ConstructorList:        To store value for ConstructorList
# @var DestructorList:         To store value for DestructorList
# @var Binaries:               To store value for Binaries, it is a list structure as
#                              [ ModuleBinaryClassObject, ...]
# @var Sources:                To store value for Sources, it is a list structure as
#                              [ ModuleSourceFilesClassObject, ... ]
# @var LibraryClasses:         To store value for LibraryClasses, it is a set structure as
#                              { [LibraryClassName, ModuleType] : LibraryClassInfFile }
# @var Protocols:              To store value for Protocols, it is a list structure as
#                              [ ProtocolName, ... ]
# @var Ppis:                   To store value for Ppis, it is a list structure as
#                              [ PpiName, ... ]
# @var Guids:                  To store value for Guids, it is a list structure as
#                              [ GuidName, ... ]
# @var Includes:               To store value for Includes, it is a list structure as
#                              [ IncludePath, ... ]
# @var Packages:               To store value for Packages, it is a list structure as
#                              [ DecFileName, ... ]
# @var Pcds:                   To store value for Pcds, it is a set structure as
#                              { [(PcdCName, PcdGuidCName)] : PcdClassObject}
# @var BuildOptions:           To store value for BuildOptions, it is a set structure as
#                              { [BuildOptionKey] : BuildOptionValue}
# @var Depex:                  To store value for Depex
#
class ModuleBuildClassObject(object):
    def __init__(self):
        self.AutoGenVersion          = 0
        self.DescFilePath            = ''
        self.BaseName                = ''
        self.ModuleType              = ''
        self.Guid                    = ''
        self.Version                 = ''
        self.PcdIsDriver             = ''
        self.BinaryModule            = ''
        self.Shadow                  = ''
        self.CustomMakefile          = {}
        self.Specification           = {}
        self.LibraryClass            = []
        self.ModuleEntryPointList    = []
        self.ModuleUnloadImageList   = []
        self.ConstructorList         = []
        self.DestructorList          = []

        self.Binaries                = []
        self.Sources                 = []
        self.LibraryClasses          = sdict()
        self.Libraries               = []
        self.Protocols               = []
        self.Ppis                    = []
        self.Guids                   = []
        self.Includes                = []
        self.Packages                = []
        self.Pcds                    = {}
        self.BuildOptions            = {}
        self.Depex                   = ''

    ## Convert the class to a string
    #
    #  Convert member DescFilePath of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return self.DescFilePath

    ## Override __eq__ function
    #
    # Check whether ModuleBuildClassObjects are the same
    #
    # @retval False The two ModuleBuildClassObjects are different
    # @retval True  The two ModuleBuildClassObjects are the same
    #
    def __eq__(self, Other):
        return self.DescFilePath == str(Other)

    ## Override __hash__ function
    #
    # Use DescFilePath as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.DescFilePath)

## PackageBuildClassObject
#
# This Class defines PackageBuildClass
# 
# @param object:        Inherited from object class
#
# @var DescFilePath:    To store value for DescFilePath
# @var PackageName:     To store value for PackageName
# @var Guid:            To store value for Guid
# @var Version:         To store value for Version
# @var Protocols:       To store value for Protocols, it is a set structure as
#                       { [ProtocolName] : Protocol Guid, ... }
# @var Ppis:            To store value for Ppis, it is a set structure as
#                       { [PpiName] : Ppi Guid, ... }
# @var Guids:           To store value for Guids, it is a set structure as
#                       { [GuidName] : Guid, ... }
# @var Includes:        To store value for Includes, it is a list structure as
#                       [ IncludePath, ... ]
# @var LibraryClasses:  To store value for LibraryClasses, it is a set structure as
#                       { [LibraryClassName] : LibraryClassInfFile }
# @var Pcds:            To store value for Pcds, it is a set structure as
#                       { [(PcdCName, PcdGuidCName)] : PcdClassObject}
#
class PackageBuildClassObject(object):
    def __init__(self):
        self.DescFilePath            = ''
        self.PackageName             = ''
        self.Guid                    = ''
        self.Version                 = ''

        self.Protocols               = {}
        self.Ppis                    = {}
        self.Guids                   = {}
        self.Includes                = []
        self.LibraryClasses          = {}
        self.Pcds                    = {}

    ## Convert the class to a string
    #
    #  Convert member DescFilePath of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return self.DescFilePath

    ## Override __eq__ function
    #
    # Check whether PackageBuildClassObjects are the same
    #
    # @retval False The two PackageBuildClassObjects are different
    # @retval True  The two PackageBuildClassObjects are the same
    #
    def __eq__(self, Other):
        return self.DescFilePath == str(Other)

    ## Override __hash__ function
    #
    # Use DescFilePath as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.DescFilePath)

## PlatformBuildClassObject
#
# This Class defines PlatformBuildClass
# 
# @param object:          Inherited from object class
#
# @var DescFilePath:      To store value for DescFilePath
# @var PlatformName:      To store value for PlatformName
# @var Guid:              To store value for Guid
# @var Version:           To store value for Version
# @var DscSpecification:  To store value for DscSpecification
# @var OutputDirectory:   To store value for OutputDirectory
# @var FlashDefinition:   To store value for FlashDefinition
# @var BuildNumber:       To store value for BuildNumber
# @var MakefileName:      To store value for MakefileName
# @var SkuIds:            To store value for SkuIds, it is a set structure as
#                         { 'SkuName' : SkuId, '!include' : includefilename, ...}
# @var Modules:           To store value for Modules, it is a list structure as
#                         [ InfFileName, ... ]
# @var Libraries:         To store value for Libraries, it is a list structure as
#                         [ InfFileName, ... ]
# @var LibraryClasses:    To store value for LibraryClasses, it is a set structure as
#                         { (LibraryClassName, ModuleType) : LibraryClassInfFile }
# @var Pcds:              To store value for Pcds, it is a set structure as
#                         { [(PcdCName, PcdGuidCName)] : PcdClassObject }
# @var BuildOptions:      To store value for BuildOptions, it is a set structure as
#                         { [BuildOptionKey] : BuildOptionValue }
#
class PlatformBuildClassObject(object):
    def __init__(self):
        self.DescFilePath            = ''
        self.PlatformName            = ''
        self.Guid                    = ''
        self.Version                 = ''
        self.DscSpecification        = ''
        self.OutputDirectory         = ''
        self.FlashDefinition         = ''
        self.BuildNumber             = ''
        self.MakefileName            = ''

        self.SkuIds                  = {}
        self.Modules                 = []
        self.LibraryInstances        = []
        self.LibraryClasses          = {}
        self.Libraries               = {}
        self.Pcds                    = {}
        self.BuildOptions            = {}

    ## Convert the class to a string
    #
    #  Convert member DescFilePath of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return self.DescFilePath

    ## Override __eq__ function
    #
    # Check whether PlatformBuildClassObjects are the same
    #
    # @retval False The two PlatformBuildClassObjects are different
    # @retval True  The two PlatformBuildClassObjects are the same
    #
    def __eq__(self, other):
        return self.DescFilePath == str(other)

    ## Override __hash__ function
    #
    # Use DescFilePath as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.DescFilePath)

## ItemBuild
#
# This Class defines Module/Platform/Package databases for build system
#
# @param object:          Inherited from object class
# @param Arch:            Build arch
# @param Platform:        Build Platform
# @param Package:         Build Package
# @param Module:          Build Module
#
# @var Arch:              To store value for Build Arch
# @var PlatformDatabase:  To store value for PlatformDatabase, it is a set structure as
#                         { [DscFileName] : PlatformBuildClassObject, ...}
# @var PackageDatabase:   To store value for PackageDatabase, it is a set structure as
#                         { [DecFileName] : PacakgeBuildClassObject, ...}
# @var ModuleDatabase:    To store value for ModuleDatabase, it is a list structure as
#                         { [InfFileName] : ModuleBuildClassObject, ...}
#
class ItemBuild(object):
    def __init__(self, Arch, Platform = None, Package = None, Module = None):
        self.Arch                    = Arch
        self.PlatformDatabase        = {}
        self.PackageDatabase         = {}
        self.ModuleDatabase          = {}

## WorkspaceBuild
#
# This class is used to parse active platform to init all inf/dec/dsc files
# Generate module/package/platform databases for build
#
# @param object:          Inherited from object class
# @param ActivePlatform:  Input value for current active platform
# @param WorkspaceDir:    Input value for current WorkspaceDir
#
# @var WorkspaceDir:      To store value for WorkspaceDir
# @var SupArchList:       To store value for SupArchList, selection scope is in below list
#                         EBC | IA32 | X64 | IPF | ARM | PPC
# @var BuildTarget:       To store value for WorkspaceDir, selection scope is in below list
#                         RELEASE | DEBUG
# @var SkuId:             To store value for SkuId
# @var Fdf:               To store value for Fdf
# @var FdTargetList:      To store value for FdTargetList
# @var FvTargetList:      To store value for FvTargetList
# @var TargetTxt:         To store value for TargetTxt, it is a set structure as
#                         TargetTxtClassObject
# @var ToolDef:           To store value for ToolDef, it is a set structure as
#                         ToolDefClassObject
# @var InfDatabase:       To store value for InfDatabase, it is a set structure as
#                         { [InfFileName] : InfClassObject}
# @var DecDatabase:       To store value for DecDatabase, it is a set structure as
#                         { [DecFileName] : DecClassObject}
# @var DscDatabase:       To store value for DscDatabase, it is a set structure as
#                         { [DscFileName] : DscClassObject}
# @var Build:             To store value for DscDatabase, it is a set structure as
#                         ItemBuild
# @var DscFileName:       To store value for Active Platform
# @var UnFoundPcdInDsc:   To store values for the pcds defined in INF/DEC but not found in DSC, it is a set structure as
#                         { (PcdGuid, PcdCName, Arch) : DecFileName }
#
class WorkspaceBuild(object):
    def __init__(self, ActivePlatform, WorkspaceDir):
        self.WorkspaceDir            = NormPath(WorkspaceDir)
        self.SupArchList             = []
        self.BuildTarget             = []
        self.SkuId                   = ''
        self.Fdf                     = ''
        self.FdTargetList            = []
        self.FvTargetList            = []
        self.TargetTxt               = None
        self.ToolDef                 = None

        self.InfDatabase             = {}
        self.DecDatabase             = {}
        self.DscDatabase             = {}
        
        self.UnFoundPcdInDsc         = {}

        #
        # Init build for all arches
        #
        self.Build                   = {}
        for Arch in DataType.ARCH_LIST:
            self.Build[Arch] = ItemBuild(Arch)

        #
        # Init build database
        #
        self.Db = Database.Database(DATABASE_PATH)
        self.Db.InitDatabase()
        
        #
        # Get active platform
        #
        self.DscFileName = NormPath(ActivePlatform)
        File = self.WorkspaceFile(self.DscFileName)
        if os.path.exists(File) and os.path.isfile(File):
            self.DscDatabase[self.DscFileName] = Dsc(File, False, True, self.WorkspaceDir, self.Db)
        else:
            EdkLogger.error("AutoGen", FILE_NOT_FOUND, ExtraData = File)

        #
        # Parse platform to get module
        #
        for DscFile in self.DscDatabase.keys():
            Platform = self.DscDatabase[DscFile].Platform

            #
            # Get global information
            #
            Tmp = set()
            for Arch in DataType.ARCH_LIST:
                for Item in Platform.Header[Arch].SupArchList:
                    Tmp.add(Item)
            self.SupArchList = list(Tmp)
            Tmp = set()
            for Arch in DataType.ARCH_LIST:
                for Item in Platform.Header[Arch].BuildTargets:
                    Tmp.add(Item)
            self.BuildTarget = list(Tmp)
            for Arch in self.SupArchList:
                self.SkuId = Platform.Header[Arch].SkuIdName
            self.Fdf = Platform.FlashDefinitionFile.FilePath

            #
            # Get all inf files
            #
            for Item in Platform.LibraryClasses.LibraryList:
                for Arch in Item.SupArchList:
                    self.AddToInfDatabase(Item.FilePath)

            for Item in Platform.Libraries.LibraryList:
                for Arch in Item.SupArchList:
                    self.AddToInfDatabase(Item.FilePath)

            for Item in Platform.Modules.ModuleList:
                for Arch in Item.SupArchList:
                    #
                    # Add modules
                    #
                    Module = Item.FilePath
                    self.AddToInfDatabase(Module)
                    #
                    # Add library used in modules
                    #
                    for Lib in Item.LibraryClasses.LibraryList:
                        self.AddToInfDatabase(Lib.FilePath)
                        self.UpdateLibraryClassOfModule(Module, Lib.Name, Arch, Lib.FilePath)

        #
        # Parse module to get package
        #
        for InfFile in self.InfDatabase.keys():
            Module = self.InfDatabase[InfFile].Module
            #
            # Get all dec
            #
            for Item in Module.PackageDependencies:
                for Arch in Item.SupArchList:
                    self.AddToDecDatabase(Item.FilePath)
    # End of self.Init()

    ## Generate PlatformDatabase
    #
    # Go through each arch to get all items in DscDatabase to PlatformDatabase
    #
    def GenPlatformDatabase(self, PcdsSet={}):
        for Dsc in self.DscDatabase.keys():
            Platform = self.DscDatabase[Dsc].Platform
            for Arch in self.SupArchList:
                Pb = PlatformBuildClassObject()

                #
                # Defines
                #
                Pb.DescFilePath = Dsc
                Pb.PlatformName = Platform.Header[Arch].Name
                if Pb.PlatformName == '':
                    EdkLogger.error("AutoGen", PARSER_ERROR, "The BaseName of platform %s is not defined for arch %s" % (Dsc, Arch))
                Pb.Guid = Platform.Header[Arch].Guid
                Pb.Version = Platform.Header[Arch].Version
                Pb.DscSpecification = Platform.Header[Arch].DscSpecification
                Pb.OutputDirectory = Platform.Header[Arch].OutputDirectory
                Pb.FlashDefinition = Platform.FlashDefinitionFile.FilePath
                Pb.BuildNumber = Platform.Header[Arch].BuildNumber

                #
                # SkuId
                #
                for Key in Platform.SkuInfos.SkuInfoList.keys():
                    Pb.SkuIds[Key] = Platform.SkuInfos.SkuInfoList[Key]

                #
                # Module
                #
                for Item in Platform.Modules.ModuleList:
                    if Arch in Item.SupArchList:
                        Pb.Modules.append(Item.FilePath)

                #
                # BuildOptions
                #
                for Item in Platform.BuildOptions.BuildOptionList:
                    if Arch in Item.SupArchList:
                        Pb.BuildOptions[(Item.ToolChainFamily, Item.ToolChain)] = Item.Option

                #
                # LibraryClass
                #
                for Item in Platform.LibraryClasses.LibraryList:
                    SupModuleList = self.FindSupModuleListOfLibraryClass(Item, Platform.LibraryClasses.LibraryList, Arch)
                    if Arch in Item.SupArchList:
                        for ModuleType in SupModuleList:
                            Pb.LibraryClasses[(Item.Name, ModuleType)] = Item.FilePath

                #
                # Libraries
                # 
                for Item in Platform.Libraries.LibraryList:
                    for ItemArch in Item.SupArchList:
                        Library = self.InfDatabase[Item.FilePath]
                        if ItemArch not in Library.Module.Header:
                            continue
                        Pb.Libraries[Library.Module.Header[ItemArch].Name] = Item.FilePath
                
                #
                # Pcds
                #
                for Item in Platform.DynamicPcdBuildDefinitions:
                    if Arch in Item.SupArchList:
                        Name = Item.CName
                        Guid = Item.TokenSpaceGuidCName
                        Type = Item.ItemType
                        DatumType = Item.DatumType
                        Value = Item.DefaultValue
                        Token = Item.Token
                        MaxDatumSize = Item.MaxDatumSize
                        SkuInfoList = Item.SkuInfoList
                        Pb.Pcds[(Name, Guid)] = PcdClassObject(Name, Guid, Type, DatumType, Value, Token, MaxDatumSize, SkuInfoList, False)

                for (Name, Guid) in PcdsSet:
                    Value = PcdsSet[Name, Guid]
                    for PcdType in ["FixedAtBuild", "PatchableInModule", "FeatureFlag", "Dynamic", "DynamicEx"]:
                        for Dec in self.Build[Arch].PackageDatabase:
                            Pcds = self.Build[Arch].PackageDatabase[Dec].Pcds
                            if (Name, Guid, PcdType) in Pcds:
                                Pcd = Pcds[(Name, Guid, PcdType)]
                                Type = PcdType
                                DatumType = Pcd.DatumType
                                Token = Pcd.TokenValue
                                MaxDatumSize = Pcd.MaxDatumSize
                                SkuInfoList = Pcd.SkuInfoList
                                Pb.Pcds[(Name, Guid)] = PcdClassObject(Name, Guid, Type, DatumType, Value, Token, MaxDatumSize, SkuInfoList, False)
                                break
                        else:
                            # nothing found
                            continue
                        # found in one package, find next PCD
                        break
                    else:
                        EdkLogger.error("AutoGen", PARSER_ERROR, "PCD is not found in any package", ExtraData="%s.%s" % (Guid, Name))
                #
                # Add to database
                #
                self.Build[Arch].PlatformDatabase[Dsc] = Pb
                Pb = None

    ## Generate PackageDatabase
    #
    # Go through each arch to get all items in DecDatabase to PackageDatabase
    #
    def GenPackageDatabase(self):
        for Dec in self.DecDatabase.keys():
            Package = self.DecDatabase[Dec].Package

            for Arch in self.SupArchList:
                Pb = PackageBuildClassObject()

                #
                # Defines
                #
                Pb.DescFilePath = Dec
                Pb.PackageName = Package.Header[Arch].Name
                if Pb.PackageName == '':
                    EdkLogger.error("AutoGen", PARSER_ERROR, "The BaseName of package %s is not defined for arch %s" % (Dec, Arch))

                Pb.Guid = Package.Header[Arch].Guid
                Pb.Version = Package.Header[Arch].Version

                #
                # Protocols
                #
                for Item in Package.ProtocolDeclarations:
                    if Arch in Item.SupArchList:
                        Pb.Protocols[Item.CName] = Item.Guid

                #
                # Ppis
                #
                for Item in Package.PpiDeclarations:
                    if Arch in Item.SupArchList:
                        Pb.Ppis[Item.CName] = Item.Guid

                #
                # Guids
                #
                for Item in Package.GuidDeclarations:
                    if Arch in Item.SupArchList:
                        Pb.Guids[Item.CName] = Item.Guid

                #
                # Includes
                #
                for Item in Package.Includes:
                    if Arch in Item.SupArchList:
                        Pb.Includes.append(Item.FilePath)

                #
                # LibraryClasses
                #
                for Item in Package.LibraryClassDeclarations:
                    if Arch in Item.SupArchList:
                        Pb.LibraryClasses[Item.LibraryClass] = Item.RecommendedInstance

                #
                # Pcds
                #
                for Item in Package.PcdDeclarations:
                    if Arch in Item.SupArchList:
                        Name = Item.CName
                        Guid = Item.TokenSpaceGuidCName
                        Type = Item.ItemType
                        DatumType = Item.DatumType
                        Value = Item.DefaultValue
                        Token = Item.Token
                        MaxDatumSize = Item.MaxDatumSize
                        SkuInfoList = Item.SkuInfoList
                        Pb.Pcds[(Name, Guid, Type)] = PcdClassObject(Name, Guid, Type, DatumType, Value, Token, MaxDatumSize, SkuInfoList, False)

                #
                # Add to database
                #
                self.Build[Arch].PackageDatabase[Dec] = Pb
                Pb = None

    ## Generate ModuleDatabase
    #
    # Go through each arch to get all items in InfDatabase to ModuleDatabase
    #    
    def GenModuleDatabase(self, InfList = []):
        for Inf in self.InfDatabase.keys():
            Module = self.InfDatabase[Inf].Module

            for Arch in self.SupArchList:
                if not self.IsModuleDefinedInPlatform(Inf, Arch, InfList) or Arch not in Module.Header:
                    continue

                ModuleHeader = Module.Header[Arch]
                Pb = ModuleBuildClassObject()

                #
                # Defines
                #
                Pb.DescFilePath = Inf
                Pb.BaseName = ModuleHeader.Name
                if Pb.BaseName == '':
                    EdkLogger.error("AutoGen", PARSER_ERROR, "The BaseName of module %s is not defined for arch %s" % (Inf, Arch))                
                Pb.Guid = ModuleHeader.Guid
                Pb.Version = ModuleHeader.Version
                Pb.ModuleType = ModuleHeader.ModuleType
                Pb.PcdIsDriver = ModuleHeader.PcdIsDriver
                Pb.BinaryModule = ModuleHeader.BinaryModule
                Pb.CustomMakefile = ModuleHeader.CustomMakefile
                Pb.Shadow = ModuleHeader.Shadow

                #
                # Specs os Defines
                #
                Pb.Specification = ModuleHeader.Specification
                Pb.Specification[TAB_INF_DEFINES_EDK_RELEASE_VERSION] = ModuleHeader.EdkReleaseVersion
                Pb.Specification[TAB_INF_DEFINES_EFI_SPECIFICATION_VERSION] = ModuleHeader.UefiSpecificationVersion
                Pb.Specification[TAB_INF_DEFINES_UEFI_SPECIFICATION_VERSION] = ModuleHeader.UefiSpecificationVersion
                Pb.AutoGenVersion = int(ModuleHeader.InfVersion, 0)

                #
                # LibraryClass of Defines
                #
                for Item in ModuleHeader.LibraryClass:
                    Pb.LibraryClass.append(LibraryClassObject(Item.LibraryClass, Item.SupModuleList, None))

                #
                # Module image and library of Defines
                #
                for Item in Module.ExternImages:
                    if Item.ModuleEntryPoint != '' and Item.ModuleEntryPoint not in Pb.ModuleEntryPointList:
                        Pb.ModuleEntryPointList.append(Item.ModuleEntryPoint)
                    if Item.ModuleUnloadImage != '' and Item.ModuleUnloadImage not in Pb.ModuleUnloadImageList:
                        Pb.ModuleUnloadImageList.append(Item.ModuleUnloadImage)
                for Item in Module.ExternLibraries:
                    if Item.Constructor != '' and Item.Constructor not in Pb.ConstructorList:
                        Pb.ConstructorList.append(Item.Constructor)
                    if Item.Destructor != '' and Item.Destructor not in Pb.DestructorList:
                        Pb.DestructorList.append(Item.Destructor)

                #
                # Binaries
                #
                for Item in Module.Binaries:
                    if Arch in Item.SupArchList:
                        FileName = Item.BinaryFile
                        FileType = Item.FileType
                        Target = Item.Target
                        FeatureFlag = Item.FeatureFlag
                        Pb.Binaries.append(ModuleBinaryFileClass(FileName, FileType, Target, FeatureFlag, Arch.split()))

                #
                # Sources
                #
                for Item in Module.Sources:
                    if Arch in Item.SupArchList:
                        SourceFile = Item.SourceFile
                        TagName = Item.TagName
                        ToolCode = Item.ToolCode
                        ToolChainFamily = Item.ToolChainFamily
                        FeatureFlag = Item.FeatureFlag
                        Pb.Sources.append(ModuleSourceFileClass(SourceFile, TagName, ToolCode, ToolChainFamily, FeatureFlag))

                #
                # Protocols
                #
                for Item in Module.Protocols:
                    if Arch in Item.SupArchList:
                        Pb.Protocols.append(Item.CName)

                #
                # Ppis
                #
                for Item in Module.Ppis:
                    if Arch in Item.SupArchList:
                        Pb.Ppis.append(Item.CName)

                #
                # Guids
                #
                for Item in Module.Guids:
                    if Arch in Item.SupArchList:
                        Pb.Ppis.append(Item.CName)

                #
                # Includes
                #
                for Item in Module.Includes:
                    if Arch in Item.SupArchList:
                        Pb.Includes.append(Item.FilePath)

                #
                # Packages
                #
                for Item in Module.PackageDependencies:
                    if Arch in Item.SupArchList:
                        Pb.Packages.append(Item.FilePath)

                #
                # BuildOptions
                #
                for Item in Module.BuildOptions:
                    if Arch in Item.SupArchList:
                        if (Item.ToolChainFamily, Item.ToolChain) not in Pb.BuildOptions:
                            Pb.BuildOptions[(Item.ToolChainFamily, Item.ToolChain)] = Item.Option
                        else:
                            OptionString = Pb.BuildOptions[(Item.ToolChainFamily, Item.ToolChain)]
                            Pb.BuildOptions[(Item.ToolChainFamily, Item.ToolChain)] = OptionString + " " + Item.Option
                self.FindBuildOptions(Arch, Inf, Pb.BuildOptions)

                #
                # Depex
                #
                for Item in Module.Depex:
                    if Arch in Item.SupArchList:
                        Pb.Depex = Pb.Depex + Item.Depex + ' '
                Pb.Depex = Pb.Depex.strip()

                #
                # LibraryClasses
                #
                for Item in Module.LibraryClasses:
                    if Arch in Item.SupArchList:
                        Lib = Item.LibraryClass
                        RecommendedInstance = Item.RecommendedInstance
                        if Pb.LibraryClass != []:
                            #
                            # For Library
                            #
                            for Libs in Pb.LibraryClass:
                                for Type in Libs.SupModList:
                                    Instance = self.FindLibraryClassInstanceOfLibrary(Lib, Arch, Type)
                                    if Instance == None:
                                        Instance = RecommendedInstance
                                    Pb.LibraryClasses[(Lib, Type)] = Instance
                        else:
                            #
                            # For Module
                            #
                            Instance = self.FindLibraryClassInstanceOfModule(Lib, Arch, Pb.ModuleType, Inf)
                            if Instance == None:
                                Instance = RecommendedInstance
                            Pb.LibraryClasses[(Lib, Pb.ModuleType)] = Instance

                #
                # Libraries
                #
                for Item in Module.Libraries:
                    if Arch in Item.SupArchList:
                        Pb.Libraries.append(Item.Library)

                #
                # Pcds
                #
                for Item in Module.PcdCodes:
                    if Arch in Item.SupArchList:
                        Name = Item.CName
                        Guid = Item.TokenSpaceGuidCName
                        Type = Item.ItemType
                        Pb.Pcds[(Name, Guid)] = self.FindPcd(Arch, Inf, Name, Guid, Type)

                #
                # Add to database
                #
                self.Build[Arch].ModuleDatabase[Inf] = Pb
                Pb = None

    ## Update Libraries Of Platform Database
    #
    # @param InfList: A list for all inf files
    #
    def UpdateLibrariesOfPlatform(self, InfList = []):
        for Arch in self.SupArchList:
            PlatformDatabase = self.Build[Arch].PlatformDatabase
            for Dsc in PlatformDatabase:
                Platform = PlatformDatabase[Dsc]
                for Inf in Platform.Modules:
                    if not self.IsModuleDefinedInPlatform(Inf, Arch, InfList):
                        continue
                    Module = self.Build[Arch].ModuleDatabase[Inf]
                    if Module.LibraryClass == None or Module.LibraryClass == []:
                        self.UpdateLibrariesOfModule(Platform, Module, Arch)
                        for Key in Module.LibraryClasses:
                            Lib = Module.LibraryClasses[Key]
                            if Lib not in Platform.LibraryInstances:
                                Platform.LibraryInstances.append(Lib)


    ## Update Libraries Of Module Database
    #
    # @param Module:  The module need to be updated libraries
    # @param Arch:    The supportted arch of the module
    #
    def UpdateLibrariesOfModule(self, Platform, Module, Arch):
        ModuleDatabase = self.Build[Arch].ModuleDatabase
        ModuleType = Module.ModuleType

        # check Edk module
        if Module.AutoGenVersion < 0x00010005:
            EdkLogger.verbose("")
            EdkLogger.verbose("Library instances of module [%s] [%s]:" % (str(Module), Arch))
            LibraryConsumerList = [Module]

            # "CompilerStub" is a must for Edk modules
            Module.Libraries.append("CompilerStub")
            while len(LibraryConsumerList) > 0:
                M = LibraryConsumerList.pop()
                for LibraryName in M.Libraries:
                    if LibraryName not in Platform.Libraries:
                        EdkLogger.warn("AutoGen", "Library [%s] is not found" % LibraryName,
                                        ExtraData="\t%s [%s]" % (str(Module), Arch))
                        continue
    
                    LibraryFile = Platform.Libraries[LibraryName]
                    if (LibraryName, ModuleType) not in Module.LibraryClasses:
                        Module.LibraryClasses[LibraryName, ModuleType] = LibraryFile
                        LibraryConsumerList.append(ModuleDatabase[LibraryFile])
                        EdkLogger.verbose("\t" + LibraryName + " : " + LibraryFile)
            return

        # EdkII module
        LibraryConsumerList = [Module]
        Constructor         = []
        ConsumedByList      = sdict()
        LibraryInstance     = sdict()

        EdkLogger.verbose("")
        EdkLogger.verbose("Library instances of module [%s] [%s]:" % (str(Module), Arch))
        while len(LibraryConsumerList) > 0:
            M = LibraryConsumerList.pop()
            for Key, LibraryPath in M.LibraryClasses.iteritems():
                # The "Key" is in format of (library_class_name, supported_module_type)
                if ModuleType != "USER_DEFINED" and ModuleType not in Key:
                    EdkLogger.debug(EdkLogger.DEBUG_3, "%s for module type %s is not supported (%s)" % (Key + (LibraryPath,)))
                    continue

                LibraryClassName = Key[0]
                if LibraryClassName not in LibraryInstance or LibraryInstance[LibraryClassName] == None:
                    if LibraryPath == None or LibraryPath == "":
                        LibraryInstance[LibraryClassName] = None
                        continue
                    LibraryModule = ModuleDatabase[LibraryPath]
                    LibraryInstance[LibraryClassName] = LibraryModule
                    LibraryConsumerList.append(LibraryModule)
                    EdkLogger.verbose("\t" + LibraryClassName + " : " + str(LibraryModule))
                elif LibraryPath == None or LibraryPath == "":
                    continue
                else:
                    LibraryModule = LibraryInstance[LibraryClassName]

                if LibraryModule.ConstructorList != [] and LibraryModule not in Constructor:
                    Constructor.append(LibraryModule)

                if LibraryModule not in ConsumedByList:
                    ConsumedByList[LibraryModule] = []
                if M != Module:
                    if M in ConsumedByList[LibraryModule]:
                        continue
                    ConsumedByList[LibraryModule].append(M)
        #
        # Initialize the sorted output list to the empty set
        #
        SortedLibraryList = []
        #
        # Q <- Set of all nodes with no incoming edges
        #
        LibraryList = [] #LibraryInstance.values()
        Q = []
        for LibraryClassName in LibraryInstance:
            M = LibraryInstance[LibraryClassName]
            if M == None:
                EdkLogger.error("AutoGen", AUTOGEN_ERROR,
                                "Library instance for library class [%s] is not found" % LibraryClassName,
                                ExtraData="\t%s [%s]" % (str(Module), Arch))
            LibraryList.append(M)
            #
            # check if there're duplicate library classes
            #
            for Lc in M.LibraryClass:
                if Lc.SupModList != None and ModuleType not in Lc.SupModList:
                    EdkLogger.error("AutoGen", AUTOGEN_ERROR,
                                    "Module type [%s] is not supported by library instance [%s]" % (ModuleType, str(M)),
                                    ExtraData="\t%s" % str(Module))

                if Lc.LibraryClass in LibraryInstance and str(M) != str(LibraryInstance[Lc.LibraryClass]):
                    EdkLogger.error("AutoGen", AUTOGEN_ERROR,
                                    "More than one library instance found for library class [%s] in module [%s]" % (Lc.LibraryClass, Module),
                                    ExtraData="\t%s\n\t%s" % (LibraryInstance[Lc.LibraryClass], str(M))
                                    )
            if ConsumedByList[M] == []:
                Q.insert(0, M)
        #
        # while Q is not empty do
        #
        while Q != []:
            #
            # remove node from Q
            #
            Node = Q.pop()
            #
            # output Node
            #
            SortedLibraryList.append(Node)
            #
            # for each node Item with an edge e from Node to Item do
            #
            for Item in LibraryList:
                if Node not in ConsumedByList[Item]:
                    continue
                #
                # remove edge e from the graph
                #
                ConsumedByList[Item].remove(Node)
                #
                # If Item has no other incoming edges then
                #
                if ConsumedByList[Item] == []:
                    #
                    # insert Item into Q
                    #
                    Q.insert(0, Item)

            EdgeRemoved = True
            while Q == [] and EdgeRemoved:
                EdgeRemoved = False
                #
                # for each node Item with a Constructor
                #
                for Item in LibraryList:
                    if Item in Constructor:
                        #
                        # for each Node without a constructor with an edge e from Item to Node
                        #
                        for Node in ConsumedByList[Item]:
                            if Node not in Constructor:
                                #
                                # remove edge e from the graph
                                #
                                ConsumedByList[Item].remove(Node)
                                EdgeRemoved = True
                                if ConsumedByList[Item] == []:
                                    #
                                    # insert Item into Q
                                    #
                                    Q.insert(0, Item)
                                    break
                    if Q != []:
                        break

        #
        # if any remaining node Item in the graph has a constructor and an incoming edge, then the graph has a cycle
        #
        for Item in LibraryList:
            if ConsumedByList[Item] != [] and Item in Constructor and len(Constructor) > 1:
                ErrorMessage = 'Library [%s] with constructors has a cycle' % str(Item)
                EdkLogger.error("AutoGen", AUTOGEN_ERROR, ErrorMessage,
                                "\tconsumed by " + "\n\tconsumed by ".join([str(L) for L in ConsumedByList[Item]]))
            if Item not in SortedLibraryList:
                SortedLibraryList.append(Item)

        #
        # Build the list of constructor and destructir names
        # The DAG Topo sort produces the destructor order, so the list of constructors must generated in the reverse order
        #
        SortedLibraryList.reverse()
        Module.LibraryClasses = sdict()
        for L in SortedLibraryList:
            for Lc in L.LibraryClass:
                Module.LibraryClasses[Lc.LibraryClass, ModuleType] = str(L)
            #
            # Merge PCDs from library instance
            #
            for Key in L.Pcds:
                if Key not in Module.Pcds:
                    LibPcd = L.Pcds[Key]
                    Module.Pcds[Key] = self.FindPcd(Arch, str(Module), LibPcd.TokenCName, LibPcd.TokenSpaceGuidCName, LibPcd.Type)
            #
            # Merge GUIDs from library instance
            #
            for CName in L.Guids:
                if CName not in Module.Guids:
                    Module.Guids.append(CName)
            #
            # Merge Protocols from library instance
            #
            for CName in L.Protocols:
                if CName not in Module.Protocols:
                    Module.Protocols.append(CName)
            #
            # Merge Ppis from library instance
            #
            for CName in L.Ppis:
                if CName not in Module.Ppis:
                    Module.Ppis.append(CName)

    ## GenBuildDatabase
    #
    # Generate build database for all arches
    #
    # @param PcdsSet: Pcd list for override from Fdf parse result
    # @param InfList: Inf list for override from Fdf parse result
    #
    def GenBuildDatabase(self, PcdsSet = {}, InfList = []):
        #
        # Add additional inf file defined in Fdf file
        #
        for InfFile in InfList:
            self.AddToInfDatabase(NormPath(InfFile))
        
        #
        # Generate PlatformDatabase, PackageDatabase and ModuleDatabase
        #
        self.GenPackageDatabase()
        self.GenPlatformDatabase(PcdsSet)
        self.GenModuleDatabase(InfList)
        
        self.Db.Close()
        
        #
        # Update Libraries Of Platform
        #
        self.UpdateLibrariesOfPlatform(InfList)
        
        #
        # Output used Pcds not found in DSC file
        #
        self.ShowUnFoundPcds()

    ## ShowUnFoundPcds()
    #
    # If there is any pcd used but not defined in DSC
    # Print warning message on screen and output a list of pcds
    #
    def ShowUnFoundPcds(self):
        if self.UnFoundPcdInDsc != {}:
            WrnMessage = '**** WARNING ****\n'
            WrnMessage += 'The following Pcds were not defined in the DSC file: %s\n' % self.DscFileName
            WrnMessage += 'The default values were obtained from the DEC file that declares the PCD and the PCD default value\n'
            for (Guid, Name, Type, Arch) in self.UnFoundPcdInDsc:
                Dec = self.UnFoundPcdInDsc[(Guid, Name, Type, Arch)]
                Pcds = self.Build[Arch].PackageDatabase[Dec].Pcds
                if (Name, Guid, Type) in Pcds:
                    Pcd = Pcds[(Name, Guid, Type)]
                    PcdItemTypeUsed = Pcd.Type
                    DefaultValue = Pcd.DefaultValue
                    WrnMessage += '%s.%s: Defined in file %s, PcdItemType is Pcds%s, DefaultValue is %s\n' % (Guid, Name, Dec, PcdItemTypeUsed, DefaultValue)
            EdkLogger.verbose(WrnMessage)
        
    ## Create a full path with workspace dir
    #
    # Convert Filename with workspace dir to create a full path
    #
    # @param Filename: The filename need to be added workspace dir
    #
    # @retval string Full path
    #
    def WorkspaceFile(self, Filename):
        return WorkspaceFile(self.WorkspaceDir, Filename)

    ## Update LibraryClass of Module
    #
    # If a module of a platform has its own override libraryclass but the libraryclass not defined in the module
    # Add this libraryclass to the module
    #
    # @param InfFileName:       InfFileName specificed in platform
    # @param LibraryClass:      LibraryClass specificed in platform
    # @param Arch:              Supportted Arch
    # @param InstanceFilePath:  InstanceFilePath specificed in platform
    #
    def UpdateLibraryClassOfModule(self, InfFileName, LibraryClass, Arch, InstanceFilePath):
        #
        # Update the library instance itself to add this libraryclass name
        #
        LibraryModule = self.InfDatabase[InstanceFilePath].Module
        LibList = LibraryModule.Header[Arch].LibraryClass
        NotFound = True
        for Lib in LibList:
            #
            # Find this LibraryClass
            #
            if Lib.LibraryClass == LibraryClass:
                NotFound = False;
                break;
        if NotFound:
            NewLib = LibraryClassClass()
            NewLib.LibraryClass = LibraryClass
            NewLib.SupModuleList = DataType.SUP_MODULE_LIST # LibraryModule.Header[Arch].ModuleType.split()
            LibraryModule.Header[Arch].LibraryClass.append(NewLib)

        #
        # Add it to LibraryClasses Section for the module which is using the library
        #
        Module = self.InfDatabase[InfFileName].Module
        LibList = Module.LibraryClasses
        NotFound = True
        for Lib in LibList:
            #
            # Find this LibraryClass
            #
            if Lib.LibraryClass == LibraryClass:
                if Arch in Lib.SupArchList:
                    return
                else:
                    Lib.SupArchList.append(Arch)
                    return
        if NotFound:
            Lib = LibraryClassClass()
            Lib.LibraryClass = LibraryClass
            Lib.SupArchList = [Arch]
            Module.LibraryClasses.append(Lib)

    ## Add Inf file to InfDatabase
    #
    # Create a Inf instance for input inf file and add it to InfDatabase
    #
    # @param InfFileName: The InfFileName need to be added to database
    #
    def AddToInfDatabase(self, InfFileName):
        File = self.WorkspaceFile(InfFileName)
        if os.path.exists(File) and os.path.isfile(File):
            if InfFileName not in self.InfDatabase:
                self.InfDatabase[InfFileName] = Inf(File, False, True, self.WorkspaceDir, self.Db, self.SupArchList)
        else:
            EdkLogger.error("AutoGen", FILE_NOT_FOUND, ExtraData=File)

    ## Add Dec file to DecDatabase
    #
    # Create a Dec instance for input dec file and add it to DecDatabase
    #
    # @param DecFileName: The DecFileName need to be added to database
    #
    def AddToDecDatabase(self, DecFileName):
        File = self.WorkspaceFile(DecFileName)
        if os.path.exists(File) and os.path.isfile(File):
            if DecFileName not in self.DecDatabase:
                self.DecDatabase[DecFileName] = Dec(File, False, True, self.WorkspaceDir, self.Db, self.SupArchList)
        else:
            EdkLogger.error("AutoGen", FILE_NOT_FOUND, ExtraData=File)

    ## Search LibraryClass Instance for Module
    #
    # Search PlatformBuildDatabase to find LibraryClass Instance for Module
    # Return the instance if found
    #
    # @param Lib:         Input value for Library Class Name
    # @param Arch:        Supportted Arch
    # @param ModuleType:  Supportted ModuleType
    # @param ModuleName:  Input value for Module Name
    #
    # @retval string Found LibraryClass Instance file path
    #
    def FindLibraryClassInstanceOfModule(self, Lib, Arch, ModuleType, ModuleName):
        #
        # First find if exist in <LibraryClass> of <Components> from dsc file
        #
        for Dsc in self.DscDatabase.keys():
            Platform = self.DscDatabase[Dsc].Platform
            for Module in Platform.Modules.ModuleList:
                if Arch in Module.SupArchList:
                    if Module.FilePath == ModuleName:
                        for LibraryClass in Module.LibraryClasses.LibraryList:
                            if LibraryClass.Name == Lib:
                                return LibraryClass.FilePath
        #
        #Second find if exist in <LibraryClass> of <LibraryClasses> from dsc file
        #
        return self.FindLibraryClassInstanceOfLibrary(Lib, Arch, ModuleType)

    ## Search LibraryClass Instance for Library
    #
    # Search PlatformBuildDatabase to find LibraryClass Instance for Library
    # Return the instance if found
    #
    # @param Lib:   Input value for Library Class Name
    # @param Arch:  Supportted Arch
    # @param Type:  Supportted Library Usage Type
    #
    # @retval string Found LibraryClass Instance file path
    # @retval None   Not Found
    #
    def FindLibraryClassInstanceOfLibrary(self, Lib, Arch, Type):
        for Dsc in self.DscDatabase.keys():
            Platform  = self.DscDatabase[Dsc].Platform
            if (Lib, Type) in self.Build[Arch].PlatformDatabase[Dsc].LibraryClasses:
                return self.Build[Arch].PlatformDatabase[Dsc].LibraryClasses[(Lib, Type)]
            elif (Lib, '') in self.Build[Arch].PlatformDatabase[Dsc].LibraryClasses:
                return self.Build[Arch].PlatformDatabase[Dsc].LibraryClasses[(Lib, '')]
        return None

    ## Find BuildOptions
    #
    # Search DscDatabase to find component definition of ModuleName
    # Override BuildOption if it is defined in component
    #
    # @param Arch:          Supportted Arch
    # @param ModuleName:    The module which has buildoption definition in component of platform
    # @param BuildOptions:  The set of all buildopitons
    #
    def FindBuildOptions(self, Arch, ModuleName, BuildOptions):
        for Dsc in self.DscDatabase.keys():
            #
            # First find if exist in <BuildOptions> of <Components> from dsc file
            # if find, use that override the one defined in inf file
            #
            Platform = self.DscDatabase[Dsc].Platform
            for Module in Platform.Modules.ModuleList:
                if Arch in Module.SupArchList:
                    if Module.FilePath == ModuleName:
                        for BuildOption in Module.ModuleSaBuildOption.BuildOptionList:
                            #
                            # Add to BuildOptions
                            #
                            BuildOptions[(BuildOption.ToolChainFamily, BuildOption.ToolChain)] = BuildOption.Option

    ## Find Pcd
    #
    # Search platform database, package database, module database and PcdsSet from Fdf
    # Return found Pcd
    #
    # @param Arch:        Supportted Arch
    # @param ModuleName:  The module which has pcd definition in component of platform
    # @param Name:        Name of Pcd
    # @param Guid:        Guid of Pcd
    # @param Type:        Type of Pcd
    #
    # @retval PcdClassObject An instance for PcdClassObject with all members filled
    #
    def FindPcd(self, Arch, ModuleName, Name, Guid, Type):
        NewType = ''
        DatumType = ''
        Value = ''
        Token = ''
        MaxDatumSize = ''
        SkuInfoList = {}
        IsOverrided = False
        IsFoundInDsc = False
        IsFoundInDec = False
        FoundInDecFile = ''
        
        #
        # Second get information from platform database
        #
        OwnerPlatform = ''
        for Dsc in self.Build[Arch].PlatformDatabase.keys():
            Pcds = self.Build[Arch].PlatformDatabase[Dsc].Pcds
            if (Name, Guid) in Pcds:
                OwnerPlatform = Dsc
                Pcd = Pcds[(Name, Guid)]
                if Pcd.Type != '' and Pcd.Type != None:
                    NewType = Pcd.Type
                    if NewType in DataType.PCD_DYNAMIC_TYPE_LIST:
                        NewType = DataType.TAB_PCDS_DYNAMIC
                    elif NewType in DataType.PCD_DYNAMIC_EX_TYPE_LIST:
                        NewType = DataType.TAB_PCDS_DYNAMIC_EX
                else:
                    NewType = Type

                if Type != '' and Type != NewType:
                    ErrorMsg = "PCD %s.%s is declared as [%s] in module\n\t%s\n\n"\
                               "    But it's used as [%s] in platform\n\t%s"\
                               % (Guid, Name, Type, ModuleName, NewType, OwnerPlatform)
                    EdkLogger.error("AutoGen", PARSER_ERROR, ErrorMsg)


                if Pcd.DatumType != '' and Pcd.DatumType != None:
                    DatumType = Pcd.DatumType
                if Pcd.TokenValue != '' and Pcd.TokenValue != None:
                    Token = Pcd.TokenValue
                if Pcd.DefaultValue != '' and Pcd.DefaultValue != None:
                    Value = Pcd.DefaultValue
                if Pcd.MaxDatumSize != '' and Pcd.MaxDatumSize != None:
                    MaxDatumSize = Pcd.MaxDatumSize
                SkuInfoList =  Pcd.SkuInfoList
                
                IsOverrided = True
                IsFoundInDsc = True
                break

        #
        # Third get information from <Pcd> of <Compontents> from module database
        #
        for Dsc in self.DscDatabase.keys():
            for Module in self.DscDatabase[Dsc].Platform.Modules.ModuleList:
                if Arch in Module.SupArchList:
                    if Module.FilePath == ModuleName:
                        for Pcd in Module.PcdBuildDefinitions:
                            if (Name, Guid) == (Pcd.CName, Pcd.TokenSpaceGuidCName):
                                if Pcd.DefaultValue != '':
                                    Value = Pcd.DefaultValue
                                if Pcd.MaxDatumSize != '':
                                    MaxDatumSize = Pcd.MaxDatumSize
                                    
                                IsFoundInDsc = True
                                IsOverrided = True
                                break

        #
        # First get information from package database
        #
        Pcd = None
        if NewType == '':
            if Type != '':
                PcdTypeList = [Type]
            else:
                PcdTypeList = ["FixedAtBuild", "PatchableInModule", "FeatureFlag", "Dynamic", "DynamicEx"]

            for Dec in self.Build[Arch].PackageDatabase.keys():
                Pcds = self.Build[Arch].PackageDatabase[Dec].Pcds
                for PcdType in PcdTypeList:
                    if (Name, Guid, PcdType) in Pcds:
                        Pcd = Pcds[(Name, Guid, PcdType)]
                        NewType = PcdType
                        IsOverrided = True
                        IsFoundInDec = True
                        FoundInDecFile = Dec
                        break
                else:
                    continue
                break
        else:
            for Dec in self.Build[Arch].PackageDatabase.keys():
                Pcds = self.Build[Arch].PackageDatabase[Dec].Pcds
                if (Name, Guid, NewType) in Pcds:
                    Pcd = Pcds[(Name, Guid, NewType)]
                    IsOverrided = True
                    IsFoundInDec = True
                    FoundInDecFile = Dec
                    break

        if not IsFoundInDec:
            ErrorMsg = "Pcd '%s.%s [%s]' defined in module '%s' is not found in any package for Arch '%s'" % (Guid, Name, NewType, ModuleName, Arch)
            EdkLogger.error("AutoGen", PARSER_ERROR, ErrorMsg)

        #
        # Not found in any platform and fdf
        #
        if not IsFoundInDsc:
            Value = Pcd.DefaultValue
            if NewType.startswith("Dynamic") and SkuInfoList == {}:
                SkuIds = self.Build[Arch].PlatformDatabase.values()[0].SkuIds
                SkuInfoList['DEFAULT'] = SkuInfoClass(SkuIdName='DEFAULT', SkuId=SkuIds['DEFAULT'], DefaultValue=Value)
            self.UnFoundPcdInDsc[(Guid, Name, NewType, Arch)] = FoundInDecFile
        #elif Type != '' and NewType.startswith("Dynamic"):
        #    NewType = Pcd.Type
        DatumType = Pcd.DatumType
        if Token in [None, '']:
            Token = Pcd.TokenValue
        if DatumType == "VOID*" and MaxDatumSize in ['', None]:
            EdkLogger.verbose("No MaxDatumSize specified for PCD %s.%s in module [%s]" % (Guid, Name, ModuleName))
            if Value[0] == 'L':
                MaxDatumSize = str(len(Value) * 2)
            elif Value[0] == '{':
                MaxDatumSize = str(len(Value.split(',')))
            else:
                MaxDatumSize = str(len(Value))

        return PcdClassObject(Name, Guid, NewType, DatumType, Value, Token, MaxDatumSize, SkuInfoList, IsOverrided)

    ## Find Supportted Module List Of LibraryClass
    #
    # Search in InfDatabase, find the supmodulelist of the libraryclass
    #
    # @param LibraryClass:               LibraryClass name for search
    # @param OverridedLibraryClassList:  A list of all LibraryClass
    # @param Arch:                       Supportted Arch
    #
    # @retval list SupModuleList
    #
    def FindSupModuleListOfLibraryClass(self, LibraryClass, OverridedLibraryClassList, Arch):
        Name = LibraryClass.Name
        FilePath = LibraryClass.FilePath
        SupModuleList = copy.copy(LibraryClass.SupModuleList)

        #
        # If the SupModuleList means all, remove overrided module types of platform
        #
        if SupModuleList == DataType.SUP_MODULE_LIST:
            EdkLogger.debug(EdkLogger.DEBUG_3, "\tLibraryClass %s supports all module types" % Name)
            for Item in OverridedLibraryClassList:
                #
                # Find a library class (Item) with the same name
                #
                if Item.Name == Name:
                    #
                    # Do nothing if it is itself
                    #
                    if Item.SupModuleList == DataType.SUP_MODULE_LIST:
                        continue
                    #
                    # If not itself, check arch first
                    #
                    if Arch in LibraryClass.SupArchList:
                        #
                        # If arch is supportted, remove all related module type
                        #
                        if Arch in Item.SupArchList:
                            for ModuleType in Item.SupModuleList:
                                EdkLogger.debug(EdkLogger.DEBUG_3, "\tLibraryClass %s has specific defined module types" % Name)
                                if ModuleType in SupModuleList:
                                    SupModuleList.remove(ModuleType)

        return SupModuleList

    ## Find Module inf Platform
    #
    # Check if the module is defined in <Compentent> of <Platform>
    #
    # @param Inf:      Inf file (Module) need to be searched
    # @param Arch:     Supportted Arch
    # @param InfList:  A list for all Inf file
    #
    # @retval True     Mudule Found
    # @retval Flase    Module Not Found
    #
    def IsModuleDefinedInPlatform(self, Inf, Arch, InfList):
        for Dsc in self.DscDatabase.values():
            for LibraryClass in Dsc.Platform.LibraryClasses.LibraryList:
                if Inf == LibraryClass.FilePath and Arch in LibraryClass.SupArchList:
                    return True
            for Module in Dsc.Platform.Modules.ModuleList:
                if Inf == Module.FilePath and Arch in Module.SupArchList:
                    return True
                for Item in Module.LibraryClasses.LibraryList:
                    if Inf == Item.FilePath:
                        return True
            for Library in Dsc.Platform.Libraries.LibraryList:
                if Inf == Library.FilePath and Arch in Library.SupArchList:
                    return True

        return False

    ## Show all content of the workspacebuild
    #
    # Print each item of the workspacebuild with (Key = Value) pair
    #
    def ShowWorkspaceBuild(self):
        print self.DscDatabase
        print self.InfDatabase
        print self.DecDatabase
        print 'SupArchList', self.SupArchList
        print 'BuildTarget', self.BuildTarget
        print 'SkuId', self.SkuId

        for Arch in self.SupArchList:
            print Arch
            print 'Platform'
            for Platform in self.Build[Arch].PlatformDatabase.keys():
                P = self.Build[Arch].PlatformDatabase[Platform]
                print 'DescFilePath = ', P.DescFilePath
                print 'PlatformName = ', P.PlatformName
                print 'Guid = ', P.Guid
                print 'Version = ', P.Version
                print 'OutputDirectory = ', P.OutputDirectory
                print 'FlashDefinition = ', P.FlashDefinition
                print 'SkuIds = ', P.SkuIds
                print 'Modules = ', P.Modules
                print 'LibraryClasses = ', P.LibraryClasses
                print 'Pcds = ', P.Pcds
                for item in P.Pcds.keys():
                    print P.Pcds[item]
                print 'BuildOptions = ', P.BuildOptions
                print ''
            # End of Platform

            print 'package'
            for Package in self.Build[Arch].PackageDatabase.keys():
                P = self.Build[Arch].PackageDatabase[Package]
                print 'DescFilePath = ', P.DescFilePath
                print 'PackageName = ', P.PackageName
                print 'Guid = ', P.Guid
                print 'Version = ', P.Version
                print 'Protocols = ', P.Protocols
                print 'Ppis = ', P.Ppis
                print 'Guids = ', P.Guids
                print 'Includes = ', P.Includes
                print 'LibraryClasses = ', P.LibraryClasses
                print 'Pcds = ', P.Pcds
                for item in P.Pcds.keys():
                    print P.Pcds[item]
                print ''
            # End of Package

            print 'module'
            for Module in self.Build[Arch].ModuleDatabase.keys():
                P = self.Build[Arch].ModuleDatabase[Module]
                print 'DescFilePath = ', P.DescFilePath
                print 'BaseName = ', P.BaseName
                print 'ModuleType = ', P.ModuleType
                print 'Guid = ', P.Guid
                print 'Version = ', P.Version
                print 'CustomMakefile = ', P.CustomMakefile
                print 'Specification = ', P.Specification
                print 'Shadow = ', P.Shadow
                print 'PcdIsDriver = ', P.PcdIsDriver
                for Lib in P.LibraryClass:
                    print 'LibraryClassDefinition = ', Lib.LibraryClass, 'SupModList = ', Lib.SupModList
                print 'ModuleEntryPointList = ', P.ModuleEntryPointList
                print 'ModuleUnloadImageList = ', P.ModuleUnloadImageList
                print 'ConstructorList = ', P.ConstructorList
                print 'DestructorList = ', P.DestructorList

                print 'Binaries = '
                for item in P.Binaries:
                    print item.BinaryFile, item.FeatureFlag, item.SupArchList
                print 'Sources = '
                for item in P.Sources:
                    print item.SourceFile
                print 'LibraryClasses = ', P.LibraryClasses
                print 'Protocols = ', P.Protocols
                print 'Ppis = ', P.Ppis
                print 'Guids = ', P.Guids
                print 'Includes = ', P.Includes
                print 'Packages = ', P.Packages
                print 'Pcds = ', P.Pcds
                for item in P.Pcds.keys():
                    print P.Pcds[item]
                print 'BuildOptions = ', P.BuildOptions
                print 'Depex = ', P.Depex
                print ''
            # End of Module

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    print 'Start!', time.strftime('%H:%M:%S', time.localtime())
    EdkLogger.Initialize()
    EdkLogger.SetLevel(EdkLogger.QUIET)
    
    W = os.getenv('WORKSPACE')
    Ewb = WorkspaceBuild('Nt32Pkg/Nt32Pkg.dsc', W)
    Ewb.GenBuildDatabase({('PcdDevicePathSupportDevicePathFromText', 'gEfiMdeModulePkgTokenSpaceGuid') : 'KKKKKKKKKKKKKKKKKKKKK'}, ['Test.Inf'])
    print 'Done!', time.strftime('%H:%M:%S', time.localtime())
    Ewb.ShowWorkspaceBuild()
