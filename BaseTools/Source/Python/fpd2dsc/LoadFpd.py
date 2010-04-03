## @file
# Open an FPD file and load all its contents to a PlatformClass object.
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
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
from CommonDataClass.PlatformClass import *
from CommonDataClass.FdfClass import *
from Common.XmlRoutines import *
from Common.MigrationUtilities import *
from EdkIIWorkspaceGuidsInfo import gEdkIIWorkspaceGuidsInfo

## Load Platform Header
#
# Read an input Platform XML DOM object and return Platform Header class object
# contained in the DOM object.
#
# @param  XmlFpd                An XML DOM object read from FPD file
# @param  FpdFileName           The file path of FPD File
#
# @retvel  PlatformHeader       A new Platform Header object loaded from XmlFpd
#
def LoadPlatformHeader(XmlFpd, FpdFileName):
    PlatformHeader = PlatformHeaderClass()
    
    XmlTag = "PlatformSurfaceArea/PlatformHeader"
    FpdHeader = XmlNode(XmlFpd, XmlTag)
    
    SetIdentification(PlatformHeader, FpdHeader, "PlatformName", FpdFileName)
    SetCommonHeader(PlatformHeader, FpdHeader)

    XmlTag = "PlatformSurfaceArea/PlatformHeader/Specification"
    List = XmlElement(XmlFpd, XmlTag).split()
    SpecificationName = List[0]
    SpecificationValue = List[1]
    PlatformHeader.Specification = {SpecificationName:SpecificationValue}
    
    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/SupportedArchitectures"
    PlatformHeader.SupArchList = XmlElement(XmlFpd, XmlTag).split()

    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/BuildTargets"
    PlatformHeader.BuildTargets = XmlElement(XmlFpd, XmlTag).split()

    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/IntermediateDirectories"
    PlatformHeader.IntermediateDirectories = XmlElement(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/OutputDirectory"
    PlatformHeader.OutputDirectory = XmlElement(XmlFpd, XmlTag)

    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/SkuInfo"
    List = map(LoadSkuId, XmlList(XmlFpd, XmlTag))
    if List != []:
        PlatformHeader.SkuIdName = List[0]
    
    return PlatformHeader

## Load a Platform SkuId
#
# Read an input Platform XML DOM object and return a list of Platform SkuId
# contained in the DOM object.
#
# @param    XmlPlatformSkuInfo     An XML DOM object read from FPD file
#
# @retvel   PlatformSkuInfo        A SkuInfo loaded from XmlFpd
#
def LoadPlatformSkuInfo(XmlPlatformSkuInfo):
    XmlTag = "SkuInfo/SkuId"
    SkuInfo = []
    SkuId = XmlElement(XmlPlatformSkuInfo, XmlTag)
    SkuInfo.append(SkuId)
    
    XmlTag = "SkuInfo/Value"
    Value = XmlElement(XmlPlatformSkuInfo, XmlTag)
    SkuInfo.append(Value)
    return SkuInfo

## Load a Platform SkuId
#
# Read an input Platform XML DOM object and return a list of Platform SkuId
# contained in the DOM object.
#
# @param    XmlSkuInfo     An XML DOM object read from FPD file
#
# @retvel   List           A list of SkuId and SkuValue loaded from XmlFpd
#
def LoadSkuId(XmlSkuInfo):
    XmlTag = "SkuInfo/UiSkuName"
    SkuValue = XmlElement(XmlSkuInfo, XmlTag)
    
    XmlTag = "SkuInfo/UiSkuName"
    List = map(LoadSkuID, XmlList(XmlSkuInfo, XmlTag))
    if List != []:
        SkuID = List[0]
    #SkuID = XmlAttribute(XmlSkuInfo, XmlTag)
    List = []
    List.append(SkuID)
    List.append(SkuValue)
    return List

def LoadSkuID(XmlUiSkuName):
    XmlTag = "SkuID"
    SkuID = XmlAttribute(XmlUiSkuName, XmlTag)
    return SkuID

## Load a list of Platform SkuIds
#
# Read an input Platform XML DOM object and return a list of Platform SkuId
# contained in the DOM object.
#
# @param    XmlFpd                 An XML DOM object read from FPD file
#
# @retvel   PlatformSkuIds         A platform SkuIds object loaded from XmlFpd
#
def LoadPlatformSkuInfos(XmlFpd):
    PlatformSkuIds = SkuInfoListClass()

    SkuInfoList = []
    
    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/SkuInfo"
    List = map(LoadSkuId, XmlList(XmlFpd, XmlTag))
    SkuInfoList = List
    
    XmlTag = "PlatformSurfaceArea/PlatformDefinitions/SkuInfo/UiSkuName"
    Value = XmlElement(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/DynamicPcdBuildDefinitions/PcdBuildData/SkuInfo"
    # here return a List
    List = map(LoadPlatformSkuInfo, XmlList(XmlFpd, XmlTag))

    for SkuInfo in List:
        SkuId = SkuInfo[0]
        Value = SkuInfo[1]

        SkuInfoList.append(SkuInfo)

    PlatformSkuIds.SkuInfoList = SkuInfoList

    return PlatformSkuIds

## Load Platform Module Build Option
#
# Read an input Platform XML DOM object and return Platform Module Build Option class object
# contained in the DOM object.
#
# @param    XmlModuleBuildOption            An XML DOM object read from FPD file
#
# @retvel   PlatformBuildOption             A Platform Build Option object loaded from XmlFpd
#
def LoadModuleBuildOption(XmlModuleBuildOption):
    PlatformBuildOption = PlatformBuildOptionClass()
    PlatformBuildOption.UserDefinedAntTasks = {}
    
    XmlTag = "BuildOptions/Options/Option"
    PlatformBuildOption.Options = map(LoadBuildOption, XmlList(XmlModuleBuildOption, XmlTag))
    
    PlatformBuildOption.UserExtensions = {}
    PlatformBuildOption.FfsKeyList = {}
    return PlatformBuildOption

## Load Platform Module Extern
#
# Read an input Platform XML DOM object and return Platform Module Extern class object
# contained in the DOM object.
#
# @param    XmlModuleExtern                  An XML DOM object read from FPD file
#
# @retvel   PlatformModuleExtern             A Platform Module Extern object loaded from XmlFpd
#
def LoadModuleExtern(XmlModuleExtern):
    PlatformModuleExtern = []
    
    XmlTag = "Externs/PcdIsDriver"
    PcdIsDriver = XmlElement(XmlModuleExtern, XmlTag)
    PlatformModuleExtern.append(PcdIsDriver)
    
    XmlTag = "Externs/Specification"
    Specification = XmlElement(XmlModuleExtern, XmlTag)
    PlatformModuleExtern.append(Specification)
    
    XmlTag = "Externs/Extern"

    return PlatformModuleExtern

## Load Platform ModuleSaBuildOptions
#
# Read an input Platform XML DOM object and return Platform ModuleSaBuildOptions class object
# contained in the DOM object.
#
# @param    XmlModuleSaBuildOptions            An XML DOM object read from FPD file
#
# @retvel   PlatformBuildOptions               A list of Platform ModuleSaBuildOption object loaded from XmlFpd
#
def LoadPlatformModuleSaBuildOption(XmlModuleSA):
    PlatformModuleSaBuildOption = PlatformBuildOptionClasses()
    
    XmlTag = "ModuleSA/ModuleSaBuildOptions/FvBinding"
    PlatformModuleSaBuildOption.FvBinding = XmlElement(XmlModuleSA, XmlTag)
    
    XmlTag = "ModuleSA/ModuleSaBuildOptions/FfsFormatKey"
    PlatformModuleSaBuildOption.FfsFormatKey = XmlElement(XmlModuleSA, XmlTag)
    
    XmlTag = "ModuleSA/ModuleSaBuildOptions/FfsFileNameGuid"
    PlatformModuleSaBuildOption.FfsFileNameGuid = XmlElement(XmlModuleSA, XmlTag)
    
    XmlTag = "ModuleSA/ModuleSaBuildOptions/Options/Option"
    PlatformModuleSaBuildOption.BuildOptionList = map(LoadBuildOption, XmlList(XmlModuleSA, XmlTag))

    return PlatformModuleSaBuildOption

## Load a list of Platform Library Classes
#
# Read an input Platform XML DOM object and return a list of Library Classes
# contained in the DOM object.
#
# @param  XmlLibraryInstance       An XML DOM object read from FPD file
#
# @retvel  LibraryInstance         A Library Instance loaded from XmlFpd
#
def LoadPlatformModuleLibraryInstance(XmlLibraryInstance):
    LibraryInstance = []

    XmlTag = "ModuleGuid"
    ModuleGuid = XmlAttribute(XmlLibraryInstance, XmlTag)
    
    ModulePath = gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(ModuleGuid)
    ModuleMSAFile = ModulePath.replace('.inf', '.msa')
    WorkSpace = os.getenv('WORKSPACE')
    ModuleMSAFileName = os.path.join(WorkSpace, ModuleMSAFile)
    XmlMsa = XmlParseFile(ModuleMSAFileName)
    
    XmlTag = "ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass/Keyword"
    Name = XmlElement(XmlMsa, XmlTag)
    LibraryInstance.append(Name)
    LibraryInstance.append(ModulePath)
    
    #XmlTag = "PackageGuid"
    #PackageGuid = XmlAttribute(XmlLibraryInstance, XmlTag)
    #LibraryInstance.append(PackageGuid)
    return LibraryInstance

## Load a Library Class
#
# Read an input Platform XML DOM object and return a library class object
# contained in the DOM object.
#
# @param    XmlLibraryClass       An XML DOM object read from FPD file
#
# @retvel   SupModuleList         A Library Class Supported Module List object loaded from XmlFpd
#
def LoadLibraryClassSupModuleList(XmlLibraryClass):
    XmlTag = "Usage"
    Usage = XmlAttribute(XmlLibraryClass, XmlTag)
    if Usage == "ALWAYS_PRODUCED":
        XmlTag = "SupModuleList"
        SupModuleList = XmlAttribute(XmlLibraryClass, XmlTag).split()
        return SupModuleList
        
## Load Platform Library Class
#
# Read an input Platform XML DOM object and return Platform module class object
# contained in the DOM object.
#
# @param    XmlLibraries             An XML DOM object read from FPD file
#
# @retvel   PlatformLibraryClass     A Platform Library Class object loaded from XmlFpd
#
def LoadPlatformLibraryClass(XmlPlatformLibraryClass):
    PlatformLibraryInstance = PlatformLibraryClass()

    XmlTag = "ModuleGuid"
    LibraryInstanceModuleGuid = XmlAttribute(XmlPlatformLibraryClass, XmlTag)
    
    XmlTag = "PackageGuid"
    LibraryInstancePackageGuid = XmlAttribute(XmlPlatformLibraryClass, XmlTag)
    
    LibraryInstancePath = gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(LibraryInstanceModuleGuid)
    
    if LibraryInstancePath != "": # if LibraryInstancePath == "" that's because the module guid cannot be resolved
        PlatformLibraryInstance.FilePath = LibraryInstancePath
        # replace *.inf to *.msa
        LibraryInstanceMSAName = LibraryInstancePath.replace('.inf', '.msa')
        WorkSpace = os.getenv('WORKSPACE')
        LibraryInstanceMSAPath = os.path.join(WorkSpace, LibraryInstanceMSAName)
        
        PlatformLibraryInstance.FilePath = LibraryInstancePath
    
        XmlMsa = XmlParseFile(LibraryInstanceMSAPath)

        XmlTag = "ModuleSurfaceArea/MsaHeader/ModuleName"
        PlatformLibraryInstance.Name = XmlElement(XmlMsa, XmlTag)
    
        XmlTag = "ModuleSurfaceArea/MsaHeader/ModuleType"
        PlatformLibraryInstance.ModuleType = XmlElement(XmlMsa, XmlTag)
    
        if PlatformLibraryInstance.ModuleType != "BASE":
            XmlTag = "ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass"
            List = map(LoadLibraryClassSupModuleList, XmlList(XmlMsa, XmlTag))
            if List != []:
                PlatformLibraryInstance.SupModuleList = List[0]
        XmlTag = "ModuleSurfaceArea/ModuleDefinitions/SupportedArchitectures"
        PlatformLibraryInstance.SupArchList = XmlElement(XmlMsa, XmlTag).split()
    
        PlatformLibraryInstance.ModuleGuid = LibraryInstanceModuleGuid
    
        XmlTag = "ModuleSurfaceArea/MsaHeader/Version"
        PlatformLibraryInstance.ModuleVersion = XmlElement(XmlMsa, XmlTag)
    
        PlatformLibraryInstance.PackageGuid = LibraryInstancePackageGuid
        PlatformLibraryInstance.PackageVersion = ''
    
        return PlatformLibraryInstance

## Load Platform Library Classes
#
# Read an input Platform XML DOM object and return Platform module class object
# contained in the DOM object.
#
# @param    XmlLibraries             An XML DOM object read from FPD file
#
# @retvel   PlatformLibraryClasses    A list of Platform Library Class object loaded from XmlFpd
#
def LoadPlatformLibraryClasses(XmlFpd):
    PlatformLibraryInstances = PlatformLibraryClasses()
    PlatformLibraryInstances.LibraryList = []

    List = []
    XmlTag = "PlatformSurfaceArea/FrameworkModules/ModuleSA/Libraries/Instance"
    List = map(LoadPlatformLibraryClass, XmlList(XmlFpd, XmlTag))
    #List.sort()
    if List == []:
        print "Error"
    else:
        PlatformLibraryInstances.LibraryList = List
    
    return PlatformLibraryInstances

## Load Platform module
#
# Read an input Platform XML DOM object and return Platform module class object
# contained in the DOM object.
#
# @param    XmlModuleSA            An XML DOM object read from FPD file
#
# @retvel   PlatformModule         A Platform module object loaded from XmlFpd
#
def LoadModuleSA(XmlModuleSA):
    PlatformModule = PlatformModuleClass()

    # three parts: Libraries instances, PcdBuildDefinition, ModuleSaBuildOptions
    XmlTag = "ModuleSA/Libraries/Instance"

    PlatformModule.LibraryClasses = map(LoadPlatformModuleLibraryInstance, XmlList(XmlModuleSA, XmlTag))

    XmlTag = "ModuleSA/PcdBuildDefinition/PcdData"
    PlatformModule.PcdBuildDefinitions = map(LoadPlatformPcdData, XmlList(XmlModuleSA, XmlTag))

    XmlTag = "ModuleSA/ModuleSaBuildOptions"
    PlatformModule.ModuleSaBuildOption = LoadPlatformModuleSaBuildOption(XmlModuleSA)

    XmlTag = "ModuleSA/BuildOptions"
    PlatformModule.BuildOptions = map(LoadModuleBuildOption, XmlList(XmlModuleSA, XmlTag)) #bugbug fix me
    
    XmlTag = "ModuleSA/Externs"
    PlatformModule.Externs = map(LoadModuleExtern, XmlList(XmlModuleSA, XmlTag)) #bugbug fix me
    
    XmlTag = "SupArchList"
    PlatformModule.SupArchList = XmlAttribute(XmlModuleSA, XmlTag).split()
    
    # the package guid which the module depends on, do not care for now
    XmlTag = "PackageGuid"
    PlatformModule.PackageGuid = XmlAttribute(XmlModuleSA, XmlTag)

    # the module guid, use this guid to get the module *.msa file and convert it to *.inf file with path
    XmlTag = "ModuleGuid"
    PlatformModule.ModuleGuid = XmlAttribute(XmlModuleSA, XmlTag)
    # use this guid to find the *.msa file path or FilePath $(WORKSPACE)/EdkModulePkg/Core/Dxe/DxeMain.msa
    # then convert $(WORKSPACE)/EdkModulePkg/Core/Dxe/DxeMain.msa to $(WORKSPACE)/EdkModulePkg/Core/Dxe/DxeMain.inf, it's FilePath
    PlatformModulePath = gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(PlatformModule.ModuleGuid)

    PlatformModule.FilePath = PlatformModulePath # *.inf file path
    # *.inf back to *.msa
    ModuleMSAFileName = PlatformModulePath.replace('.inf', '.msa')
    WorkSpace = os.getenv('WORKSPACE')
    ModuleMSAFileName = os.path.join(WorkSpace, ModuleMSAFileName)
    # Open this module
    #ModuleMSA = open(ModuleMSAFileName, "r")
    XmlMsa = XmlParseFile(ModuleMSAFileName)

    XmlTag = "ModuleSurfaceArea/MsaHeader/ModuleName"
    PlatformModule.Name = XmlElement(XmlMsa, XmlTag)     # ModuleName

    XmlTag = "ModuleSurfaceArea/MsaHeader/ModuleType"
    PlatformModule.ModuleType = XmlElement(XmlMsa, XmlTag)

    # IA32, X64, IPF and EBC which the module support arch
    #XmlTag = "ModuleSurfaceArea/ModuleDefinitions/SupportedArchitectures"
    #PlatformModule.SupArchList = XmlElement(XmlMsa, XmlTag).split()

    #XmlTag = "ModuleSurfaceArea/MsaHeader/"
    PlatformModule.Type = ''     #LIBRARY | LIBRARY_CLASS | MODULE, used by dsc. New in DSC spec

    PlatformModule.ExecFilePath = '' # New in DSC spec

    XmlTag = "ModuleSurfaceArea/MsaHeader/Specification"
    PlatformModule.Specifications = XmlElement(XmlMsa, XmlTag).split()

    return PlatformModule

## Load Platform modules
#
# Read an input Platform XML DOM object and return a list of Platform modules class object
# contained in the DOM object.
#
# @param    XmlFpd                  An XML DOM object read from FPD file
#
# @retvel   PlatformModules         A list of Platform modules object loaded from XmlFpd
#
def LoadPlatformModules(XmlFpd):
    PlatformModules = PlatformModuleClasses()
    
    XmlTag = "PlatformSurfaceArea/FrameworkModules/ModuleSA"
    PlatformModules.ModuleList = map(LoadModuleSA, XmlList(XmlFpd, XmlTag))
    
    return PlatformModules

## Load Platform Flash Definition File
#
# Read an input Platform XML DOM object and return Platform Flash Definition File class object
# contained in the DOM object.
#
# @param    XmlFpd                              An XML DOM object read from FPD file
# @param    FpdFileName                         The file path of FPD File
#
# @retvel   PlatformFlashDefinitionFile         A new Platform Flash Definition File object loaded from XmlFpd
#
def LoadPlatformFlashDefinitionFile(XmlFpd, FpdFileName):
    PlatformFlashDefinitionFile = PlatformFlashDefinitionFileClass()
    
    XmlTag = "PlatformSurfaceArea/Flash/FlashDefinitionFile"
    PlatformFlashDefinitionFile.FilePath = XmlElement(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/Flash/FlashDefinitionFile/Id"
    PlatformFlashDefinitionFile.Id = XmlAttribute(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/Flash/FlashDefinitionFile/UiName"
    PlatformFlashDefinitionFile.UiName = XmlAttribute(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/Flash/FlashDefinitionFile/Preferred"
    PlatformFlashDefinitionFile.Preferred = XmlAttribute(XmlFpd, XmlTag)
    
    return PlatformFlashDefinitionFile

## Load Platform User Defined Ant Tasks
#
# Read an input Platform XML DOM object and return platform
# User Defined Ant Tasks contained in the DOM object.
#
# @param   XmlUserDefinedAntTasks   An XML DOM object read from FPD file
#
# @retvel  AntTask                  An Ant Task loaded from XmlFpd
#
def LoadUserDefinedAntTasks(XmlFpd):
    Dict = {}
    AntTask = PlatformAntTaskClass()

    XmlTag = "PlatformSurfaceArea/BuildOptions/UserDefinedAntTasks/AntTask/Id"
    AntTask.Id = XmlAttribute(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/BuildOptions/UserDefinedAntTasks/AntTask/AntCmdOptions"
    AntTask.AntCmdOptions = XmlElement(XmlFpd, XmlTag)
    
    XmlTag = "PlatformSurfaceArea/BuildOptions/UserDefinedAntTasks/AntTask/Filename"
    AntTask.FilePath = XmlElement(XmlFpd, XmlTag)
    
    Dict[AntTask.Id] = AntTask
    return Dict

## Load Platform Build Options
#
# Read an input Platform XML DOM object and return a list of platform
# Build Option contained in the DOM object.
#
# @param   XmlBuildOptions              An XML DOM object read from FPD file
#
# @retvel  PlatformBuildOptions         A list of platform Build Options loaded from XmlFpd
#
def LoadBuildOptions(XmlBuildOptions):
    XmlTag = "Option"
    return map(LoadBuildOption, XmlList(XmlBuildOptions, XmlTag)) # LoadBuildOption is a method in MigrationUtilities.py

## Load Platform Build Option
#
# Read an input Platform XML DOM object and return a Build Option
# contained in the DOM object.
#
# @param   XmlFpd                      An XML DOM object read from FPD file
#
# @retvel  PlatformBuildOption         A Build Options loaded from XmlFpd
#
def LoadPlatformBuildOption(XmlBuildOptions):
    PlatformBuildOption = PlatformBuildOptionClass()
    
    # handle UserDefinedAntTasks
    XmlTag = "BuildOptions/UserDefinedAntTasks/AntTask"
    PlatformBuildOption.UserDefinedAntTasks = LoadUserDefinedAntTasks(XmlTag)
    
    # handle Options
    XmlTag = "BuildOptions/Options/Option"
    PlatformBuildOption.Options = map(LoadBuildOption, XmlList(XmlBuildOptions, XmlTag))
    
    # handle UserExtensions
    XmlTag = "BuildOptions/UserExtensions"
    PlatformBuildOption.UserExtensions = LoadUserExtensions(XmlTag) # from MigrationUtilities.py LoadUserExtensions

    # handle Ffs
    XmlTag = "BuildOptions/Ffs/FfsKey"
    PlatformBuildOption.FfsKeyList = map(LoadPlatformFfs, XmlList(XmlBuildOptions, XmlTag))

    return PlatformBuildOption

## Load Platform Ffs Dictionary
#
# Read an input Platform XML DOM object and return a platform Ffs Dictionary
# contained in the DOM object.
#
# @param  XmlFpd     An XML DOM object read from FPD file
#
# @retvel  Dict      A platform Ffs Dict loaded from XmlFpd
#
def LoadPlatformFfsDict(XmlFpd):
    Dict = {}
    XmlTag = "PlatformSurfaceArea/BuildOptions/Ffs"
    List = map(LoadPlatformFfs, XmlList(XmlFpd, XmlTag))
    if List != []:
        for Ffs in List:
            Dict[Ffs.Key] = Ffs
    return Dict

## Load Platform Ffs Section
#
# Read an input Platform XML DOM object and return a platform Ffs Section
# contained in the DOM object.
#
# @param   XmlFfs                  An XML DOM object read from FPD file
#
# @retvel  PlatformFfsSection      A platform Ffs Section loaded from XmlFpd
#
def LoadPlatformFfsSection(XmlFfsSection):
    PlatformFfsSection = PlatformFfsSectionClass()
    
    XmlTag = ""
    PlatformFfsSection.BindingOrder = ''
    
    XmlTag = ""
    PlatformFfsSection.Compressible = ''
    
    XmlTag = "SectionType"
    PlatformFfsSection.SectionType = XmlAttribute(XmlFfsSection, XmlTag)
    
    XmlTag = ""
    PlatformFfsSection.EncapsulationType = ''
    
    XmlTag = ""
    PlatformFfsSection.ToolName = ''
    
    XmlTag = ""
    PlatformFfsSection.Filenames = []
    
    XmlTag = ""
    PlatformFfsSection.Args = ''
    
    XmlTag = ""
    PlatformFfsSection.OutFile = ''
    
    XmlTag = ""
    PlatformFfsSection.OutputFileExtension = ''
    
    XmlTag = ""
    PlatformFfsSection.ToolNameElement = ''
    
    return PlatformFfsSection

## Load Platform Ffs Sections
#
# Read an input Platform XML DOM object and return a platform Ffs Sections
# contained in the DOM object.
#
# @param   XmlFfs                   An XML DOM object read from FPD file
#
# @retvel  PlatformFfsSections      A platform Ffs Sections loaded from XmlFpd
#
def LoadFfsSections():
    PlatformFfsSections = PlatformFfsSectionsClass()
    PlatformFfsSections.BindingOrder = ''
    PlatformFfsSections.Compressible = ''
    PlatformFfsSections.SectionType = ''
    PlatformFfsSections.EncapsulationType = ''
    PlatformFfsSections.ToolName = ''
    PlatformFfsSections.Section = []
    PlatformFfsSections.Sections = []
    
    return PlatformFfsSections

## Load Platform Ffs Sections
#
# Read an input Platform XML DOM object and return a platform Ffs Sections
# contained in the DOM object.
#
# @param   XmlFfs                   An XML DOM object read from FPD file
#
# @retvel  PlatformFfsSections      A platform Ffs Sections loaded from XmlFpd
#
def LoadPlatformFfsSections(XmlFfsSections):
    PlatformFfsSections = PlatformFfsSectionsClass()
    
    XmlTag = ""
    PlatformFfsSections.BindingOrder = ''
    
    XmlTag = ""
    Compressible = ''
    
    XmlTag = ""
    SectionType = ''
    
    XmlTag = "EncapsulationType"
    EncapsulationType = XmlAttribute(XmlFfsSections, XmlTag)
    
    XmlTag = ""
    ToolName = ''
    
    XmlTag = "Sections/Section"
    Section = []   #[ PlatformFfsSectionClass, ... ]
    Section = map(LoadPlatformFfsSection, XmlList(XmlFfsSections, XmlTag))
    
    
    XmlTag = "Sections/Sections"
    Sections = map(LoadFfsSections, XmlList(XmlFfsSections, XmlTag)) #[ PlatformFfsSectionsClass, ...]
    
    return PlatformFfsSections

## Load Platform Ffs Attribute
#
# Read an input Platform XML DOM object and return a platform Ffs Attribute
# contained in the DOM object.
#
# @param   XmlFfs    An XML DOM object read from FPD file
#
# @retvel  List      A platform Ffs Attribute loaded from XmlFpd
#
def LoadFfsAttribute(XmlFfs):
    List = []
    XmlTag = "Ffs/Attribute"
    for XmlAttr in XmlList(XmlFfs, XmlTag):
        XmlTag = "Name"
        Name = XmlAttribute(XmlAttr, XmlTag)
        XmlTag = "Value"
        Value = XmlAttribute(XmlAttr, XmlTag)
        List.append([Name,Value])
    return List

## Load a list of Platform Build Options
#
# Read an input Platform XML DOM object and return a list of Build Options
# contained in the DOM object.
#
# @param   XmlFfs              An XML DOM object read from FPD file
#
# @retvel  PlatformFfsKey      A platform Ffs key loaded from XmlFpd
#
def LoadPlatformFfs(XmlFfs):
    PlatformFfs = PlatformFfsClass()
    
    PlatformFfs.Attribute = {}
    Dict = {}

    List = LoadFfsAttribute(XmlFfs)
    
    XmlTag = "Ffs/Sections/Sections"
    PlatformFfs.Sections = map(LoadPlatformFfsSections, XmlList(XmlFfs, XmlTag)) #[PlatformFfsSectionsClass, ...]
    
    for Item in List:
        Name = Item[0]
        Value = Item[1]
        for Item in PlatformFfs.Sections:
            Dict[(Name, Item)] = Value
    PlatformFfs.Attribute = Dict
    
    XmlTag = "Ffs/FfsKey"
    PlatformFfs.Key = XmlAttribute(XmlFfs, XmlTag)
    
    return PlatformFfs

## Load a list of Platform Build Options
#
# Read an input Platform XML DOM object and return a list of Build Options
# contained in the DOM object.
#
# @param   XmlFpd                       An XML DOM object read from FPD file
#
# @retvel  PlatformBuildOptions         A list of Build Options loaded from XmlFpd
#
def LoadPlatformBuildOptions(XmlFpd):
    PlatformBuildOptions = PlatformBuildOptionClass()

    PlatformBuildOptions.UserDefinedAntTasks = LoadUserDefinedAntTasks(XmlFpd)
    
    XmlTag = "PlatformSurfaceArea/BuildOptions/Options/Option"
    PlatformBuildOptions.Options = map(LoadBuildOption, XmlList(XmlFpd, XmlTag))

    PlatformBuildOptions.UserExtensions = LoadPlatformUserExtension(XmlFpd)
    
    PlatformBuildOptions.FfsKeyList = LoadPlatformFfsDict(XmlFpd)
    
    return PlatformBuildOptions

## Load Platform Pcd Data
#
# Read an input Platform XML DOM object and return Platform module class object
# contained in the DOM object.
#
# @param    XmlPcd             An XML DOM object read from FPD file
#
# @retvel   PlatformPcdData    A Platform Pcd object loaded from XmlFpd
#
def LoadPlatformPcdData(XmlPcdData):
    PcdData = PcdClass() # defined in CommonDataClass.CommonClass.py

    XmlTag = "ItemType"
    PcdData.ItemType = XmlAttribute(XmlPcdData, XmlTag) #DYNAMIC

    XmlTag = "PcdData/C_Name"
    PcdData.C_NAME = XmlElement(XmlPcdData, XmlTag)
    
    XmlTag = "PcdData/Token"
    PcdData.Token = XmlElement(XmlPcdData, XmlTag)
    
    XmlTag = "PcdData/TokenSpaceGuidCName"
    PcdData.TokenSpaceGuidCName = XmlElement(XmlPcdData, XmlTag)
    
    XmlTag = "PcdData/DatumType"
    PcdData.DatumType = XmlElement(XmlPcdData, XmlTag)
    
    XmlTag = "PcdData/MaxDatumSize"
    PcdData.MaxDatumSize = XmlElement(XmlPcdData, XmlTag)
    
    XmlTag = "PcdData/Value"
    PcdData.Value = XmlElement(XmlPcdData, XmlTag)
    
    return PcdData

## Load a Platform Pcd Build Data
#
# Read an input Platform XML DOM object and return a list of Pcd Dynamic
# contained in the DOM object.
#
# @param    XmlPcdBuildData        An XML DOM object read from FPD file
#
# @retvel   PcdBuildData           A Platform Pcd Build Data loaded from XmlFpd
#
def LoadPlatformPcdBuildData(XmlPcdBuildData):
    PcdBuildData = PcdClass() # defined in CommonDataClass.CommonClass.py

    XmlTag = "ItemType"
    PcdBuildData.ItemType = XmlAttribute(XmlPcdBuildData, XmlTag) #DYNAMIC

    XmlTag = "PcdBuildData/C_Name"
    PcdBuildData.C_NAME = XmlElement(XmlPcdBuildData, XmlTag)

    XmlTag = "PcdBuildData/Token"
    PcdBuildData.Token = XmlElement(XmlPcdBuildData, XmlTag)

    XmlTag = "PcdBuildData/TokenSpaceGuidCName"
    PcdBuildData.TokenSpaceGuidCName = XmlElement(XmlPcdBuildData, XmlTag)

    XmlTag = "PcdBuildData/DatumType"
    PcdBuildData.DatumType = XmlElement(XmlPcdBuildData, XmlTag)

    XmlTag = "PcdBuildData/MaxDatumSize"
    PcdBuildData.MaxDatumSize = XmlElement(XmlPcdBuildData, XmlTag)
    
    #XmlTag = "PcdBuildData/Value"
    #PcdBuildData.Value = XmlElement(XmlPcdBuildData, XmlTag)

    return PcdBuildData

## Load a list of Platform Pcd Dynamic
#
# Read an input Platform XML DOM object and return a list of Pcd Dynamic
# contained in the DOM object.
#
# @param    XmlFpd             An XML DOM object read from FPD file
#
# @retvel   PcdDynamic         A list of Pcd Dynamic loaded from XmlFpd
#
def LoadDynamicPcdBuildDefinitions(XmlFpd):
    DynamicPcdBuildDefinitions = []
    XmlTag = "PlatformSurfaceArea/DynamicPcdBuildDefinitions/PcdBuildData"
    return map(LoadPlatformPcdBuildData, XmlList(XmlFpd, XmlTag))

## Load a Platform NameValue object
#
# Read an input Platform XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlNameValue       An XML DOM object read from FPD file
#
# @retvel NameValue          A Platform NameValue object
#
def LoadNameValue(XmlNameValue):
    NameValue = []
    
    XmlTag = "Name"
    Name = XmlAttribute(XmlNameValue, XmlTag)
    NameValue.append(Name)

    XmlTag = "Value"
    Value = XmlAttribute(XmlNameValue, XmlTag)
    NameValue.append(Value)
    
    return NameValue

## Load a Platform Fv Image Name object
#
# Read an input Platform XML DOM object and return a platform Fv Image
# Name contained in the DOM object.
#
# @param  XmlFvImageNames       An XML DOM object read from FPD file
#
# @retvel FvImageNames          A Platform Fv Image Name object
#
def LoadFvImageNames(XmlFvImageNames):
    XmlTag = "FvImageNames"
    FvImageNames = XmlElement(XmlFvImageNames, XmlTag)
    return FvImageNames

## Load a Platform Fv Image option object
#
# Read an input Platform XML DOM object and return a platform Fv Image
# Option contained in the DOM object.
#
# @param  XmlFvImageOptions         An XML DOM object read from FPD file
#
# @retvel PlatformFvImageOption     A Platform Fv Image Option object
#
def LoadFvImageOptions(XmlFvImageOptions):
    PlatformFvImageOption = PlatformFvImageOptionClass()
    
    XmlTag = ""
    PlatformFvImageOption.FvImageOptionName = ''
    
    XmlTag = ""
    PlatformFvImageOption.FvImageOptionValues = []

    XmlTag = "FvImageOptions/NameValue"
    List = map(LoadNameValue, XmlList(XmlFvImageOptions, XmlTag))
    
    return PlatformFvImageOption
    
## Load a Platform Fv Image object
#
# Read an input Platform XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlFvImage          An XML DOM object read from Fpd file
#
# @retvel PlatformFvImage     A Platform Fv Image object
#
def LoadPlatformFvImage(XmlFvImage):
    PlatformFvImage = PlatformFvImageClass()

    XmlTag = "Name"
    PlatformFvImage.Name = XmlAttribute(XmlFvImage, XmlTag)

    XmlTag = "Value"
    PlatformFvImage.Value = XmlAttribute(XmlFvImage, XmlTag)

    XmlTag = "Type"
    PlatformFvImage.Type = XmlAttribute(XmlFvImage, XmlTag)

    XmlTag = "FvImage/FvImageNames"
    PlatformFvImage.FvImageNames = map(LoadFvImageNames, XmlList(XmlFvImage, XmlTag))

    XmlTag = "FvImage/FvImageOptions"
    PlatformFvImage.FvImageOptions = map(LoadFvImageOptions, XmlList(XmlFvImage, XmlTag))
    
    return PlatformFvImage

## Load a Platform fdf object
#
# Read an input Platform XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlFvImages          An XML DOM object read from FPD file
#
# @retvel PlatformFdf          A Platform fdf object
#
def LoadPlatformFvImages(XmlFvImages):
    List = []
    
    XmlTag = "FvImages/NameValue"
    NameValues = map(LoadNameValue, XmlList(XmlFvImages, XmlTag))
    List.append(NameValues)
    
    XmlTag = "FvImages/FvImage"
    FvImages = map(LoadPlatformFvImage, XmlList(XmlFvImages, XmlTag))
    List.append(FvImages)
    
    XmlTag = "FvImages/FvImageName"
    FvImageNames = map(LoadPlatformFvImageName, XmlList(XmlFvImages, XmlTag))
    List.append(FvImageNames)
    
    return List

## Load a Platform Fv Image Name object
#
# Read an input Platform XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlFvImageName        An XML DOM object read from FPD file
#
# @retvel PlatformFvImageName   A Platform Fv Image Name object
#
def LoadPlatformFvImageName(XmlFvImageName):
    PlatformFvImageName = PlatformFvImageNameClass()

    XmlTag = "Name"
    PlatformFvImageName.Name = XmlAttribute(XmlFvImageName, XmlTag)
    
    XmlTag = "Type"
    PlatformFvImageName.Type = XmlAttribute(XmlFvImageName, XmlTag)
    
    XmlTag = "FvImageOptions"
    PlatformFvImageName.FvImageOptions = map(LoadFvImageOptions, XmlList(XmlFvImageName, XmlTag))
    
    return PlatformFvImageName
    
## Load a list of Platform fdf objects
#
# Read an input Platform XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlFpd                An XML DOM object read from FPD file
#
# @retvel PlatformFdfs          A list of Platform fdf object
#
def LoadPlatformFdfs(XmlFpd):
    PlatformFvImages = PlatformFvImagesClass()

    XmlTag = "PlatformSurfaceArea/Flash/FvImages"
    PlatformFvImages.FvImages = map(LoadPlatformFvImages, XmlList(XmlFpd, XmlTag))

    return PlatformFvImages

## Load a Platform User Extensions
#
# Read an input Platform XML DOM object and return an User Extension
# contained in the DOM object.
#
# @param  XmlUserExtension             An XML DOM object read from FPD file
#
# @retvel PlatformUserExtensions       A platform User Extension loaded from XmlFpd
#
def LoadPlatformUserExtension(XmlFpd):
    Dict = {}
    
    PlatformUserExtensions = UserExtensionsClass()

    XmlTag = "PlatformSurfaceArea/BuildOptions/UserExtensions"
    List = map(LoadUserExtensions, XmlList(XmlFpd, XmlTag))
    if List != []:
        for Item in List:
            UserID = Item.UserID
            Identifier = Item.Identifier
            Dict[(UserID, Identifier)] = Item
    #XmlTag = "PlatformSurfaceArea/BuildOptions/UserExtensions/UserID"
    #PlatformUserExtensions.UserID = XmlAttribute(XmlFpd, XmlTag)
    
    #XmlTag = "PlatformSurfaceArea/BuildOptions/UserExtensions/Identifier"
    #PlatformUserExtensions.Identifier = XmlAttribute(XmlFpd, XmlTag)
    
    #PlatformUserExtensions.Content = XmlElementData(XmlFpd)
    #Dict[(PlatformUserExtensions.UserID,PlatformUserExtensions.Identifier)] = PlatformUserExtensions
    #return PlatformUserExtensions
    return Dict

## Load a list of Platform User Extensions
#
# Read an input Platform XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlFpd               An XML DOM object read from FPD file
#
# @retvel UserExtensions       A list of platform User Extensions loaded from XmlFpd
#
def LoadPlatformUserExtensions(XmlFpd):
    XmlTag = "PlatformSurfaceArea/UserExtensions"
    return map(LoadUserExtensions, XmlList(XmlFpd, XmlTag)) # from MigrationUtilities.py LoadUserExtensions

## Load a new Platform class object
#
# Read an input FPD File and return a new Platform class Object.
#
# @param   FpdFileName              An XML DOM object read from FPD file
#
# @retvel  Platform                 A new Platform class object loaded from FPD File
#
def LoadFpd(FpdFileName):
    XmlFpd = XmlParseFile(FpdFileName)
    EdkLogger.verbose("Load FPD File: %s" % FpdFileName)
    
    Platform = PlatformClass()
    Platform.Header = LoadPlatformHeader(XmlFpd, FpdFileName)
    Platform.SkuInfos = LoadPlatformSkuInfos(XmlFpd)
    Platform.Libraries = [] #New in dsc spec, do not handle for now
    Platform.LibraryClasses = LoadPlatformLibraryClasses(XmlFpd)
    Platform.Modules = LoadPlatformModules(XmlFpd)
    Platform.FlashDefinitionFile = LoadPlatformFlashDefinitionFile(XmlFpd, FpdFileName)
    Platform.BuildOptions = LoadPlatformBuildOptions(XmlFpd)
    Platform.DynamicPcdBuildDefinitions = LoadDynamicPcdBuildDefinitions(XmlFpd)
    Platform.Fdf = LoadPlatformFdfs(XmlFpd)
    Platform.UserExtensions = LoadPlatformUserExtensions(XmlFpd)

    return Platform

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    pass