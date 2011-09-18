## @file
# This file is used to define a class object to describe a module
#
# Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
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

## ModuleHeaderClass
#
# This class defined header items used in Module file
# 
# @param IdentificationClass:    Inherited from IdentificationClass class
# @param CommonHeaderClass:      Inherited from CommonHeaderClass class
# @param DefineClass:            Inherited from DefineClass class
#
# @var ModuleType:               To store value for ModuleType
# @var SupArchList:              To store value for SupArchList, selection scope is in below list
#                                EBC | IA32 | X64 | IPF | ARM | PPC
# @var BinaryModule:             To store value for BinaryModule
# @var OutputFileBasename:       To store value for OutputFileBasename
# @var ClonedFrom:               To store value for ClonedFrom, it is a set structure as
#                                [ ClonedRecordClass, ... ]
# @var PcdIsDriver:              To store value for PcdIsDriver, selection scope is in below list
#                                PEI_PCD_DRIVER | DXE_PCD_DRIVER
# @var TianoEdkFlashMap_h:       To store value for TianoEdkFlashMap_h
# @var InfVersion:               To store value for InfVersion
# @var UefiSpecificationVersion: To store value for UefiSpecificationVersion
# @var EdkReleaseVersion:        To store value for EdkReleaseVersion
# @var LibraryClass:             To store value for LibraryClass, it is a set structure as
#                                [ LibraryClassClass, ...]
# @var ComponentType:            To store value for ComponentType, selection scope is in below list
#                                LIBRARY | SECURITY_CORE | PEI_CORE | COMBINED_PEIM_DRIVER | PIC_PEIM | RELOCATABLE_PEIM | BS_DRIVER | RT_DRIVER | SAL_RT_DRIVER | APPLICATION
# @var MakefileName:             To store value for MakefileName
# @var BuildNumber:              To store value for BuildNumber
# @var BuildType:                To store value for BuildType
# @var FfsExt:                   To store value for FfsExt
# @var FvExt:                    To store value for FvExt
# @var SourceFv:                 To store value for SourceFv
# @var CustomMakefile:           To store value for CustomMakefile, it is a set structure as
#                                { Family : Filename, ... }
# @var Shadow:                   To store value for Shadow
# @var MacroDefines              To store the defined macros
#
class ModuleHeaderClass(IdentificationClass, CommonHeaderClass, DefineClass):
    def __init__(self):
        IdentificationClass.__init__(self)
        CommonHeaderClass.__init__(self)
        DefineClass.__init__(self)
        self.ModuleType = ''
        self.SupModuleList = []
        self.SupArchList = []
        self.BinaryModule = False
        self.OutputFileBasename = ''
        self.ClonedFrom = []
        self.PcdIsDriver = ''
        self.TianoEdkFlashMap_h = False
        self.InfVersion = ''
        self.PiSpecificationVersion = ''
        self.UefiSpecificationVersion = ''
        self.EdkReleaseVersion = ''
        self.LibraryClass = []
        self.ComponentType = ''
        self.MakefileName = ''
        self.BuildNumber = ''
        self.BuildType = ''
        self.FfsExt = ''
        self.FvExt = ''
        self.SourceFv = ''
        self.CustomMakefile = {}
        self.Shadow = ''
        self.MacroDefines = {}
        self.SourceOverridePath = ''
        self.Specification = []

## ModuleSourceFileClass
#
# This class defined source file item used in Module file
# 
# @param CommonClass:      Inherited from CommonClass class
# @param SourceFile:       Input value for SourceFile, default is ''
# @param TagName:          Input value for TagName, default is ''
# @param ToolCode:         Input value for ToolCode, default is ''
# @param ToolChainFamily:  Input value for ToolChainFamily, default is ''
# @param FeatureFlag:      Input value for FeatureFlag, default is ''
# @param SupArchList:      Input value for SupArchList, default is []
#
# @var SourceFile:         To store value for SourceFile
# @var TagName:            To store value for TagName
# @var ToolCode:           To store value for ToolCode
# @var ToolChainFamily:    To store value for ToolChainFamily
#
class ModuleSourceFileClass(CommonClass):
    def __init__(self, SourceFile = '', TagName = '', ToolCode = '', ToolChainFamily = '', FeatureFlag = '', SupArchList = None):
        self.SourceFile = SourceFile
        self.TagName = TagName
        self.ToolCode = ToolCode
        self.ToolChainFamily = ToolChainFamily
        self.FileType = ''
        CommonClass.__init__(self, FeatureFlag = FeatureFlag, SupArchList = SupArchList)

## ModuleBinaryFileClass
#
# This class defined binary file item used in Module file
# 
# @param CommonClass:    Inherited from CommonClass class
# @param BinaryFile:     Input value for BinaryFile, default is ''
# @param FileType:       Input value for FileType, default is ''
# @param FeatureFlag:    Input value for FeatureFlag, default is ''
# @param SupArchList:    Input value for SupArchList, default is []
#
# @var BinaryFile:       To store value for BinaryFile
# @var FileType:         To store value for FileType, selection scope is in below list
#                        FW | GUID | PREEFORM | UEFI_APP | UNI_UI | UNI_VER | LIB | PE32 | PIC | PEI_DEPEX | DXE_DEPEX | SMM_DEPEX| TE | VER | UI | BIN | FV
# @var Target:           To store value for Target
# @var ToolChainFamily:  To store value for ToolChainFamily
#
class ModuleBinaryFileClass(CommonClass):
    def __init__(self, BinaryFile = '', FileType = '', Target = '', FeatureFlag = '', SupArchList = None):
        self.BinaryFile = BinaryFile
        self.FileType = FileType
        self.Target = Target
        CommonClass.__init__(self, FeatureFlag = FeatureFlag, SupArchList = SupArchList)
        self.Filenames = []
        self.PatchPcdValues = []
        self.PcdExValues = []
        self.LibraryInstances = []
        self.BuildFlags = []

## ModulePackageDependencyClass
#
# This class defined package dependency item used in Module file
# 
# @param CommonClass:   Inherited from CommonClass class
# @param DefineClass:   Input value for DefineClass class
#
# @var FilePath:        To store value for FilePath
# @var PackageName:     To store value for PackageName
# @var PackageVersion:  To store value for PackageVersion
# @var PackageGuid:     To store value for PackageGuid
#
class ModulePackageDependencyClass(CommonClass, DefineClass):
    def __init__(self):
        self.FilePath = ''
        self.PackageName = ''
        self.PackageVersion = ''
        self.PackageGuid = ''
        self.Description = ''
        CommonClass.__init__(self)
        DefineClass.__init__(self)       

## ModuleLibraryClass
#
# This class defined library item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var Library:        To store value for Library
#
class ModuleLibraryClass(CommonClass):
    def __init__(self):
        self.Library = ''
        CommonClass.__init__(self)

## ModuleEventClass
#
# This class defined event item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var CName:          To store value for CName
# @var GuidCName:      To store value for GuidCName
# @var Type:           To store value for Type, selection scope is in below list
#                      CREATE_EVENT | SIGNAL_EVENT
#
class ModuleEventClass(CommonClass):        
    def __init__(self):
        self.CName = ''
        self.GuidCName = ''
        self.Type = ''                              
        CommonClass.__init__(self)

## ModuleHobClass
#
# This class defined hob item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var GuidCName:      To store value for GuidCName
# @var Type:           To store value for Type, selection scope is in below list
#                      PHIT | MEMORY_ALLOCATION | RESOURCE_DESCRIPTOR | GUID_EXTENSION | FIRMWARE_VOLUME | CPU | POOL | CAPSULE_VOLUME
#
class ModuleHobClass(CommonClass):
    def __init__(self):
        self.Type = ''
        self.GuidCName = ''
        CommonClass.__init__(self)

## ModuleVariableClass
#
# This class defined variable item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var GuidCName:      To store value for GuidCName
# @var Name:           To store value for Name
#
class ModuleVariableClass(CommonClass):
    def __init__(self):
        self.Name = ''
        self.GuidCName = ''
        CommonClass.__init__(self)

## ModuleBootModeClass
#
# This class defined boot mode item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var Name:           To store value for Name, selection scope is in below list
#                      FULL | MINIMAL | NO_CHANGE | DIAGNOSTICS | DEFAULT | S2_RESUME | S3_RESUME | S4_RESUME | S5_RESUME | FLASH_UPDATE | RECOVERY_FULL | RECOVERY_MINIMAL | RECOVERY_NO_CHANGE | RECOVERY_DIAGNOSTICS | RECOVERY_DEFAULT | RECOVERY_S2_RESUME | RECOVERY_S3_RESUME | RECOVERY_S4_RESUME | RECOVERY_S5_RESUME | RECOVERY_FLASH_UPDATE 
#
class ModuleBootModeClass(CommonClass):
    def __init__(self):
        self.Name = ''
        CommonClass.__init__(self)

## ModuleSystemTableClass
#
# This class defined system table item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var CName:          To store value for CName
#
class ModuleSystemTableClass(CommonClass):
    def __init__(self):
        self.CName = ''
        CommonClass.__init__(self)

## ModuleDataHubClass
#
# This class defined data hub item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var CName:          To store value for CName
#
class ModuleDataHubClass(CommonClass):
    def __init__(self):
        self.CName = ''
        CommonClass.__init__(self)        

## ModuleHiiPackageClass
#
# This class defined Hii package item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var CName:          To store value for CName
#
class ModuleHiiPackageClass(CommonClass):
    def __init__(self):
        self.CName = ''
        CommonClass.__init__(self)

## ModuleExternImageClass
#
# This class defined Extern Image item used in Module file
# 
# @param object:           Inherited from object class
#
# @var ModuleEntryPoint:   To store value for ModuleEntryPoint
# @var ModuleUnloadImage:  To store value for ModuleUnloadImage
#
class ModuleExternImageClass(object):
    def __init__(self):
        self.ModuleEntryPoint = ''
        self.ModuleUnloadImage = ''

## ModuleExternLibraryClass
#
# This class defined Extern Library item used in Module file
# 
# @param object:     Inherited from object class
#
# @var Constructor:  To store value for Constructor
# @var Destructor:   To store value for Destructor
#
class ModuleExternLibraryClass(object):
    def __init__(self):
        self.Constructor = ''
        self.Destructor = ''

## ModuleExternDriverClass
#
# This class defined Extern Driver item used in Module file
# 
# @param object:       Inherited from object class
#
# @var DriverBinding:  To store value for DriverBinding
# @var ComponentName:  To store value for ComponentName
# @var DriverConfig:   To store value for DriverConfig
# @var DriverDiag:     To store value for DriverDiag
#
class ModuleExternDriverClass(object):
    def __init__(self):
        self.DriverBinding= ''
        self.ComponentName = ''
        self.DriverConfig = ''
        self.DriverDiag = ''

## ModuleExternCallBackClass
#
# This class defined Extern Call Back item used in Module file
# 
# @param object:                      Inherited from object class
#
# @var SetVirtualAddressMapCallBack:  To store value for SetVirtualAddressMapCallBack
# @var ExitBootServicesCallBack:      To store value for ExitBootServicesCallBack
#
class ModuleExternCallBackClass(object):
    def __init__(self):
        self.SetVirtualAddressMapCallBack = ''
        self.ExitBootServicesCallBack = ''

## ModuleExternClass
#
# This class defined Extern used in Module file
# 
# @param object:                      Inherited from object class
#
#
class ModuleExternClass(CommonClass):
    def __init__(self):
        self.EntryPoint = ''
        self.UnloadImage = ''
        self.Constructor = ''
        self.Destructor = ''
        CommonClass.__init__(self)

## ModuleDepexClass
#
# This class defined depex item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
# @param DefineClass:  Input value for DefineClass class
#
# @var Depex:          To store value for Depex
#
class ModuleDepexClass(CommonClass, DefineClass):
    def __init__(self):
        CommonClass.__init__(self)
        DefineClass.__init__(self)
        self.Depex = ''

## ModuleNmakeClass
#
# This class defined nmake item used in Module file
# 
# @param CommonClass:  Inherited from CommonClass class
#
# @var Name:           To store value for Name
# @var Value:          To store value for Value
#
class ModuleNmakeClass(CommonClass):
    def __init__(self):
        CommonClass.__init__(self)
        self.Name = ''
        self.Value = ''

## ModuleClass
#
# This class defined a complete module item
# 
# @param object:    Inherited from object class
#
# @var Header:               To store value for Header, it is a structure as
#                            {Arch : ModuleHeaderClass}
# @var LibraryClasses:       To store value for LibraryClasses, it is a list structure as
#                            [ LibraryClassClass, ...]
# @var Libraries:            To store value for Libraries, it is a list structure as
#                            [ ModuleLibraryClass, ...]
# @var Sources:              To store value for Sources, it is a list structure as
#                            [ ModuleSourceFileClass, ...]
# @var Binaries:             To store value for Binaries, it is a list structure as
#                            [ ModuleBinaryFileClass, ...]
# @var NonProcessedFiles:    To store value for NonProcessedFiles, it is a list structure as
#                            [ '', '', ...]
# @var PackageDependencies:  To store value for PackageDependencies, it is a list structure as
#                            [ ModulePackageDependencyClass, ... ] 
# @var Nmake:                To store value for Nmake, it is a list structure as
#                            [ ModuleNmakeClass, ... ]
# @var Depex:                To store value for Depex, it is a list structure as
#                            [ ModuleDepexClass, ... ]
# @var Includes:             To store value for Includes, it is a list structure as
#                            [ IncludeClass, ...]
# @var Protocols:            To store value for Protocols, it is a list structure as
#                            [ ProtocolClass, ...]
# @var Ppis:                 To store value for Ppis, it is a list structure as
#                            [ PpiClass, ...]
# @var Events:               To store value for Events, it is a list structure as
#                            [ ModuleEventClass, ...]
# @var Hobs:                 To store value for Hobs, it is a list structure as
#                            [ ModuleHobClass, ...] 
# @var Variables:            To store value for Variables, it is a list structure as
#                            [ ModuleVariableClass, ...]
# @var BootModes:            To store value for BootModes, it is a list structure as
#                            [ ModuleBootModeClass, ...]
# @var SystemTables:         To store value for SystemTables, it is a list structure as
#                            [ ModuleSystemTableClass, ...]
# @var DataHubs:             To store value for DataHubs, it is a list structure as
#                            [ ModuleDataHubClass, ...]
# @var HiiPackages:          To store value for HiiPackages, it is a list structure as
#                            [ ModuleHiiPackageClass, ...]
# @var Guids:                To store value for Guids, it is a list structure as
#                            [ GuidClass, ...]
# @var PcdCodes:             To store value for PcdCodes, it is a list structure as
#                            [ PcdClass, ...]
# @var ExternImages:         To store value for ExternImages, it is a list structure as
#                            [ ModuleExternImageClass, ...]
# @var ExternLibraries:      To store value for ExternLibraries, it is a list structure as
#                            [ ModuleExternLibraryClass, ...]
# @var ExternDrivers:        To store value for ExternDrivers, it is a list structure as
#                            [ ModuleExternDriverClass, ...]
# @var ExternCallBacks:      To store value for ExternCallBacks, it is a list structure as
#                            [ ModuleExternCallBackClass, ...]
# @var BuildOptions:         To store value for BuildOptions, it is a list structure as
#                            [ BuildOptionClass, ...]
# @var UserExtensions:       To store value for UserExtensions, it is a list structure as
#                            [ UserExtensionsClass, ...]
#
class ModuleClass(object):
    def __init__(self):
        self.Header = {}
        self.ModuleHeader = ModuleHeaderClass()
        self.LibraryClasses = []
        self.Libraries = []
        self.Sources = []
        self.Binaries = []
        self.NonProcessedFiles = []
        self.PackageDependencies = []
        self.Nmake = []
        self.Depex = []
        self.PeiDepex = None
        self.DxeDepex = None
        self.SmmDepex = None
        self.Includes = []
        self.Protocols = []
        self.Ppis = []
        self.Events = []
        self.Hobs = []
        self.Variables = []
        self.BootModes = []
        self.SystemTables = []
        self.DataHubs = []
        self.HiiPackages = []
        self.Guids = []
        self.PcdCodes = []
        self.ExternImages = []
        self.ExternLibraries = []
        self.ExternDrivers = []
        self.ExternCallBacks = []
        self.Externs = []
        self.BuildOptions = []
        self.UserExtensions = None
        self.MiscFiles = None
        self.FileList = []

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    M = ModuleClass()
