## @file
# This file is used to define common items of class object
#
# Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


#
# Generate help text
#
def GenerateHelpText(Text, Lang):
    if Text:
        Ht = HelpTextClass()
        Ht.Lang = Lang
        Ht.String = Text
    
        return Ht
    
    return None

## CommonClass
#
# This class defined common items used in Module/Platform/Package files
# 
# @param object:       Inherited from object class
# @param Usage:        Input value for Usage, default is [] 
# @param FeatureFlag:  Input value for FeatureFalg, default is ''
# @param SupArchList:  Input value for SupArchList, default is []
# @param HelpText:     Input value for HelpText, default is ''
#
# @var Usage:          To store value for Usage, selection scope is in below list
#                      ALWAYS_CONSUMED | SOMETIMES_CONSUMED | ALWAYS_PRODUCED | SOMETIMES_PRODUCED | TO_START | BY_START | PRIVATE
# @var FeatureFlag:    To store value for FeatureFlag
# @var SupArchList:    To store value for SupArchList, selection scope is in below list
#                      EBC | IA32 | X64 | IPF | ARM | PPC
# @var HelpText:       To store value for HelpText
#
class CommonClass(object):
    def __init__(self, Usage = None, FeatureFlag = '', SupArchList = None, HelpText = ''):
        self.Usage = Usage
        if self.Usage == None:
            self.Usage = []
        self.FeatureFlag = FeatureFlag
        self.SupArchList = SupArchList
        if self.SupArchList == None:
            self.SupArchList = []
        self.HelpText = HelpText
        self.HelpTextList = []

## CommonHeaderClass
#
# This class defined common items used in Module/Platform/Package files
# 
# @param object:          Inherited from object class
#
# @var Abstract:          To store value for Abstract
# @var Description:       To store value for Description
# @var Copyright:         To store value for Copyright
# @var License:           To store value for License
# @var Specification:     To store value for Specification
#
class CommonHeaderClass(object):
    def __init__(self):
        self.Abstract = ''
        self.Description = ''
        self.Copyright = ''
        self.License = ''
        self.Specification = {}

## HelpTextClass
#
# This class defined HelpText item used in PKG file
# 
# @param object:     Inherited from object class
#
# @var Lang:         To store value for Lang
# @var String:       To store value for String
#
class HelpTextClass(object):
    def __init__(self):
        self.Lang = ''
        self.String = ''
    
## DefineClass
#
# This class defined item DEFINE used in Module/Platform/Package files
#
# @param object:  Inherited from object class
#
# @var Define:    To store value for Define, it is a set structure as
#                 { (DefineName, Arch) : DefineValue, ... }
#
class DefineClass(object):
    def __init__(self):
        self.Define = {}

## ClonedRecordClass
#
# This class defined ClonedRecord items used in Module/Platform/Package files
# 
# @param object:        Inherited from object class
#
# @var Id:              To store value for Id
# @var FarGuid:         To store value for FarGuid
# @var PackageGuid:     To store value for PackageGuid
# @var PackageVersion:  To store value for PackageVersion
# @var ModuleGuid:      To store value for ModuleGuid
# @var ModuleVersion:   To store value for ModuleVersion
#
class ClonedRecordClass(object):
    def __init__(self):
        self.Id = 0
        self.FarGuid = ''
        self.PackageGuid = ''
        self.PackageVersion = ''
        self.ModuleGuid = ''
        self.ModuleVersion = ''

## IdentificationClass
#
# This class defined Identification items used in Module/Platform/Package files
# 
# @param object:  Inherited from object class
#
# @var Name:      To store value for Name
#                 ModuleName(Inf) / PackageName(Dec) / PlatformName(Dsc)
# @var Guid:      To store value for Guid
# @var Version:   To store value for Version
# @var FileName:  To store value for FileName
# @var FullPath:  To store value for FullPath
#
class IdentificationClass(object):
    def __init__(self):
        self.Name = ''
        self.BaseName = ''
        self.Guid = ''
        self.Version = ''
        self.FileName = ''
        self.FullPath = ''
        self.RelaPath = ''
        self.PackagePath = ''
        self.ModulePath = ''
        self.CombinePath = ''

## IncludeStatementClass
#
# This class defined IncludeFiles item used in Module/Platform/Package files
# 
# @param object:      Inherited from object class
#
# @var IncludeFiles:  To store value for IncludeFiles
#                     It is a set structure as { IncludeFile : [Arch1, Arch2, ...], ... }
#
class IncludeStatementClass(object):
    def __init__(self):
        self.IncludeFiles = {}                             

## GuidProtocolPpiCommonClass
#
# This class defined Guid, Protocol and Ppi like items used in Module/Platform/Package files
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var Name:           To store value for Name
# @var CName:          To store value for CName
# @var Guid:           To store value for Guid
# @var Notify:         To store value for Notify
# @var GuidTypeList:   To store value for GuidTypeList, selection scope is in below list
#                      DATA_HUB_RECORD | EFI_EVENT | EFI_SYSTEM_CONFIGURATION_TABLE | EFI_VARIABLE | GUID | HII_PACKAGE_LIST | HOB | TOKEN_SPACE_GUID
# @var SupModuleList:  To store value for SupModuleList, selection scope is in below list
#                      BASE | SEC | PEI_CORE | PEIM | DXE_CORE | DXE_DRIVER | DXE_RUNTIME_DRIVER | DXE_SAL_DRIVER | DXE_SMM_DRIVER | UEFI_DRIVER | UEFI_APPLICATION | USER_DEFINED | SMM_CORE
#
class GuidProtocolPpiCommonClass(CommonClass):
    def __init__(self):
        self.Name = ''
        self.CName = ''
        self.Guid = ''
        self.VariableName = ''
        self.Notify = False
        self.GuidTypeList = []
        self.GuidTypeLists = []
        self.SupModuleList = []                           
        CommonClass.__init__(self)

## LibraryClassClass
#
# This class defined Library item used in Module/Platform/Package files
# 
# @param CommonClass:               Inherited from CommonClass class
# @param DefineClass:               Inherited from DefineClass class
#
# @var LibraryClass:                To store value for LibraryClass
# @var IncludeHeader:               To store value for IncludeHeader
# @var RecommendedInstanceVersion:  To store value for RecommendedInstanceVersion
# @var RecommendedInstanceGuid:     To store value for RecommendedInstanceGuid
# @var RecommendedInstance:         To store value for RecommendedInstance, selection scope is in below list
#                                   DATA_HUB_RECORD | EFI_EVENT | EFI_SYSTEM_CONFIGURATION_TABLE | EFI_VARIABLE | GUID | HII_PACKAGE_LIST | HOB | TOKEN_SPACE_GUID
# @var SupModuleList:               To store value for SupModuleList, selection scope is in below list
#                                   BASE | SEC | PEI_CORE | PEIM | DXE_CORE | DXE_DRIVER | DXE_RUNTIME_DRIVER | DXE_SAL_DRIVER | DXE_SMM_DRIVER | UEFI_DRIVER | UEFI_APPLICATION | USER_DEFINED | SMM_CORE
#
class LibraryClassClass(CommonClass, DefineClass):
    def __init__(self):
        self.LibraryClass = ''
        self.IncludeHeader = ''
        self.RecommendedInstanceVersion = ''
        self.RecommendedInstanceGuid = ''
        self.RecommendedInstance = ''
        self.SupModuleList = []
        CommonClass.__init__(self)
        DefineClass.__init__(self)

## GuidClass
#
# This class defined Guid item used in Module/Platform/Package files
# 
# @param GuidProtocolPpiCommonClass:  Inherited from GuidProtocolPpiCommonClass class
#
class GuidClass(GuidProtocolPpiCommonClass):
    def __init__(self):
        GuidProtocolPpiCommonClass.__init__(self)

## ProtocolClass
#
# This class defined Protocol item used in Module/Platform/Package files
# 
# @param GuidProtocolPpiCommonClass:  Inherited from GuidProtocolPpiCommonClass class
#
class ProtocolClass(GuidProtocolPpiCommonClass):
    def __init__(self):
        GuidProtocolPpiCommonClass.__init__(self)

## PpiClass
#
# This class defined Ppi item used in Module/Platform/Package files
# 
# @param GuidProtocolPpiCommonClass:  Inherited from GuidProtocolPpiCommonClass class
#
class PpiClass(GuidProtocolPpiCommonClass):        
    def __init__(self):
        GuidProtocolPpiCommonClass.__init__(self)

## SkuInfoClass
#
# This class defined SkuInfo item used in Module/Platform/Package files
# 
# @param object:           Inherited from object class
# @param SkuIdName:        Input value for SkuIdName, default is ''
# @param SkuId:            Input value for SkuId, default is ''
# @param VariableName:     Input value for VariableName, default is ''
# @param VariableGuid:     Input value for VariableGuid, default is ''
# @param VariableOffset:   Input value for VariableOffset, default is ''
# @param HiiDefaultValue:  Input value for HiiDefaultValue, default is ''
# @param VpdOffset:        Input value for VpdOffset, default is ''
# @param DefaultValue:     Input value for DefaultValue, default is ''
#
# @var SkuIdName:          To store value for SkuIdName
# @var SkuId:              To store value for SkuId
# @var VariableName:       To store value for VariableName
# @var VariableGuid:       To store value for VariableGuid
# @var VariableOffset:     To store value for VariableOffset
# @var HiiDefaultValue:    To store value for HiiDefaultValue
# @var VpdOffset:          To store value for VpdOffset
# @var DefaultValue:       To store value for DefaultValue
#
class SkuInfoClass(object):
    def __init__(self, SkuIdName = '', SkuId = '', VariableName = '', VariableGuid = '', VariableOffset = '', 
                 HiiDefaultValue = '', VpdOffset = '', DefaultValue = '', VariableGuidValue = ''):
        self.SkuIdName = SkuIdName
        self.SkuId = SkuId
        
        #
        # Used by Hii
        #
        self.VariableName = VariableName
        self.VariableGuid = VariableGuid
        self.VariableGuidValue = VariableGuidValue
        self.VariableOffset = VariableOffset
        self.HiiDefaultValue = HiiDefaultValue
        
        #
        # Used by Vpd
        #
        self.VpdOffset = VpdOffset
        
        #
        # Used by Default
        #
        self.DefaultValue = DefaultValue
        
    ## Convert the class to a string
    #
    #  Convert each member of the class to string
    #  Organize to a signle line format string
    #
    #  @retval Rtn Formatted String
    #
    def __str__(self):
        Rtn = 'SkuId = ' + str(self.SkuId) + "," + \
                    'SkuIdName = ' + str(self.SkuIdName) + "," + \
                    'VariableName = ' + str(self.VariableName) + "," + \
                    'VariableGuid = ' + str(self.VariableGuid) + "," + \
                    'VariableOffset = ' + str(self.VariableOffset) + "," + \
                    'HiiDefaultValue = ' + str(self.HiiDefaultValue) + "," + \
                    'VpdOffset = ' + str(self.VpdOffset) + "," + \
                    'DefaultValue = ' + str(self.DefaultValue) + ","
        return Rtn
## PcdErrorClass
#
#
#
class PcdErrorClass(object):
    def __init__(self):
        self.ValidValueList = ''
        self.ValidValueListLang = ''
        self.ValidValueRange = ''
        self.Expression = ''
        self.ErrorNumber = ''
        self.ErrorMessage = []

## PcdClass
#
# This class defined Pcd item used in Module/Platform/Package files
# 
# @param CommonClass:          Inherited from CommonClass class
# @param CName:                Input value for CName, default is ''
# @param Token:                Input value for Token, default is ''
# @param TokenSpaceGuidCName:  Input value for TokenSpaceGuidCName, default is ''
# @param DatumType:            Input value for DatumType, default is ''
# @param MaxDatumSize:         Input value for MaxDatumSize, default is ''
# @param DefaultValue:         Input value for DefaultValue, default is ''
# @param ItemType:             Input value for ItemType, default is ''
# @param ValidUsage:           Input value for ValidUsage, default is []
# @param SkuInfoList:          Input value for SkuInfoList, default is {}
# @param SupModuleList:        Input value for SupModuleList, default is []
#
# @var CName:                  To store value for CName
# @var Token:                  To store value for Token
# @var TokenSpaceGuidCName:    To store value for TokenSpaceGuidCName
# @var DatumType:              To store value for DatumType, selection scope is in below list
#                              UINT8 | UINT16 | UINT32 | UINT64 | VOID* | BOOLEAN 
# @var MaxDatumSize:           To store value for MaxDatumSize
# @var DefaultValue:           To store value for DefaultValue
# @var ItemType:               To store value for ItemType, selection scope is in below list
#                              FEATURE_FLAG | FIXED_AT_BUILD | PATCHABLE_IN_MODULE | DYNAMIC | DYNAMIC_EX
# @var ValidUsage:             To store value for ValidUsage, selection scope is in below list
#                              FEATURE_FLAG | FIXED_AT_BUILD | PATCHABLE_IN_MODULE | DYNAMIC | DYNAMIC_EX
# @var SkuInfoList:            To store value for SkuInfoList
#                              It is a set structure as { [SkuIdName] : SkuInfoClass } 
# @var SupModuleList:          To store value for SupModuleList, selection scope is in below list
#                              BASE | SEC | PEI_CORE | PEIM | DXE_CORE | DXE_DRIVER | DXE_RUNTIME_DRIVER | DXE_SAL_DRIVER | DXE_SMM_DRIVER | UEFI_DRIVER | UEFI_APPLICATION | USER_DEFINED | SMM_CORE
#
class PcdClass(CommonClass):
    def __init__(self, CName = '', Token = '', TokenSpaceGuidCName = '', DatumType = '', MaxDatumSize = '', DefaultValue = '', ItemType = '', ValidUsage = None, SkuInfoList = None, SupModuleList = None):
        self.CName = CName
        self.Token = Token
        self.TokenSpaceGuidCName = TokenSpaceGuidCName
        self.DatumType = DatumType
        self.MaxDatumSize = MaxDatumSize
        self.DefaultValue = DefaultValue
        self.ItemType = ItemType
        self.ValidUsage = ValidUsage
        self.PcdItemType = ''
        self.TokenSpaceGuidValue = ''
        self.PcdUsage = ''
        self.PcdCName = ''
        self.Value = ''
        self.Offset = ''
        if self.ValidUsage == None:
            self.ValidUsage = []
        self.SkuInfoList = SkuInfoList
        if self.SkuInfoList  == None:
            self.SkuInfoList  = {}
        self.SupModuleList = SupModuleList
        if self.SupModuleList == None:
            self.SupModuleList = []
        CommonClass.__init__(self)
        self.PcdErrors = []

## BuildOptionClass
#
# This class defined BuildOption item used in Module/Platform/Package files
# 
# @param IncludeStatementClass:  Inherited from IncludeStatementClass class
# @param ToolChainFamily:        Input value for ToolChainFamily, default is ''
# @param ToolChain:              Input value for ToolChain, default is ''
# @param Option:                 Input value for Option, default is ''
#
# @var Statement:                To store value for Statement
#                                It is a string in a special format as "Family:Target_TagName_Tarch_ToolCode_FLAGS = String"
# @var ToolChainFamily:          To store value for ToolChainFamily
# @var ToolChain:                To store value for ToolChain
# @var Option:                   To store value for Option
# @var BuildTarget:              To store value for BuildTarget
# @var TagName:                  To store value for TagName
# @var ToolCode:                 To store value for ToolCode
# @var SupArchList:              To store value for SupArchList, selection scope is in below list
#                                EBC | IA32 | X64 | IPF | ARM | PPC
#
class BuildOptionClass(IncludeStatementClass):
    def __init__(self, ToolChainFamily = '', ToolChain = '', Option = ''):
        IncludeStatementClass.__init__(self)
        self.Statement = ''                               
        self.ToolChainFamily = ToolChainFamily
        self.ToolChain = ToolChain
        self.Option = Option
        self.BuildTarget = ''
        self.TagName = ''
        self.ToolCode = ''
        self.SupArchList = []

## IncludeClass
#
# This class defined Include item used in Module/Platform/Package files
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var FilePath:       To store value for FilePath
# @var ModuleType:     To store value for ModuleType
# @var Comment:        To store value for Comment
#
class IncludeClass(CommonClass):
    def __init__(self):
        self.FilePath = ''
        self.ModuleType = ''
        self.SupModuleList = []
        self.Comment = ''
        CommonClass.__init__(self)        

## FileClass
#
#
class FileClass(CommonClass):
    def __init__(self):
        self.Filename = ''
        self.Executable = ''
        self.Family = ''
        self.FileType = ''
        CommonClass.__init__(self)
        

## MiscFileClass
#
#
class MiscFileClass(CommonHeaderClass):
    def __init__(self):
        CommonHeaderClass.__init__(self)
        self.Name = ''
        self.Files = []
    

## UserExtensionsClass
#
# This class defined UserExtensions item used in Module/Platform/Package files
# 
# @param object:    Inherited from object class
#
# @var UserID:      To store value for UserID
# @var Identifier:  To store value for Identifier
# @var Content:     To store value for Content
#       
class UserExtensionsClass(object):
    def __init__(self):
        self.UserID = ''
        self.Identifier = 0
        self.Content = ''
        self.Defines = []
        self.BuildOptions = []
