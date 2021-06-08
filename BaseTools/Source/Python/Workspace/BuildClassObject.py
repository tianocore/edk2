## @file
# This file is used to define each component of the build database
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from collections import OrderedDict, namedtuple
from Common.DataType import *
import collections
import re
from collections import OrderedDict
from Common.Misc import CopyDict,ArrayIndex
import copy
from CommonDataClass.DataClass import *
import Common.EdkLogger as EdkLogger
import Common.GlobalData as GlobalData
from Common.BuildToolError import OPTION_VALUE_INVALID
from Common.caching import cached_property
StructPattern = re.compile(r'[_a-zA-Z][0-9A-Za-z_\[\]]*$')

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
        self._DatumType = DatumType
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
        self.DscRawValueInfo = {}
        if IsDsc:
            self.DscDefaultValue = Value
        self.PcdValueFromComm = ""
        self.PcdValueFromFdf = ""
        self.PcdValueFromComponents = {} #{ModuleGuid:value, file_path,lineNo}
        self.CustomAttribute = {}
        self.UserDefinedDefaultStoresFlag = UserDefinedDefaultStoresFlag
        self._Capacity = None

    @property
    def Capacity(self):
        if self._Capacity is None:
            self._Capacity = []
            dimension = ArrayIndex.findall(self._DatumType)
            for item in dimension:
                maxsize = item.lstrip("[").rstrip("]").strip()
                if not maxsize:
                    maxsize = "-1"
                maxsize = str(int(maxsize,16)) if maxsize.startswith(("0x","0X")) else maxsize
                self._Capacity.append(maxsize)
            if hasattr(self, "SkuOverrideValues"):
                for sku in self.SkuOverrideValues:
                    for defaultstore in self.SkuOverrideValues[sku]:
                        fields = self.SkuOverrideValues[sku][defaultstore]
                        for demesionattr in fields:
                            fieldinfo = fields[demesionattr]
                            deme = ArrayIndex.findall(demesionattr)
                            for i in range(len(deme)):
                                if int(deme[i].lstrip("[").rstrip("]").strip()) >= int(self._Capacity[i]):
                                    if self._Capacity[i] != "-1":
                                        firstfieldinfo = list(fieldinfo.values())[0]
                                        EdkLogger.error('Build', OPTION_VALUE_INVALID, "For Pcd %s, Array Index exceed the Array size. From %s Line %s \n " %
                                    (".".join((self.TokenSpaceGuidCName, self.TokenCName)), firstfieldinfo[1],firstfieldinfo[2] ))
            if hasattr(self,"DefaultValues"):
                for demesionattr in self.DefaultValues:
                    fieldinfo = self.DefaultValues[demesionattr]
                    deme = ArrayIndex.findall(demesionattr)
                    for i in range(len(deme)):
                        if int(deme[i].lstrip("[").rstrip("]").strip()) >= int(self._Capacity[i]):
                            if self._Capacity[i] != "-1":
                                firstfieldinfo = list(fieldinfo.values())[0]
                                EdkLogger.error('Build', OPTION_VALUE_INVALID, "For Pcd %s, Array Index exceed the Array size. From %s Line %s \n " %
                                    (".".join((self.TokenSpaceGuidCName, self.TokenCName)), firstfieldinfo[1],firstfieldinfo[2] ))
        return self._Capacity

    def PcdArraySize(self):
        if self.Capacity[-1] == "-1":
            return -1
        size = 1
        for de in self.Capacity:
            size = size * int(de)
        return size
    @property
    def DatumType(self):
        return self._DatumType

    @DatumType.setter
    def DatumType(self,DataType):
        self._DatumType = DataType
        self._Capacity = None

    @property
    def BaseDatumType(self):
        if self.IsArray():
            return self._DatumType[:self._DatumType.index("[")]
        else:
            return self._DatumType
    def IsArray(self):
        return True if len(self.Capacity) else False

    def IsAggregateDatumType(self):
        if self.DatumType in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64, TAB_VOID, "BOOLEAN"]:
            return False
        if self.IsArray() or StructPattern.match(self.DatumType):
            return True
        return False

    def IsSimpleTypeArray(self):
        if self.IsArray() and self.BaseDatumType in [TAB_UINT8, TAB_UINT16, TAB_UINT32, TAB_UINT64, "BOOLEAN"]:
            return True
        return False

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
    #  Organize to a single line format string
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

    @cached_property
    def _fullname(self):
        return ".".join((self.TokenSpaceGuidCName,self.TokenCName))

    def __lt__(self,pcd):
        return self._fullname < pcd._fullname
    def __gt__(self,pcd):
        return self._fullname > pcd._fullname

    def sharedcopy(self,new_pcd):
        new_pcd.TokenCName = self.TokenCName
        new_pcd.TokenSpaceGuidCName = self.TokenSpaceGuidCName
        new_pcd.TokenSpaceGuidValue = self.TokenSpaceGuidValue
        new_pcd.Type = self.Type
        new_pcd.DatumType = self.DatumType
        new_pcd.DefaultValue = self.DefaultValue
        new_pcd.TokenValue = self.TokenValue
        new_pcd.MaxDatumSize = self.MaxDatumSize
        new_pcd.MaxSizeUserSet = self.MaxSizeUserSet

        new_pcd.Phase = self.Phase
        new_pcd.Pending = self.Pending
        new_pcd.IsOverrided = self.IsOverrided
        new_pcd.IsFromBinaryInf = self.IsFromBinaryInf
        new_pcd.IsFromDsc = self.IsFromDsc
        new_pcd.PcdValueFromComm = self.PcdValueFromComm
        new_pcd.PcdValueFromFdf = self.PcdValueFromFdf
        new_pcd.UserDefinedDefaultStoresFlag = self.UserDefinedDefaultStoresFlag
        new_pcd.DscRawValue = self.DscRawValue
        new_pcd.DscRawValueInfo = self.DscRawValueInfo
        new_pcd.CustomAttribute = self.CustomAttribute
        new_pcd.validateranges = [item for item in self.validateranges]
        new_pcd.validlists = [item for item in self.validlists]
        new_pcd.expressions = [item for item in self.expressions]
        new_pcd.SkuInfoList = {key: copy.deepcopy(skuobj) for key,skuobj in self.SkuInfoList.items()}
        return new_pcd

    def __deepcopy__(self,memo):
        new_pcd = PcdClassObject()
        self.sharedcopy(new_pcd)
        return new_pcd

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
        self.DefaultValueFromDecInfo = None
        self.ValueChain = set()
        self.PcdFieldValueFromComm = OrderedDict()
        self.PcdFieldValueFromFdf = OrderedDict()
        self.DefaultFromDSC=None
        self.PcdFiledValueFromDscComponent = OrderedDict()
    def __repr__(self):
        return self.TypeName

    def AddDefaultValue (self, FieldName, Value, FileName="", LineNo=0,DimensionAttr ="-1"):
        if DimensionAttr not in self.DefaultValues:
            self.DefaultValues[DimensionAttr] = collections.OrderedDict()
        if FieldName in self.DefaultValues[DimensionAttr]:
            del self.DefaultValues[DimensionAttr][FieldName]
        self.DefaultValues[DimensionAttr][FieldName] = [Value.strip(), FileName, LineNo]
        return self.DefaultValues[DimensionAttr][FieldName]

    def SetDecDefaultValue(self, DefaultValue,decpath=None,lineno=None):
        self.DefaultValueFromDec = DefaultValue
        self.DefaultValueFromDecInfo = (decpath,lineno)
    def AddOverrideValue (self, FieldName, Value, SkuName, DefaultStoreName, FileName="", LineNo=0, DimensionAttr = '-1'):
        if SkuName not in self.SkuOverrideValues:
            self.SkuOverrideValues[SkuName] = OrderedDict()
        if DefaultStoreName not in self.SkuOverrideValues[SkuName]:
            self.SkuOverrideValues[SkuName][DefaultStoreName] = OrderedDict()
        if DimensionAttr not in self.SkuOverrideValues[SkuName][DefaultStoreName]:
            self.SkuOverrideValues[SkuName][DefaultStoreName][DimensionAttr] = collections.OrderedDict()
        if FieldName in self.SkuOverrideValues[SkuName][DefaultStoreName][DimensionAttr]:
            del self.SkuOverrideValues[SkuName][DefaultStoreName][DimensionAttr][FieldName]
        self.SkuOverrideValues[SkuName][DefaultStoreName][DimensionAttr][FieldName] = [Value.strip(), FileName, LineNo]
        return self.SkuOverrideValues[SkuName][DefaultStoreName][DimensionAttr][FieldName]

    def AddComponentOverrideValue(self,FieldName, Value, ModuleGuid, FileName="", LineNo=0, DimensionAttr = '-1'):
        self.PcdFiledValueFromDscComponent.setdefault(ModuleGuid, OrderedDict())
        self.PcdFiledValueFromDscComponent[ModuleGuid].setdefault(DimensionAttr,OrderedDict())
        self.PcdFiledValueFromDscComponent[ModuleGuid][DimensionAttr][FieldName] =  [Value.strip(), FileName, LineNo]
        return self.PcdFiledValueFromDscComponent[ModuleGuid][DimensionAttr][FieldName]

    def SetPcdMode (self, PcdMode):
        self.PcdMode = PcdMode

    def copy(self, PcdObject):
        self.TokenCName = PcdObject.TokenCName if PcdObject.TokenCName else self.TokenCName
        self.TokenSpaceGuidCName = PcdObject.TokenSpaceGuidCName if PcdObject.TokenSpaceGuidCName else PcdObject.TokenSpaceGuidCName
        self.TokenSpaceGuidValue = PcdObject.TokenSpaceGuidValue if PcdObject.TokenSpaceGuidValue else self.TokenSpaceGuidValue
        self.Type = PcdObject.Type if PcdObject.Type else self.Type
        self._DatumType = PcdObject.DatumType if PcdObject.DatumType else self.DatumType
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
        self.DscRawValueInfo = PcdObject.DscRawValueInfo if PcdObject.DscRawValueInfo else self.DscRawValueInfo
        self.PcdValueFromComm = PcdObject.PcdValueFromComm if PcdObject.PcdValueFromComm else self.PcdValueFromComm
        self.PcdValueFromFdf = PcdObject.PcdValueFromFdf if PcdObject.PcdValueFromFdf else self.PcdValueFromFdf
        self.CustomAttribute = PcdObject.CustomAttribute if PcdObject.CustomAttribute else self.CustomAttribute
        self.UserDefinedDefaultStoresFlag = PcdObject.UserDefinedDefaultStoresFlag if PcdObject.UserDefinedDefaultStoresFlag else self.UserDefinedDefaultStoresFlag
        if isinstance(PcdObject, StructurePcd):
            self.StructuredPcdIncludeFile = PcdObject.StructuredPcdIncludeFile if PcdObject.StructuredPcdIncludeFile else self.StructuredPcdIncludeFile
            self.PackageDecs = PcdObject.PackageDecs if PcdObject.PackageDecs else self.PackageDecs
            self.DefaultValues = PcdObject.DefaultValues if PcdObject.DefaultValues else self.DefaultValues
            self.PcdMode = PcdObject.PcdMode if PcdObject.PcdMode else self.PcdMode
            self.DefaultValueFromDec = PcdObject.DefaultValueFromDec if PcdObject.DefaultValueFromDec else self.DefaultValueFromDec
            self.DefaultValueFromDecInfo = PcdObject.DefaultValueFromDecInfo if PcdObject.DefaultValueFromDecInfo else self.DefaultValueFromDecInfo
            self.SkuOverrideValues = PcdObject.SkuOverrideValues if PcdObject.SkuOverrideValues else self.SkuOverrideValues
            self.StructName = PcdObject.DatumType if PcdObject.DatumType else self.StructName
            self.PcdDefineLineNo = PcdObject.PcdDefineLineNo if PcdObject.PcdDefineLineNo else self.PcdDefineLineNo
            self.PkgPath = PcdObject.PkgPath if PcdObject.PkgPath else self.PkgPath
            self.ValueChain = PcdObject.ValueChain if PcdObject.ValueChain else self.ValueChain
            self.PcdFieldValueFromComm = PcdObject.PcdFieldValueFromComm if PcdObject.PcdFieldValueFromComm else self.PcdFieldValueFromComm
            self.PcdFieldValueFromFdf = PcdObject.PcdFieldValueFromFdf if PcdObject.PcdFieldValueFromFdf else self.PcdFieldValueFromFdf
            self.PcdFiledValueFromDscComponent = PcdObject.PcdFiledValueFromDscComponent if PcdObject.PcdFiledValueFromDscComponent else self.PcdFiledValueFromDscComponent

    def __deepcopy__(self,memo):
        new_pcd = StructurePcd()
        self.sharedcopy(new_pcd)

        new_pcd.DefaultValueFromDec = self.DefaultValueFromDec
        new_pcd.DefaultValueFromDecInfo = self.DefaultValueFromDecInfo
        new_pcd.PcdMode = self.PcdMode
        new_pcd.StructName = self.DatumType
        new_pcd.PcdDefineLineNo = self.PcdDefineLineNo
        new_pcd.PkgPath = self.PkgPath
        new_pcd.StructuredPcdIncludeFile = [item for item in self.StructuredPcdIncludeFile]
        new_pcd.PackageDecs = [item for item in self.PackageDecs]
        new_pcd.DefaultValues = CopyDict(self.DefaultValues)
        new_pcd.DefaultFromDSC=CopyDict(self.DefaultFromDSC)
        new_pcd.SkuOverrideValues = CopyDict(self.SkuOverrideValues)
        new_pcd.PcdFieldValueFromComm = CopyDict(self.PcdFieldValueFromComm)
        new_pcd.PcdFieldValueFromFdf = CopyDict(self.PcdFieldValueFromFdf)
        new_pcd.PcdFiledValueFromDscComponent = CopyDict(self.PcdFiledValueFromDscComponent)
        new_pcd.ValueChain = {item for item in self.ValueChain}
        return new_pcd

LibraryClassObject = namedtuple('LibraryClassObject', ['LibraryClass','SupModList'])

class BuildData(object):
    # dict used to convert PCD type in database to string used by build tool

    _PCD_TYPE_STRING_ = {
        MODEL_PCD_FIXED_AT_BUILD        :   TAB_PCDS_FIXED_AT_BUILD,
        MODEL_PCD_PATCHABLE_IN_MODULE   :   TAB_PCDS_PATCHABLE_IN_MODULE,
        MODEL_PCD_FEATURE_FLAG          :   TAB_PCDS_FEATURE_FLAG,
        MODEL_PCD_DYNAMIC               :   TAB_PCDS_DYNAMIC,
        MODEL_PCD_DYNAMIC_DEFAULT       :   TAB_PCDS_DYNAMIC,
        MODEL_PCD_DYNAMIC_HII           :   TAB_PCDS_DYNAMIC_HII,
        MODEL_PCD_DYNAMIC_VPD           :   TAB_PCDS_DYNAMIC_VPD,
        MODEL_PCD_DYNAMIC_EX            :   TAB_PCDS_DYNAMIC_EX,
        MODEL_PCD_DYNAMIC_EX_DEFAULT    :   TAB_PCDS_DYNAMIC_EX,
        MODEL_PCD_DYNAMIC_EX_HII        :   TAB_PCDS_DYNAMIC_EX_HII,
        MODEL_PCD_DYNAMIC_EX_VPD        :   TAB_PCDS_DYNAMIC_EX_VPD,
    }

    def UpdatePcdTypeDict(self):
        if GlobalData.gCommandLineDefines.get(TAB_DSC_DEFINES_PCD_DYNAMIC_AS_DYNAMICEX,"FALSE").upper() == "TRUE":
            self._PCD_TYPE_STRING_ = {
                MODEL_PCD_FIXED_AT_BUILD        :   TAB_PCDS_FIXED_AT_BUILD,
                MODEL_PCD_PATCHABLE_IN_MODULE   :   TAB_PCDS_PATCHABLE_IN_MODULE,
                MODEL_PCD_FEATURE_FLAG          :   TAB_PCDS_FEATURE_FLAG,
                MODEL_PCD_DYNAMIC               :   TAB_PCDS_DYNAMIC_EX,
                MODEL_PCD_DYNAMIC_DEFAULT       :   TAB_PCDS_DYNAMIC_EX,
                MODEL_PCD_DYNAMIC_HII           :   TAB_PCDS_DYNAMIC_EX_HII,
                MODEL_PCD_DYNAMIC_VPD           :   TAB_PCDS_DYNAMIC_EX_VPD,
                MODEL_PCD_DYNAMIC_EX            :   TAB_PCDS_DYNAMIC_EX,
                MODEL_PCD_DYNAMIC_EX_DEFAULT    :   TAB_PCDS_DYNAMIC_EX,
                MODEL_PCD_DYNAMIC_EX_HII        :   TAB_PCDS_DYNAMIC_EX_HII,
                MODEL_PCD_DYNAMIC_EX_VPD        :   TAB_PCDS_DYNAMIC_EX_VPD,
            }

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
class ModuleBuildClassObject(BuildData):
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
        self.StrPcdSet               = []
        self.StrPcdOverallValue      = {}

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
class PackageBuildClassObject(BuildData):
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
class PlatformBuildClassObject(BuildData):
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
