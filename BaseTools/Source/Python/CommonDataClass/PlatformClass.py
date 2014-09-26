## @file
# This file is used to define a class object to describe a platform
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

##
# Import Modules
#
from CommonClass import *

## SkuInfoListClass
#
# This class defined sku info list item used in platform file
# 
# @param IncludeStatementClass:  Inherited from IncludeStatementClass class
#
# @var SkuInfoList:              To store value for SkuInfoList, it is a set structure as
#                                { SkuName : SkuId }
#
class SkuInfoListClass(IncludeStatementClass):
    def __init__(self):
        IncludeStatementClass.__init__(self)
        self.SkuInfoList = {}

## PlatformHeaderClass
#
# This class defined header items used in Platform file
# 
# @param IdentificationClass:    Inherited from IdentificationClass class
# @param CommonHeaderClass:      Inherited from CommonHeaderClass class
# @param DefineClass:            Inherited from DefineClass class
#
# @var DscSpecification:         To store value for DscSpecification
# @var SupArchList:              To store value for SupArchList, selection scope is in below list
#                                EBC | IA32 | X64 | IPF | ARM | PPC | AARCH64
# @var BuildTargets:             To store value for BuildTargets, selection scope is in below list
#                                RELEASE | DEBUG
# @var IntermediateDirectories:  To store value for IntermediateDirectories, selection scope is in below list
#                                MODULE | UNIFIED
# @var OutputDirectory:          To store value for OutputDirectory
# @var ForceDebugTarget:         To store value for ForceDebugTarget
# @var SkuIdName:                To store value for SkuIdName
# @var BuildNumber:              To store value for BuildNumber
# @var MakefileName:             To store value for MakefileName
# @var ClonedFrom:               To store value for ClonedFrom, it is a list structure as
#                                [ ClonedRecordClass, ... ]
#
class PlatformHeaderClass(IdentificationClass, CommonHeaderClass, DefineClass):
    def __init__(self):
        IdentificationClass.__init__(self)
        CommonHeaderClass.__init__(self)
        DefineClass.__init__(self)
        self.DscSpecification = ''
        self.SupArchList = []
        self.BuildTargets = []
        self.IntermediateDirectories = ''
        self.OutputDirectory = ''                                                    
        self.ForceDebugTarget = ''
        self.SkuIdName = []
        self.BuildNumber = ''
        self.MakefileName = ''
        self.ClonedFrom = []

## PlatformFlashDefinitionFileClass
#
# This class defined FlashDefinitionFile item used in platform file
# 
# @param object:   Inherited from object class
#
# @var Id:         To store value for Id
# @var UiName:     To store value for UiName
# @var Preferred:  To store value for Preferred
# @var FilePath:   To store value for FilePath
#
class PlatformFlashDefinitionFileClass(object):
    def __init__(self):
        self.Id = ''
        self.UiName = ''
        self.Preferred = False
        self.FilePath = ''

## PlatformFvImageOptionClass
#
# This class defined FvImageOption item used in platform file
# 
# @param object:             Inherited from object class
#
# @var FvImageOptionName:    To store value for FvImageOptionName
# @var FvImageOptionValues:  To store value for FvImageOptionValues
#
class PlatformFvImageOptionClass(object):
    def __init__(self):
        self.FvImageOptionName = ''
        self.FvImageOptionValues = []

## PlatformFvImageClass
#
# This class defined FvImage item used in platform file
# 
# @param object:        Inherited from object class
#
# @var Name:            To store value for Name
# @var Value:           To store value for Value
# @var Type:            To store value for Type, selection scope is in below list
#                       Attributes | Options | Components | ImageName 
# @var FvImageNames:    To store value for FvImageNames
# @var FvImageOptions:  To store value for FvImageOptions, it is a list structure as
#                       [ PlatformFvImageOption, ...]
#
class PlatformFvImageClass(object):
    def __init__(self):
        self.Name = ''
        self.Value = ''
        self.Type = ''
        self.FvImageNames = []
        self.FvImageOptions = []

## PlatformFvImageNameClass
#
# This class defined FvImageName item used in platform file
# 
# @param object:        Inherited from object class
#
# @var Name:            To store value for Name
# @var Type:            To store value for Type, selection scope is in below list
#                       FV_MAIN | FV_MAIN_COMPACT | NV_STORAGE | FV_RECOVERY | FV_RECOVERY_FLOPPY | FV_FILE | CAPSULE_CARGO | NULL | USER_DEFINED 
# @var FvImageOptions:  To store value for FvImageOptions, it is a list structure as
#                       [ PlatformFvImageOption, ...]
#
class PlatformFvImageNameClass(object):
    def __init__(self):
        self.Name = ''
        self.Type = ''
        self.FvImageOptions = []

## PlatformFvImagesClass
#
# This class defined FvImages item used in platform file
# 
# @param object:  Inherited from object class
#
# @var FvImages:  To store value for FvImages
#
class PlatformFvImagesClass(object):
    def __init__(self):
        self.FvImages = []

## PlatformAntTaskClass
#
# This class defined AntTask item used in platform file
# 
# @param object:       Inherited from object class
#
# @var Id:             To store value for Id
# @var AntCmdOptions:  To store value for AntCmdOptions
# @var FilePath:       To store value for FilePath
#
class PlatformAntTaskClass(object):
    def __init__(self):
        self.Id = ''
        self.AntCmdOptions = ''
        self.FilePath = ''

## PlatformFfsSectionClass
#
# This class defined FfsSection item used in platform file
# 
# @param CommonClass:        Inherited from CommonClass class
#
# @var BindingOrder:         To store value for BindingOrder
# @var Compressible:         To store value for Compressible
# @var SectionType:          To store value for SectionType
# @var EncapsulationType:    To store value for EncapsulationType
# @var ToolName:             To store value for ToolName
# @var Filenames:            To store value for Filenames
# @var Args:                 To store value for Args
# @var OutFile:              To store value for OutFile
# @var OutputFileExtension:  To store value for OutputFileExtension
# @var ToolNameElement:      To store value for ToolNameElement
#
class PlatformFfsSectionClass(CommonClass):
    def __init__(self):
        CommonClass.__init__(self)
        self.BindingOrder = ''
        self.Compressible = ''
        self.SectionType  = ''
        self.EncapsulationType  = ''
        self.ToolName = ''
        self.Filenames = []
        self.Args = ''
        self.OutFile = ''
        self.OutputFileExtension = ''
        self.ToolNameElement = ''

## PlatformFfsSectionsClass
#
# This class defined FfsSections item used in platform file
# 
# @param CommonClass:      Inherited from CommonClass class
#
# @var BindingOrder:       To store value for BindingOrder
# @var Compressible:       To store value for Compressible
# @var SectionType:        To store value for SectionType
# @var EncapsulationType:  To store value for EncapsulationType
# @var ToolName:           To store value for ToolName
# @var Section:            To store value for Section, it is a list structure as
#                          [ PlatformFfsSectionClass, ... ]
# @var Sections:           To store value for Sections, it is a list structure as
#                          [ PlatformFfsSectionsClass, ...]
#
class PlatformFfsSectionsClass(CommonClass):
    def __init__(self):
        CommonClass.__init__(self)
        self.BindingOrder = ''
        self.Compressible = ''
        self.SectionType = ''
        self.EncapsulationType = ''
        self.ToolName = ''
        self.Section = []
        self.Sections = []

## PlatformFfsClass
#
# This class defined Ffs item used in platform file
# 
# @param object:   Inherited from object class
#
# @var Attribute:  To store value for Attribute, it is a set structure as
#                  { [(Name, PlatformFfsSectionsClass)] : Value}
# @var Sections:   To store value for Sections, it is a list structure as
#                  [ PlatformFfsSectionsClass]
# @var ToolName:   To store value for ToolName
#
class PlatformFfsClass(object):
    def __init__(self):
        self.Attribute = {}
        self.Sections = []
        self.Key = ''

## PlatformBuildOptionClass
#
# This class defined BuildOption item used in platform file
# 
# @param object:             Inherited from object class
#
# @var UserDefinedAntTasks:  To store value for UserDefinedAntTasks, it is a set structure as
#                            { [Id] : PlatformAntTaskClass, ...}
# @var Options:              To store value for Options, it is a list structure as
#                            [ BuildOptionClass, ...]
# @var UserExtensions:       To store value for UserExtensions, it is a set structure as
#                            { [(UserID, Identifier)] : UserExtensionsClass, ...}
# @var FfsKeyList:           To store value for FfsKeyList, it is a set structure as
#                            { [FfsKey]: PlatformFfsClass, ...} 
#
class PlatformBuildOptionClass(object):
    def __init__(self):
        self.UserDefinedAntTasks = {}
        self.Options = []
        self.UserExtensions = {}
        self.FfsKeyList = {}

## PlatformBuildOptionClasses
#
# This class defined BuildOption item list used in platform file
# 
# @param IncludeStatementClass:  Inherited from IncludeStatementClass class
#
# @var FvBinding:                To store value for FvBinding
# @var FfsFileNameGuid:          To store value for FfsFileNameGuid
# @var FfsFormatKey:             To store value for FfsFormatKey
# @var BuildOptionList:          To store value for BuildOptionList, it is a list structure as
#                                [ BuildOptionClass, ... ]
#
class PlatformBuildOptionClasses(IncludeStatementClass):
    def __init__(self):
        IncludeStatementClass.__init__(self)
        self.FvBinding = ''
        self.FfsFileNameGuid = ''
        self.FfsFormatKey = ''
        self.BuildOptionList = []

## PlatformLibraryClass
#
# This class defined Library item used in platform file
# 
# @param CommonClass:   Inherited from CommonClass class
# @param DefineClass:   Inherited from DefineClass class
# @param Name:          Input value for Name, default is ''
# @param FilePath:      Input value for FilePath, default is ''
#
# @var Name:            To store value for Name
# @var FilePath:        To store value for FilePath
# @var ModuleType:      To store value for ModuleType
# @var SupModuleList:   To store value for SupModuleList
# @var ModuleGuid:      To store value for ModuleGuid
# @var ModuleVersion:   To store value for ModuleVersion
# @var PackageGuid:     To store value for PackageGuid
# @var PackageVersion:  To store value for PackageVersion
#
class PlatformLibraryClass(CommonClass, DefineClass):
    def __init__(self, Name = '', FilePath = ''):
        CommonClass.__init__(self)
        DefineClass.__init__(self)
        self.Name = Name
        self.FilePath = FilePath
        self.ModuleType = []
        self.SupModuleList = []
        self.ModuleGuid = ''
        self.ModuleVersion = ''
        self.PackageGuid = ''
        self.PackageVersion = ''

## PlatformLibraryClasses
#
# This class defined Library item list used in platform file
# 
# @param IncludeStatementClass:  Inherited from IncludeStatementClass class
#
# @var LibraryList:              To store value for LibraryList, it is a list structure as
#                                [ PlatformLibraryClass, ... ]
#
class PlatformLibraryClasses(IncludeStatementClass):
    def __init__(self):
        IncludeStatementClass.__init__(self)
        self.LibraryList = []

## PlatformModuleClass
#
# This class defined Module item used in platform file
# 
# @param CommonClass:            Inherited from CommonClass class
# @param DefineClass:            Inherited from DefineClass class
# @param IncludeStatementClass:  Inherited from IncludeStatementClass class
#
# @var Name:                     To store value for Name (Library name or libraryclass name or module name)
# @var FilePath:                 To store value for FilePath
# @var Type:                     To store value for Type, selection scope is in below list
#                                LIBRARY | LIBRARY_CLASS | MODULE
# @var ModuleType:               To store value for ModuleType
# @var ExecFilePath:             To store value for ExecFilePath
# @var LibraryClasses:           To store value for LibraryClasses, it is a structure as
#                                PlatformLibraryClasses
# @var PcdBuildDefinitions:      To store value for PcdBuildDefinitions, it is a list structure as
#                                [ PcdClass, ...]
# @var ModuleSaBuildOption:      To store value for ModuleSaBuildOption, it is a structure as
#                                PlatformBuildOptionClasses
# @var Specifications:           To store value for Specifications, it is a list structure as
#                                [ '', '', ...]
#
class PlatformModuleClass(CommonClass, DefineClass, IncludeStatementClass):
    def __init__(self):
        CommonClass.__init__(self)
        DefineClass.__init__(self)
        self.Name = ''
        self.FilePath = ''
        self.Type = ''
        self.ModuleType = ''
        self.ExecFilePath = ''
        self.LibraryClasses = PlatformLibraryClasses()
        self.PcdBuildDefinitions = []
        self.ModuleSaBuildOption = PlatformBuildOptionClasses()
        self.Specifications = []
        self.SourceOverridePath = ''

## PlatformModuleClasses
#
# This class defined Module item list used in platform file
# 
# @param IncludeStatementClass:  Inherited from IncludeStatementClass class
#
# @var ModuleList:               To store value for ModuleList, it is a list structure as
#                                [ PlatformModuleClass, ... ]
#
class PlatformModuleClasses(IncludeStatementClass):
    def __init__(self):
        IncludeStatementClass.__init__(self)
        self.ModuleList = []

## PlatformClass
#
# This class defined a complete platform item
# 
# @param object:                    Inherited from object class
#
# @var Header:                      To store value for Header, it is a structure as
#                                   {Arch : PlatformHeaderClass()}
# @var SkuInfos:                    To store value for SkuInfos, it is a structure as
#                                   SkuInfoListClass
# @var Libraries:                   To store value for Libraries, it is a structure as
#                                   PlatformLibraryClasses
# @var LibraryClasses:              To store value for LibraryClasses, it is a structure as
#                                   PlatformLibraryClasses
# @var Modules:                     To store value for Modules, it is a structure as
#                                   PlatformModuleClasses
# @var FlashDefinitionFile:         To store value for FlashDefinitionFile, it is a structure as
#                                   PlatformFlashDefinitionFileClass
# @var BuildOptions:                To store value for BuildOptions, it is a structure as
#                                   PlatformBuildOptionClasses
# @var DynamicPcdBuildDefinitions:  To store value for DynamicPcdBuildDefinitions, it is a list structure as
#                                   [ PcdClass, ...]
# @var Fdf:                         To store value for Fdf, it is a list structure as
#                                   [ FdfClass, ...]
# @var UserExtensions:              To store value for UserExtensions, it is a list structure as
#                                   [ UserExtensionsClass, ...]
#
class PlatformClass(object):
    def __init__(self):
        self.Header = {}
        self.SkuInfos = SkuInfoListClass()
        self.Libraries = PlatformLibraryClasses()
        self.LibraryClasses = PlatformLibraryClasses()
        self.Modules = PlatformModuleClasses()
        self.FlashDefinitionFile = PlatformFlashDefinitionFileClass()
        self.BuildOptions = PlatformBuildOptionClasses()
        self.DynamicPcdBuildDefinitions = []
        self.Fdf = []
        self.UserExtensions = []

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    P = PlatformClass()
