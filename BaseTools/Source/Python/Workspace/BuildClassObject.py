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

import os

from Common.Misc import sdict
from Common.Misc import RealPath2
from Common.BuildToolError import *

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
# @param GuidValue:          Input value for TokenSpaceGuidValue of Pcd, default is None
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
    def __init__(self, Name = None, Guid = None, Type = None, DatumType = None, Value = None, Token = None, MaxDatumSize = None, SkuInfoList = {}, IsOverrided = False, GuidValue = None):
        self.TokenCName = Name
        self.TokenSpaceGuidCName = Guid
        self.TokenSpaceGuidValue = GuidValue
        self.Type = Type
        self.DatumType = DatumType
        self.DefaultValue = Value
        self.TokenValue = Token
        self.MaxDatumSize = MaxDatumSize
        self.SkuInfoList = SkuInfoList
        self.Phase = "DXE"
        self.Pending = False
        self.IsOverrided = IsOverrided
        
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
        Rtn = Rtn + ', IsOverrided=' + str(self.IsOverrided)

        return Rtn

    ## Override __eq__ function
    #
    # Check whether pcds are the same
    #
    # @retval False The two pcds are different
    # @retval True  The two pcds are the same
    #
    def __eq__(self, Other):
        return Other and self.TokenCName == Other.TokenCName and self.TokenSpaceGuidCName == Other.TokenSpaceGuidCName

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
# @var MetaFile:              To store value for module meta file path
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
        self.MetaFile                = ''
        self.BaseName                = ''
        self.ModuleType              = ''
        self.Guid                    = ''
        self.Version                 = ''
        self.PcdIsDriver             = ''
        self.BinaryModule            = ''
        self.Shadow                  = ''
        self.SourceOverridePath      = ''
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
        self.Depex                   = {}

    ## Convert the class to a string
    #
    #  Convert member MetaFile of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return str(self.MetaFile)

    ## Override __eq__ function
    #
    # Check whether ModuleBuildClassObjects are the same
    #
    # @retval False The two ModuleBuildClassObjects are different
    # @retval True  The two ModuleBuildClassObjects are the same
    #
    def __eq__(self, Other):
        return self.MetaFile == Other

    ## Override __hash__ function
    #
    # Use MetaFile as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.MetaFile)

## PackageBuildClassObject
#
# This Class defines PackageBuildClass
#
# @param object:        Inherited from object class
#
# @var MetaFile:       To store value for package meta file path
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
        self.MetaFile                = ''
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
    #  Convert member MetaFile of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return str(self.MetaFile)

    ## Override __eq__ function
    #
    # Check whether PackageBuildClassObjects are the same
    #
    # @retval False The two PackageBuildClassObjects are different
    # @retval True  The two PackageBuildClassObjects are the same
    #
    def __eq__(self, Other):
        return self.MetaFile == Other

    ## Override __hash__ function
    #
    # Use MetaFile as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.MetaFile)

## PlatformBuildClassObject
#
# This Class defines PlatformBuildClass
#
# @param object:          Inherited from object class
#
# @var MetaFile:         To store value for platform meta-file path
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
        self.MetaFile                = ''
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
    #  Convert member MetaFile of the class to a string
    #
    #  @retval string Formatted String
    #
    def __str__(self):
        return str(self.MetaFile)

    ## Override __eq__ function
    #
    # Check whether PlatformBuildClassObjects are the same
    #
    # @retval False The two PlatformBuildClassObjects are different
    # @retval True  The two PlatformBuildClassObjects are the same
    #
    def __eq__(self, Other):
        return self.MetaFile == Other

    ## Override __hash__ function
    #
    # Use MetaFile as key in hash table
    #
    # @retval string Key for hash table
    #
    def __hash__(self):
        return hash(self.MetaFile)

