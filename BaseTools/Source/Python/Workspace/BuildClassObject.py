## @file
# This file is used to define each component of the build database
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

from collections import OrderedDict, namedtuple
from Common.DataType import *
import collections
import re
from collections import OrderedDict
StructPattern = re.compile(r'[_a-zA-Z][0-9A-Za-z_\[\]]*$')
ArrayIndex = re.compile("\[\s*\d{0,1}\s*\]")
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
    def __init__(self, Name = None, Guid = None, Type = None, DatumType = None, Value = None, Token = None, MaxDatumSize = None, SkuInfoList = None, IsOverrided = False, GuidValue = None, validateranges = None, validlists = None, expressions = None, IsDsc = False, UserDefinedDefaultStoresFlag = False):
        self.TokenCName = Name
        self.TokenSpaceGuidCName = Guid
        self.TokenSpaceGuidValue = GuidValue
        self.Type = Type
        self.DatumType = DatumType
        self.DefaultValue = Value
        self.TokenValue = Token
        self.MaxDatumSize = MaxDatumSize
        self.MaxSizeUserSet = None
        self.SkuInfoList = SkuInfoList if SkuInfoList is not None else OrderedDict()
        self.Phase = "DXE"
        self.Pending = False
        self.IsOverrided = IsOverrided
        self.IsFromBinaryInf = False
        self.IsFromDsc = False
        self.validateranges = validateranges if validateranges is not None else []
        self.validlists = validlists if validlists is not None else []
        self.expressions = expressions if expressions is not None else []
        self.DscDefaultValue = None
        self.DscRawValue = {}
        if IsDsc:
            self.DscDefaultValue = Value
        self.PcdValueFromComm = ""
        self.PcdValueFromFdf = ""
        self.CustomAttribute = {}
        self.UserDefinedDefaultStoresFlag = UserDefinedDefaultStoresFlag

    @staticmethod
    def GetPcdMaxSizeWorker(PcdString, MaxSize):
        if PcdString.startswith("{") and PcdString.endswith("}"):
            return  max([len(PcdString.split(",")),MaxSize])

        if PcdString.startswith("\"") or PcdString.startswith("\'"):
            return  max([len(PcdString)-2+1,MaxSize])

        if PcdString.startswith("L\""):
            return  max([2*(len(PcdString)-3+1),MaxSize])

        return max([len(PcdString),MaxSize])

    ## Get the maximum number of bytes
    def GetPcdMaxSize(self):
        if self.DatumType in TAB_PCD_NUMERIC_TYPES:
            return MAX_SIZE_TYPE[self.DatumType]

        MaxSize = int(self.MaxDatumSize, 10) if self.MaxDatumSize else 0
        if self.PcdValueFromFdf:
            MaxSize = self.GetPcdMaxSizeWorker(self.PcdValueFromFdf,MaxSize)
        if self.PcdValueFromComm:
            MaxSize = self.GetPcdMaxSizeWorker(self.PcdValueFromComm,MaxSize)
        if hasattr(self, "DefaultValueFromDec"):
            MaxSize = self.GetPcdMaxSizeWorker(self.DefaultValueFromDec,MaxSize)
        return MaxSize

    ## Get the number of bytes
    def GetPcdSize(self):
        if self.DatumType in TAB_PCD_NUMERIC_TYPES:
            return MAX_SIZE_TYPE[self.DatumType]
        if not self.DefaultValue:
            return 1
        elif self.DefaultValue[0] == 'L':
            return (len(self.DefaultValue) - 2) * 2
        elif self.DefaultValue[0] == '{':
            return len(self.DefaultValue.split(','))
        else:
            return len(self.DefaultValue) - 1


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

class StructurePcd(PcdClassObject):
    def __init__(self, StructuredPcdIncludeFile=None, Packages=None, Name=None, Guid=None, Type=None, DatumType=None, Value=None, Token=None, MaxDatumSize=None, SkuInfoList=None, IsOverrided=False, GuidValue=None, validateranges=None, validlists=None, expressions=None,default_store = TAB_DEFAULT_STORES_DEFAULT):
        if SkuInfoList is None:
            SkuInfoList = {}
        if validateranges is None:
            validateranges = []
        if validlists is None:
            validlists = []
        if expressions is None:
            expressions = []
        if Packages is None:
            Packages = []
        super(StructurePcd, self).__init__(Name, Guid, Type, DatumType, Value, Token, MaxDatumSize, SkuInfoList, IsOverrided, GuidValue, validateranges, validlists, expressions)
        self.StructuredPcdIncludeFile = [] if StructuredPcdIncludeFile is None else StructuredPcdIncludeFile
        self.PackageDecs = Packages
        self.DefaultStoreName = [default_store]
        self.DefaultValues = OrderedDict()
        self.PcdMode = None
        self.SkuOverrideValues = OrderedDict()
        self.StructName = None
        self.PcdDefineLineNo = 0
        self.PkgPath = ""
        self.DefaultValueFromDec = ""
        self.ValueChain = set()
        self.PcdFieldValueFromComm = OrderedDict()
        self.PcdFieldValueFromFdf = OrderedDict()
    def __repr__(self):
        return self.TypeName

    def AddDefaultValue (self, FieldName, Value, FileName="", LineNo=0):
        if FieldName in self.DefaultValues:
            del self.DefaultValues[FieldName]
        self.DefaultValues[FieldName] = [Value.strip(), FileName, LineNo]
        return self.DefaultValues[FieldName]

    def SetDecDefaultValue(self, DefaultValue):
        self.DefaultValueFromDec = DefaultValue
    def AddOverrideValue (self, FieldName, Value, SkuName, DefaultStoreName, FileName="", LineNo=0):
        if SkuName not in self.SkuOverrideValues:
            self.SkuOverrideValues[SkuName] = OrderedDict()
        if DefaultStoreName not in self.SkuOverrideValues[SkuName]:
            self.SkuOverrideValues[SkuName][DefaultStoreName] = OrderedDict()
        if FieldName in self.SkuOverrideValues[SkuName][DefaultStoreName]:
            del self.SkuOverrideValues[SkuName][DefaultStoreName][FieldName]
        self.SkuOverrideValues[SkuName][DefaultStoreName][FieldName] = [Value.strip(), FileName, LineNo]
        return self.SkuOverrideValues[SkuName][DefaultStoreName][FieldName]

    def SetPcdMode (self, PcdMode):
        self.PcdMode = PcdMode

    def copy(self, PcdObject):
        self.TokenCName = PcdObject.TokenCName if PcdObject.TokenCName else self.TokenCName
        self.TokenSpaceGuidCName = PcdObject.TokenSpaceGuidCName if PcdObject.TokenSpaceGuidCName else PcdObject.TokenSpaceGuidCName
        self.TokenSpaceGuidValue = PcdObject.TokenSpaceGuidValue if PcdObject.TokenSpaceGuidValue else self.TokenSpaceGuidValue
        self.Type = PcdObject.Type if PcdObject.Type else self.Type
        self.DatumType = PcdObject.DatumType if PcdObject.DatumType else self.DatumType
        self.DefaultValue = PcdObject.DefaultValue if  PcdObject.DefaultValue else self.DefaultValue
        self.TokenValue = PcdObject.TokenValue if PcdObject.TokenValue else self.TokenValue
        self.MaxDatumSize = PcdObject.MaxDatumSize if PcdObject.MaxDatumSize else self.MaxDatumSize
        self.SkuInfoList = PcdObject.SkuInfoList if PcdObject.SkuInfoList else self.SkuInfoList
        self.Phase = PcdObject.Phase if PcdObject.Phase else self.Phase
        self.Pending = PcdObject.Pending if PcdObject.Pending else self.Pending
        self.IsOverrided = PcdObject.IsOverrided if PcdObject.IsOverrided else self.IsOverrided
        self.IsFromBinaryInf = PcdObject.IsFromBinaryInf if PcdObject.IsFromBinaryInf else self.IsFromBinaryInf
        self.IsFromDsc = PcdObject.IsFromDsc if PcdObject.IsFromDsc else self.IsFromDsc
        self.validateranges = PcdObject.validateranges if PcdObject.validateranges else self.validateranges
        self.validlists = PcdObject.validlists if PcdObject.validlists else self.validlists
        self.expressions = PcdObject.expressions if PcdObject.expressions else self.expressions
        self.DscRawValue = PcdObject.DscRawValue if PcdObject.DscRawValue else self.DscRawValue
        self.PcdValueFromComm = PcdObject.PcdValueFromComm if PcdObject.PcdValueFromComm else self.PcdValueFromComm
        self.PcdValueFromFdf = PcdObject.PcdValueFromFdf if PcdObject.PcdValueFromFdf else self.PcdValueFromFdf
        self.CustomAttribute = PcdObject.CustomAttribute if PcdObject.CustomAttribute else self.CustomAttribute
        self.UserDefinedDefaultStoresFlag = PcdObject.UserDefinedDefaultStoresFlag if PcdObject.UserDefinedDefaultStoresFlag else self.UserDefinedDefaultStoresFlag
        if isinstance(PcdObject, StructurePcd):
            self.StructuredPcdIncludeFile = PcdObject.StructuredPcdIncludeFile if PcdObject.StructuredPcdIncludeFile else self.StructuredPcdIncludeFile
            self.PackageDecs = PcdObject.PackageDecs if PcdObject.PackageDecs else self.PackageDecs
            self.DefaultValues = PcdObject.DefaultValues if PcdObject.DefaultValues else self.DefaultValues
            self.PcdMode = PcdObject.PcdMode if PcdObject.PcdMode else self.PcdMode
            self.DefaultFromDSC=None
            self.DefaultValueFromDec = PcdObject.DefaultValueFromDec if PcdObject.DefaultValueFromDec else self.DefaultValueFromDec
            self.SkuOverrideValues = PcdObject.SkuOverrideValues if PcdObject.SkuOverrideValues else self.SkuOverrideValues
            self.StructName = PcdObject.DatumType if PcdObject.DatumType else self.StructName
            self.PcdDefineLineNo = PcdObject.PcdDefineLineNo if PcdObject.PcdDefineLineNo else self.PcdDefineLineNo
            self.PkgPath = PcdObject.PkgPath if PcdObject.PkgPath else self.PkgPath
            self.ValueChain = PcdObject.ValueChain if PcdObject.ValueChain else self.ValueChain
            self.PcdFieldValueFromComm = PcdObject.PcdFieldValueFromComm if PcdObject.PcdFieldValueFromComm else self.PcdFieldValueFromComm
            self.PcdFieldValueFromFdf = PcdObject.PcdFieldValueFromFdf if PcdObject.PcdFieldValueFromFdf else self.PcdFieldValueFromFdf

LibraryClassObject = namedtuple('LibraryClassObject', ['LibraryClass','SupModList'], verbose=False)

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
        self.LibraryClasses          = OrderedDict()
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
