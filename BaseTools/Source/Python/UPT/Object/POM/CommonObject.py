## @file
# This file is used to define common items of class object
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
Common Object
'''
from Library.DataType import TAB_LANGUAGE_EN_US

## HelpTextObject
#
# @param object:       Inherited from object class
#
class HelpTextObject(object):
    def __init__(self):
        self.HelpText = TextObject()

    def SetHelpText(self, HelpText):
        self.HelpText = HelpText

    def GetHelpText(self):
        return self.HelpText

## HelpTextListObject
#
# @param object:       Inherited from object class
#
class HelpTextListObject(object):
    def __init__(self):
        self.HelpTextList = []

    def SetHelpTextList(self, HelpTextList):
        self.HelpTextList = HelpTextList

    def GetHelpTextList(self):
        return self.HelpTextList

## PromptListObject
#
# @param object:       Inherited from object class
#
class PromptListObject(object):
    def __init__(self):
        self.PromptList = []

    def SetPromptList(self, PromptList):
        self.PromptList = PromptList

    def GetPromptList(self):
        return self.PromptList

## CommonPropertiesObject
#
# This class defined common attribution used in Module/Platform/Package files
#
# @param object:       Inherited from object class
# @param Usage:        Input value for Usage, default is []
# @param FeatureFlag:  Input value for FeatureFalg, default is ''
# @param SupArchList:  Input value for SupArchList, default is []
# @param HelpText:     Input value for HelpText, default is ''
# @param HelpTextList: Input value for HelpTextList, default is []
#
class CommonPropertiesObject(HelpTextObject, HelpTextListObject):
    def __init__(self):
        self.Usage = []
        self.FeatureFlag = ''
        self.SupArchList = []
        self.GuidValue = ''
        HelpTextObject.__init__(self)
        HelpTextListObject.__init__(self)

    def SetUsage(self, Usage):
        self.Usage = Usage

    def GetUsage(self):
        return self.Usage

    def SetFeatureFlag(self, FeatureFlag):
        self.FeatureFlag = FeatureFlag

    def GetFeatureFlag(self):
        return self.FeatureFlag

    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList

    def GetSupArchList(self):
        return self.SupArchList

    def SetGuidValue(self, GuidValue):
        self.GuidValue = GuidValue

    def GetGuidValue(self):
        return self.GuidValue

## CommonHeaderObject
#
# This class defined common header items used in Module/Platform/Package files
#
# @param object:          Inherited from object class
#
class CommonHeaderObject(object):
    def __init__(self):
        self.AbstractList = []
        self.DescriptionList = []
        self.CopyrightList = []
        self.LicenseList = []

    def SetAbstract(self, Abstract):
        if isinstance(Abstract, list):
            self.AbstractList = Abstract
        else:
            self.AbstractList.append(Abstract)

    def GetAbstract(self):
        return self.AbstractList

    def SetDescription(self, Description):
        if isinstance(Description, list):
            self.DescriptionList = Description
        else:
            self.DescriptionList.append(Description)

    def GetDescription(self):
        return self.DescriptionList

    def SetCopyright(self, Copyright):
        if isinstance(Copyright, list):
            self.CopyrightList = Copyright
        else:
            self.CopyrightList.append(Copyright)

    def GetCopyright(self):
        return self.CopyrightList

    def SetLicense(self, License):
        if isinstance(License, list):
            self.LicenseList = License
        else:
            self.LicenseList.append(License)

    def GetLicense(self):
        return self.LicenseList

## BinaryHeaderObject
#
# This class defined Binary header items used in Module/Platform/Package files
#
# @param object:          Inherited from object class
#
class BinaryHeaderObject(object):
    def __init__(self):
        self.BinaryHeaderAbstractList = []
        self.BinaryHeaderDescriptionList = []
        self.BinaryHeaderCopyrightList = []
        self.BinaryHeaderLicenseList = []

    def SetBinaryHeaderAbstract(self, Abstract):
        if isinstance(Abstract, list) and Abstract:
            self.BinaryHeaderAbstractList = Abstract
        elif isinstance(Abstract, tuple) and Abstract[1]:
            self.BinaryHeaderAbstractList.append(Abstract)

    def GetBinaryHeaderAbstract(self):
        return self.BinaryHeaderAbstractList

    def SetBinaryHeaderDescription(self, Description):
        if isinstance(Description, list) and Description:
            self.BinaryHeaderDescriptionList = Description
        elif isinstance(Description, tuple) and Description[1]:
            self.BinaryHeaderDescriptionList.append(Description)

    def GetBinaryHeaderDescription(self):
        return self.BinaryHeaderDescriptionList

    def SetBinaryHeaderCopyright(self, Copyright):
        if isinstance(Copyright, list) and Copyright:
            self.BinaryHeaderCopyrightList = Copyright
        elif isinstance(Copyright, tuple) and Copyright[1]:
            self.BinaryHeaderCopyrightList.append(Copyright)

    def GetBinaryHeaderCopyright(self):
        return self.BinaryHeaderCopyrightList

    def SetBinaryHeaderLicense(self, License):
        if isinstance(License, list) and License:
            self.BinaryHeaderLicenseList = License
        elif isinstance(License, tuple) and License[1]:
            self.BinaryHeaderLicenseList.append(License)

    def GetBinaryHeaderLicense(self):
        return self.BinaryHeaderLicenseList

## ClonedRecordObject
#
# This class defined ClonedRecord items used in Module/Platform/Package files
#
# @param object:        Inherited from object class
#
class ClonedRecordObject(object):
    def __init__(self):
        self.IdNum = 0
        self.FarGuid = ''
        self.PackageGuid = ''
        self.PackageVersion = ''
        self.ModuleGuid = ''
        self.ModuleVersion = ''

    def SetId(self, IdNo):
        self.IdNum = IdNo

    def GetId(self):
        return self.IdNum

    def SetFarGuid(self, FarGuid):
        self.FarGuid = FarGuid

    def GetFarGuid(self):
        return self.FarGuid

    def SetPackageGuid(self, PackageGuid):
        self.PackageGuid = PackageGuid

    def GetPackageGuid(self):
        return self.PackageGuid

    def SetPackageVersion(self, PackageVersion):
        self.PackageVersion = PackageVersion

    def GetPackageVersion(self):
        return self.PackageVersion

    def SetModuleGuid(self, ModuleGuid):
        self.ModuleGuid = ModuleGuid

    def GetModuleGuid(self):
        return self.ModuleGuid

    def SetModuleVersion(self, ModuleVersion):
        self.ModuleVersion = ModuleVersion

    def GetModuleVersion(self):
        return self.ModuleVersion

## TextObject
#
# This class defined Text item used in PKG file
#
# @param object:     Inherited from object class
#
class TextObject(object):
    def __init__(self):
        self.Lang = TAB_LANGUAGE_EN_US
        self.String = ''

    def SetLang(self, Lang):
        self.Lang = Lang

    def GetLang(self):
        return self.Lang

    def SetString(self, String):
        self.String = String

    def GetString(self):
        return self.String

## FileNameObject
#
# This class defined File item used in module, for binary files
#
# @param CommonPropertiesObject:   Inherited from CommonPropertiesObject class
#
class FileNameObject(CommonPropertiesObject):
    def __init__(self):
        self.FileType = ''
        self.Filename = ''
        CommonPropertiesObject.__init__(self)

    def SetFileType(self, FileType):
        self.FileType = FileType

    def GetFileType(self):
        return self.FileType

    def SetFilename(self, Filename):
        self.Filename = Filename

    def GetFilename(self):
        return self.Filename

## FileObject
#
# This class defined File item used in PKG file
#
# @param object:   Inherited from object class
#
class FileObject(object):
    def __init__(self):
        self.Executable = ''
        self.Uri = ''
        self.OsType = ''

    def SetExecutable(self, Executable):
        self.Executable = Executable

    def GetExecutable(self):
        return self.Executable

    def SetURI(self, URI):
        self.Uri = URI

    def GetURI(self):
        return self.Uri

    def SetOS(self, OsType):
        self.OsType = OsType

    def GetOS(self):
        return self.OsType

##
# MiscFileObject is used for xml
#
# @param CommonHeaderObject:   Inherited from CommonHeaderObject class
#
class MiscFileObject(CommonHeaderObject):
    def __init__(self):
        self.Name = ''
        self.FileList = []
        CommonHeaderObject.__init__(self)

    def SetName(self, Name):
        self.Name = Name

    def GetName(self):
        return self.Name

    def SetFileList(self, FileList):
        self.FileList = FileList

    def GetFileList(self):
        return self.FileList

##
# ToolsObject
#
class ToolsObject(MiscFileObject):
    pass

## GuidVersionObject
#
# This class defined GUID/Version items used in PKG file
#
# @param object:     Inherited from object class
#
class GuidVersionObject(object):
    def __init__(self):
        self.Guid = ''
        self.Version = ''

    def SetGuid(self, Guid):
        self.Guid = Guid

    def GetGuid(self):
        return self.Guid

    def SetVersion(self, Version):
        self.Version = Version

    def GetVersion(self):
        return self.Version

## IdentificationObject
#
# This class defined Identification items used in Module/Platform/Package files
#
# @param object:    Inherited from object class
#
class IdentificationObject(GuidVersionObject):
    def __init__(self):
        self.Name = ''
        self.BaseName = ''
        self.FileName = ''
        self.FullPath = ''
        self.RelaPath = ''
        self.PackagePath = ''
        self.ModulePath = ''
        self.CombinePath = ''
        GuidVersionObject.__init__(self)

    def SetName(self, Name):
        self.Name = Name

    def GetName(self):
        return self.Name

    def SetBaseName(self, BaseName):
        self.BaseName = BaseName

    def GetBaseName(self):
        return self.BaseName

    def SetFileName(self, FileName):
        self.FileName = FileName

    def GetFileName(self):
        return self.FileName

    def SetFullPath(self, FullPath):
        self.FullPath = FullPath

    def GetFullPath(self):
        return self.FullPath

    def SetRelaPath(self, RelaPath):
        self.RelaPath = RelaPath

    def GetRelaPath(self):
        return self.RelaPath

    def SetPackagePath(self, PackagePath):
        self.PackagePath = PackagePath

    def GetPackagePath(self):
        return self.PackagePath

    def SetModulePath(self, ModulePath):
        self.ModulePath = ModulePath

    def GetModulePath(self):
        return self.ModulePath

    def SetCombinePath(self, CombinePath):
        self.CombinePath = CombinePath

    def GetCombinePath(self):
        return self.CombinePath

## GuidProtocolPpiCommonObject
#
# This class defined Guid, Protocol and Ppi like items used in
# Module/Platform/Package files
#
# @param CommonPropertiesObject:    Inherited from CommonPropertiesObject class
#
class GuidProtocolPpiCommonObject(CommonPropertiesObject):
    def __init__(self):
        self.Name = ''
        self.CName = ''
        self.Guid = ''
        self.SupModuleList = []
        CommonPropertiesObject.__init__(self)

    def SetName(self, Name):
        self.Name = Name

    def GetName(self):
        return self.Name

    def SetCName(self, CName):
        self.CName = CName

    def GetCName(self):
        return self.CName

    def SetGuid(self, Guid):
        self.Guid = Guid

    def GetGuid(self):
        return self.Guid

    def SetSupModuleList(self, SupModuleList):
        self.SupModuleList = SupModuleList

    def GetSupModuleList(self):
        return self.SupModuleList

## GuidObject
#
# This class defined Guid item used in Module/Platform/Package files
#
# @param GuidProtocolPpiCommonObject:  GuidProtocolPpiCommonObject
#
class GuidObject(GuidProtocolPpiCommonObject):
    def __init__(self):
        self.VariableName = ''
        self.GuidTypeList = []
        GuidProtocolPpiCommonObject.__init__(self)
    def SetVariableName(self, VariableName):
        self.VariableName = VariableName

    def GetVariableName(self):
        return self.VariableName

    def SetGuidTypeList(self, GuidTypeList):
        self.GuidTypeList = GuidTypeList

    def GetGuidTypeList(self):
        return self.GuidTypeList

## ProtocolObject
#
# This class defined Protocol item used in Module/Platform/Package files
#
# @param GuidProtocolPpiCommonObject:  Inherited from
#                                      GuidProtocolPpiCommonObject
#
class ProtocolObject(GuidProtocolPpiCommonObject):
    def __init__(self):
        self.Notify = False
        GuidProtocolPpiCommonObject.__init__(self)
    def SetNotify(self, Notify):
        self.Notify = Notify

    def GetNotify(self):
        return self.Notify

## PpiObject
#
# This class defined Ppi item used in Module/Platform/Package files
#
# @param GuidProtocolPpiCommonObject:  Inherited from
#                                      GuidProtocolPpiCommonObject
#
class PpiObject(GuidProtocolPpiCommonObject):
    def __init__(self):
        self.Notify = False
        GuidProtocolPpiCommonObject.__init__(self)
    def SetNotify(self, Notify):
        self.Notify = Notify

    def GetNotify(self):
        return self.Notify

## DefineObject
#
# This class defined item DEFINE used in Module/Platform/Package files
#
# @param object:  Inherited from object class
#
class DefineClass(object):
    def __init__(self):
        self.Define = {}

## UserExtensionObject
#
# @param object:  Inherited from object class
#
class UserExtensionObject(object):
    def __init__(self):
        self.UserID = ''
        self.Identifier = ''
        self.BinaryAbstractList = []
        self.BinaryDescriptionList = []
        self.BinaryCopyrightList = []
        self.BinaryLicenseList = []
        self.UniLangDefsList = []
        #
        # { Statement : Arch , ... }
        #
        self.DefinesDict = {}
        #
        # { Arch : Statement , ... }
        #
        self.BuildOptionDict = {}
        self.IncludesDict = {}
        self.SourcesDict = {}
        self.BinariesDict = {}
        #
        # UserExtension statement from meta-data file [UserExtension] section
        #
        self.Statement = ''
        self.SupArchList = []

    def SetStatement(self, Statement):
        self.Statement = Statement

    def GetStatement(self):
        return self.Statement

    def SetSupArchList(self, ArchList):
        self.SupArchList = ArchList

    def GetSupArchList(self):
        return self.SupArchList

    def SetUserID(self, UserID):
        self.UserID = UserID

    def GetUserID(self):
        return self.UserID

    def SetIdentifier(self, Identifier):
        self.Identifier = Identifier

    def GetIdentifier(self):
        return self.Identifier

    def SetUniLangDefsList(self, UniLangDefsList):
        self.UniLangDefsList = UniLangDefsList

    def GetUniLangDefsList(self):
        return self.UniLangDefsList

    def SetBinaryAbstract(self, BinaryAbstractList):
        self.BinaryAbstractList = BinaryAbstractList

    def GetBinaryAbstract(self, Lang=None):
        if Lang:
            for (Key, Value) in self.BinaryAbstractList:
                if Key == Lang:
                    return Value
            return None
        else:
            return self.BinaryAbstractList

    def SetBinaryDescription(self, BinaryDescriptionList):
        self.BinaryDescriptionList = BinaryDescriptionList

    def GetBinaryDescription(self, Lang=None):
        if Lang:
            for (Key, Value) in self.BinaryDescriptionList:
                if Key == Lang:
                    return Value
            return None
        else:
            return self.BinaryDescriptionList

    def SetBinaryCopyright(self, BinaryCopyrightList):
        self.BinaryCopyrightList = BinaryCopyrightList

    def GetBinaryCopyright(self, Lang=None):
        if Lang:
            for (Key, Value) in self.BinaryCopyrightList:
                if Key == Lang:
                    return Value
            return None
        else:
            return self.BinaryCopyrightList

    def SetBinaryLicense(self, BinaryLicenseList):
        self.BinaryLicenseList = BinaryLicenseList

    def GetBinaryLicense(self, Lang=None):
        if Lang:
            for (Key, Value) in self.BinaryLicenseList:
                if Key == Lang:
                    return Value
            return None
        else:
            return self.BinaryLicenseList

    def SetDefinesDict(self, DefinesDict):
        self.DefinesDict = DefinesDict

    def GetDefinesDict(self):
        return self.DefinesDict

    def SetBuildOptionDict(self, BuildOptionDict):
        self.BuildOptionDict = BuildOptionDict

    def GetBuildOptionDict(self):
        return self.BuildOptionDict

    def SetIncludesDict(self, IncludesDict):
        self.IncludesDict = IncludesDict

    def GetIncludesDict(self):
        return self.IncludesDict

    def SetSourcesDict(self, SourcesDict):
        self.SourcesDict = SourcesDict

    def GetSourcesDict(self):
        return self.SourcesDict

    def SetBinariesDict(self, BinariesDict):
        self.BinariesDict = BinariesDict

    def GetBinariesDict(self):
        return self.BinariesDict

## LibraryClassObject
#
# This class defined Library item used in Module/Platform/Package files
#
# @param CommonPropertiesObject:  Inherited from CommonPropertiesObject class
#
class LibraryClassObject(CommonPropertiesObject):
    def __init__(self):
        self.LibraryClass = ''
        self.IncludeHeader = ''
        self.SupModuleList = []
        self.RecommendedInstance = GuidVersionObject()
        CommonPropertiesObject.__init__(self)

    def SetLibraryClass(self, LibraryClass):
        self.LibraryClass = LibraryClass

    def GetLibraryClass(self):
        return self.LibraryClass

    def SetSupModuleList(self, SupModuleList):
        self.SupModuleList = SupModuleList

    def GetSupModuleList(self):
        return self.SupModuleList

    def SetIncludeHeader(self, IncludeHeader):
        self.IncludeHeader = IncludeHeader

    def GetIncludeHeader(self):
        return self.IncludeHeader

    def SetRecommendedInstance(self, RecommendedInstance):
        self.RecommendedInstance = RecommendedInstance

    def GetRecommendedInstance(self):
        return self.RecommendedInstance


## PcdErrorObject
#
# @param object:  Inherited from object class
#
class PcdErrorObject(object):
    def __init__(self):
        self.ValidValue = ''
        self.ValidValueLang = ''
        self.ValidValueRange = ''
        self.Expression = ''
        self.ErrorNumber = ''
        self.ErrorMessageList = []
        self.TokenSpaceGuidCName = ''
        self.CName = ''
        self.FileLine = ''
        self.LineNum = 0

    def SetValidValue(self, ValidValue):
        self.ValidValue = ValidValue

    def GetValidValue(self):
        return self.ValidValue

    def SetValidValueLang(self, ValidValueLang):
        self.ValidValueLang = ValidValueLang

    def GetValidValueLang(self):
        return self.ValidValueLang

    def SetValidValueRange(self, ValidValueRange):
        self.ValidValueRange = ValidValueRange

    def GetValidValueRange(self):
        return self.ValidValueRange

    def SetExpression(self, Expression):
        self.Expression = Expression

    def GetExpression(self):
        return self.Expression

    def SetErrorNumber(self, ErrorNumber):
        self.ErrorNumber = ErrorNumber

    def GetErrorNumber(self):
        return self.ErrorNumber

    def SetErrorMessageList(self, ErrorMessageList):
        self.ErrorMessageList = ErrorMessageList

    def GetErrorMessageList(self):
        return self.ErrorMessageList

    def SetTokenSpaceGuidCName(self, TokenSpaceGuidCName):
        self.TokenSpaceGuidCName = TokenSpaceGuidCName

    def GetTokenSpaceGuidCName(self):
        return self.TokenSpaceGuidCName

    def SetCName(self, CName):
        self.CName = CName

    def GetCName(self):
        return self.CName

    def SetFileLine(self, FileLine):
        self.FileLine = FileLine

    def GetFileLine(self):
        return self.FileLine

    def SetLineNum(self, LineNum):
        self.LineNum = LineNum

    def GetLineNum(self):
        return self.LineNum


## IncludeObject
#
# This class defined Include item used in Module/Platform/Package files
#
# @param CommonPropertiesObject:  Inherited from CommonPropertiesObject class
#
class IncludeObject(CommonPropertiesObject):
    def __init__(self):
        self.FilePath = ''
        self.ModuleType = ''
        self.SupModuleList = []
        self.Comment = ''
        CommonPropertiesObject.__init__(self)

    def SetFilePath(self, FilePath):
        self.FilePath = FilePath

    def GetFilePath(self):
        return self.FilePath

    def SetModuleType(self, ModuleType):
        self.ModuleType = ModuleType

    def GetModuleType(self):
        return self.ModuleType

    def SetSupModuleList(self, SupModuleList):
        self.SupModuleList = SupModuleList

    def GetSupModuleList(self):
        return self.SupModuleList

    def SetComment(self, Comment):
        self.Comment = Comment

    def GetComment(self):
        return self.Comment

## PcdObject
#
# This class defined Pcd item used in Module/Platform/Package files
#
# @param CName:                Input value for CName, default is ''
# @param Token:                Input value for Token, default is ''
# @param TokenSpaceGuidCName:  Input value for TokenSpaceGuidCName, default is
#                              ''
# @param DatumType:            Input value for DatumType, default is ''
# @param MaxDatumSize:         Input value for MaxDatumSize, default is ''
# @param DefaultValue:         Input value for DefaultValue, default is ''
# @param ItemType:             Input value for ItemType, default is ''
# @param ValidUsage:           Input value for ValidUsage, default is []
# @param SkuInfoList:          Input value for SkuInfoList, default is {}
# @param SupModuleList:        Input value for SupModuleList, default is []
#
class PcdObject(CommonPropertiesObject, HelpTextListObject, PromptListObject):
    def __init__(self):
        self.PcdCName = ''
        self.CName = ''
        self.Token = ''
        self.TokenSpaceGuidCName = ''
        self.TokenSpaceGuidValue = ''
        self.DatumType = ''
        self.MaxDatumSize = ''
        self.DefaultValue = ''
        self.Offset = ''
        self.ValidUsage = ''
        self.ItemType = ''
        self.PcdErrorsList = []
        self.SupModuleList = []
        CommonPropertiesObject.__init__(self)
        HelpTextListObject.__init__(self)
        PromptListObject.__init__(self)

    def SetPcdCName(self, PcdCName):
        self.PcdCName = PcdCName

    def GetPcdCName(self):
        return self.PcdCName

    def SetCName(self, CName):
        self.CName = CName

    def GetCName(self):
        return self.CName

    def SetToken(self, Token):
        self.Token = Token

    def GetOffset(self):
        return self.Offset

    def SetOffset(self, Offset):
        self.Offset = Offset

    def GetToken(self):
        return self.Token

    def SetTokenSpaceGuidCName(self, TokenSpaceGuidCName):
        self.TokenSpaceGuidCName = TokenSpaceGuidCName

    def GetTokenSpaceGuidCName(self):
        return self.TokenSpaceGuidCName

    def SetTokenSpaceGuidValue(self, TokenSpaceGuidValue):
        self.TokenSpaceGuidValue = TokenSpaceGuidValue

    def GetTokenSpaceGuidValue(self):
        return self.TokenSpaceGuidValue

    def SetDatumType(self, DatumType):
        self.DatumType = DatumType

    def GetDatumType(self):
        return self.DatumType

    def SetMaxDatumSize(self, MaxDatumSize):
        self.MaxDatumSize = MaxDatumSize

    def GetMaxDatumSize(self):
        return self.MaxDatumSize

    def SetDefaultValue(self, DefaultValue):
        self.DefaultValue = DefaultValue

    def GetDefaultValue(self):
        return self.DefaultValue

    def SetValidUsage(self, ValidUsage):
        self.ValidUsage = ValidUsage

    def GetValidUsage(self):
        return self.ValidUsage

    def SetPcdErrorsList(self, PcdErrorsList):
        self.PcdErrorsList = PcdErrorsList

    def GetPcdErrorsList(self):
        return self.PcdErrorsList

    def SetItemType(self, ItemType):
        self.ItemType = ItemType

    def GetItemType(self):
        return self.ItemType

    def SetSupModuleList(self, SupModuleList):
        self.SupModuleList = SupModuleList

    def GetSupModuleList(self):
        return self.SupModuleList
