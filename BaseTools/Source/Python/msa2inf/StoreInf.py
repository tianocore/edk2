## @file
# Store a Module class object to an INF file.
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
from LoadMsa import LoadMsa
from CommonDataClass.ModuleClass import *
from Common.MigrationUtilities import *

## Get the produced library class.
#
# Return the item of Library Class based on Library .
#
# @param  LibraryClasses     A list of library classes the module produces.
#
# @retval LibraryClassItem   A text format library class item.
#
def GetModuleLibraryClass(LibraryClasses):
    ProducedLibraryClasses = []
    for LibraryClass in LibraryClasses:
        ProducedLibraryClass = LibraryClass.LibraryClass
        SupportedModueTypes = " ".join(LibraryClass.SupModuleList)
        if SupportedModueTypes != "":
            ProducedLibraryClass += "|" + SupportedModueTypes
        ProducedLibraryClasses.append(ProducedLibraryClass)

    return "|".join(ProducedLibraryClasses)


## Store Defines section.
#
# Write [Defines] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Defines section.
# @param  Module               An input Module class object.
#
def StoreModuleDefinesSection(InfFile, Module):
    ModuleHeader = Module.Header
    
    DefinesTupleList = []
    DefinesTupleList.append(("INF_VERSION", ModuleHeader.InfVersion))
    
    if ModuleHeader.Name != "":
        DefinesTupleList.append(("BASE_NAME", ModuleHeader.Name))

    if ModuleHeader.Guid != "":
        DefinesTupleList.append(("FILE_GUID", ModuleHeader.Guid))

    if ModuleHeader.Version != "":
        DefinesTupleList.append(("VERSION_STRING", ModuleHeader.Version))
        
    if ModuleHeader.ModuleType != "":
        DefinesTupleList.append(("MODULE_TYPE", ModuleHeader.ModuleType))

    if ModuleHeader.EfiSpecificationVersion != "":
        DefinesTupleList.append(("EFI_SPECIFICATION_VERSION", ModuleHeader.EfiSpecificationVersion))
    
    if ModuleHeader.EdkReleaseVersion != "":
        DefinesTupleList.append(("EDK_RELEASE_VERSION", ModuleHeader.EdkReleaseVersion))
    
    ProducedLibraryClass = GetModuleLibraryClass(ModuleHeader.LibraryClass)
    if ProducedLibraryClass != "":
        DefinesTupleList.append(("LIBRARY_CLASS", ProducedLibraryClass))

    if ModuleHeader.MakefileName != "":
        DefinesTupleList.append(("MAKEFILE_NAME", ModuleHeader.MakeFileName))

    if ModuleHeader.PcdIsDriver != "":
        DefinesTupleList.append(("PCD_DRIVER", "TRUE"))

    if len(Module.ExternImages) > 0:
        ModuleEntryPoint = Module.ExternImages[0].ModuleEntryPoint
        ModuleUnloadImage = Module.ExternImages[0].ModuleUnloadImage
        if ModuleEntryPoint != "":
            DefinesTupleList.append(("ENTRY_POINT", ModuleEntryPoint))
        if ModuleUnloadImage != "":
            DefinesTupleList.append(("UNLOAD_IMAGE", ModuleUnloadImage))

    if len(Module.ExternLibraries) > 0:
        Constructor = Module.ExternLibraries[0].Constructor
        Destructor = Module.ExternLibraries[0].Destructor
        if Constructor != "":
            DefinesTupleList.append(("CONSTRUCTOR", Constructor))
        if Destructor != "":
            DefinesTupleList.append(("DESTRUCTOR", Destructor))

    StoreDefinesSection(InfFile, DefinesTupleList)
    

## Return a Module Source Item.
#
# Read the input ModuleSourceFile class object and return one line of Source Item.
#
# @param  ModuleSourceFile     An input ModuleSourceFile class object.
#
# @retval SourceItem           A Module Source Item.
#
def GetModuleSourceItem(ModuleSourceFile):
    Source = []
    Source.append(ModuleSourceFile.SourceFile)
    Source.append(ModuleSourceFile.ToolChainFamily)
    Source.append(ModuleSourceFile.TagName)
    Source.append(ModuleSourceFile.ToolCode)
    Source.append(ModuleSourceFile.FeatureFlag)
    return "|".join(Source).rstrip("|")
    

## Store Sources section.
#
# Write [Sources] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Sources section.
# @param  Module               An input Module class object.
#
def StoreModuleSourcesSection(InfFile, Module):
    Section = GetSection("Sources", GetModuleSourceItem, Module.Sources)
    StoreTextFile(InfFile, Section)


## Return a Module Binary Item.
#
# Read the input ModuleBinaryFile class object and return one line of Binary Item.
#
# @param  ModuleBinaryFile     An input ModuleBinaryFile class object.
#
# @retval BinaryItem           A Module Binary Item.
#
def GetModuleBinaryItem(ModuleBinaryFile):
    Binary = []
    Binary.append(ModuleBinaryFile.FileType)
    Binary.append(ModuleBinaryFile.BinaryFile)
    Binary.append(ModuleBinaryFile.Target)
    Binary.append(ModuleBinaryFile.FeatureFlag)
    return "|".join(Binary).rstrip("|")


## Store Binaries section.
#
# Write [Binaries] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Binaries section.
# @param  Module               An input Module class object.
#
def StoreModuleBinariesSection(InfFile, Module):
    Section = GetSection("Binaries", GetModuleBinaryItem, Module.Binaries)
    StoreTextFile(InfFile, Section)


## Return a Module Library Class Item.
#
# Read the input LibraryClass class object and return one line of Library Class Item.
#
# @param  LibraryClass         An input LibraryClass class object.
#
# @retval LibraryClassItem     A Module Library Class Item.
#
def GetModuleLibraryClassItem(LibraryClass):
    if "ALWAYS_PRODUCED" in LibraryClass.Usage:
        return ""

    LibraryClassList = []
    LibraryClassList.append(LibraryClass.LibraryClass)
    LibraryClassList.append(LibraryClass.RecommendedInstance)
    LibraryClassList.append(LibraryClass.FeatureFlag)
    
    return "|".join(LibraryClassList).rstrip("|")


## Store Library Classes section.
#
# Write [LibraryClasses] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Library Classes section.
# @param  Module               An input Module class object.
#
def StoreModuleLibraryClassesSection(InfFile, Module):
    Section = GetSection("LibraryClasses", GetModuleLibraryClassItem, Module.LibraryClasses)
    StoreTextFile(InfFile, Section)


## Return a Module Package Item.
#
# Read the input PackageDependency class object and return one line of Package Item.
#
# @param  PackageDependency    An input PackageDependency class object.
#
# @retval PackageItem          A Module Package Item.
#
def GetModulePackageItem(PackageDependency):
    return PackageDependency.FilePath


## Store Packages section.
#
# Write [Packages] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Packages section.
# @param  Module               An input Module class object.
#
def StoreModulePackagesSection(InfFile, Module):
    Section = GetSection("Packages", GetModulePackageItem, Module.PackageDependencies)
    StoreTextFile(InfFile, Section)
    

## Return a Module Guid C Name Item.
#
# Read the input Guid class object and return one line of Guid C Name Item.
#
# @param  Guid                 An input Guid class object.
#
# @retval GuidCNameItem        A Module Guid C Name Item.
#
def GetModuleGuidCNameItem(Guid):
    try:
        return Guid.GuidCName
    except:
        return Guid.CName


## Store Protocols section.
#
# Write [Protocols] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Protocols section.
# @param  Module               An input Module class object.
#
def StoreModuleProtocolsSection(InfFile, Module):
    Section = GetSection("Protocols", GetModuleGuidCNameItem, Module.Protocols)
    StoreTextFile(InfFile, Section)
    

## Store Ppis section.
#
# Write [Ppis] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Ppis section.
# @param  Module               An input Module class object.
#
def StoreModulePpisSection(InfFile, Module):
    Section = GetSection("Ppis", GetModuleGuidCNameItem, Module.Ppis)
    StoreTextFile(InfFile, Section)


## Store Guids section.
#
# Write [Guids] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Guids section.
# @param  Module               An input Module class object.
#
def StoreModuleGuidsSection(InfFile, Module):
    Guids = []
    Guids += Module.Guids
    Guids += Module.Events
    Guids += Module.Hobs
    Guids += Module.Variables
    Guids += Module.SystemTables
    Guids += Module.DataHubs
    Guids += Module.HiiPackages
    Section = GetSection("Guids", GetModuleGuidCNameItem, Guids)
    StoreTextFile(InfFile, Section)


## Return a Module Pcd Item.
#
# Read the input Pcd class object and return one line of Pcd Item.
#
# @param  Pcd                  An input Pcd class object.
#
# @retval PcdItem              A Module Pcd Item.
#
def GetModulePcdItem(Pcd):
    PcdItem = "%s.%s" % (Pcd.TokenSpaceGuidCName, Pcd.CName)
    if Pcd.DefaultValue != "":
        PcdItem = "%s|%s" % (PcdItem, Pcd.DefaultValue)

    return PcdItem


## DEC Pcd Section Name dictionary indexed by PCD Item Type.
mInfPcdSectionNameDict = {
    "FEATURE_FLAG" : "FeaturePcd",
    "FIXED_AT_BUILD" : "FixedPcd",
    "PATCHABLE_IN_MODULE" : "PatchPcd",
    "DYNAMIC" : "Pcd",
    "DYNAMIC_EX" : "PcdEx"
    }
    
## Store Pcds section.
#
# Write [(PcdType)] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Pcds section.
# @param  Module               An input Module class object.
#
def StoreModulePcdsSection(InfFile, Module):
    PcdsDict = {}
    for Pcd in Module.PcdCodes:
        PcdSectionName = mInfPcdSectionNameDict.get(Pcd.ItemType)
        if PcdSectionName:
            PcdsDict.setdefault(PcdSectionName, []).append(Pcd)
        else:
            EdkLogger.info("Unknown Pcd Item Type: %s" % Pcd.ItemType)

    Section = ""
    for PcdSectionName in PcdsDict:
        Pcds = PcdsDict[PcdSectionName]
        Section += GetSection(PcdSectionName, GetModulePcdItem, Pcds)
        Section += "\n"

    StoreTextFile(InfFile, Section)
    

## Return a Module Depex Item.
#
# Read the input Depex class object and return one line of Depex Item.
#
# @param  Depex                An input Depex class object.
#
# @retval DepexItem            A Module Depex Item.
#
def GetModuleDepexItem(Depex):
    return Depex.Depex


## Store Depex section.
#
# Write [Depex] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Depex section.
# @param  Module               An input Module class object.
#
def StoreModuleDepexSection(InfFile, Module):
    Section = GetSection("Depex", GetModuleDepexItem, Module.Depex)
    StoreTextFile(InfFile, Section)
    

## Return a Module Build Option Item.
#
# Read the input BuildOption class object and return one line of Build Option Item.
#
# @param  BuildOption          An input BuildOption class object.
#
# @retval BuildOptionItem      A Module Build Option Item.
#
def GetModuleBuildOptionItem(BuildOption):
    BuildTarget = BuildOption.BuildTarget
    if BuildTarget == "":
        BuildTarget = "*"

    TagName = BuildOption.TagName
    if TagName == "":
        TagName = "*"
        
    ToolCode = BuildOption.ToolCode
    if ToolCode == "":
        ToolCode = "*"

    Item = "_".join((BuildTarget, TagName, "*", ToolCode, "Flag"))

    ToolChainFamily = BuildOption.ToolChainFamily
    if ToolChainFamily != "":
        Item = "%s:%s" % (ToolChainFamily, Item)

    return "%-30s = %s" % (Item, BuildOption.Option)


## Store Build Options section.
#
# Write [BuildOptions] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  InfFile              The output INF file to store the Build Options section.
# @param  Module               An input Module class object.
#
def StoreModuleBuildOptionsSection(InfFile, Module):
    Section = GetSection("BuildOption", GetModuleBuildOptionItem, Module.BuildOptions)
    StoreTextFile(InfFile, Section)


## Store User Extensions section.
#
# Write [UserExtensions] section to the InfFile based on Module class object.
#
# @param  InfFile              The output INF file to store the User Extensions section.
# @param  Module               An input Module class object.
#
def StoreModuleUserExtensionsSection(InfFile, Module):
    Section = "".join(map(GetUserExtensions, Module.UserExtensions))
    StoreTextFile(InfFile, Section)


## Store a Module class object to a new INF file.
#
# Read an input Module class object and save the contents to a new INF file.
#
# @param  INFFileName          The output INF file.
# @param  Module               An input Package class object.
#
def StoreInf(InfFileName, Module):
    InfFile = open(InfFileName, "w+")
    EdkLogger.info("Save file to %s" % InfFileName)

    StoreHeader(InfFile, Module.Header)
    StoreModuleDefinesSection(InfFile, Module)
    StoreModuleSourcesSection(InfFile, Module)
    StoreModuleBinariesSection(InfFile, Module)
    StoreModulePackagesSection(InfFile, Module)
    StoreModuleLibraryClassesSection(InfFile, Module)
    StoreModuleProtocolsSection(InfFile, Module)
    StoreModulePpisSection(InfFile, Module)
    StoreModuleGuidsSection(InfFile, Module)
    StoreModulePcdsSection(InfFile, Module)
    StoreModuleDepexSection(InfFile, Module)
    StoreModuleBuildOptionsSection(InfFile, Module)
    StoreModuleUserExtensionsSection(InfFile, Module)

    InfFile.close()
    
if __name__ == '__main__':
    pass
