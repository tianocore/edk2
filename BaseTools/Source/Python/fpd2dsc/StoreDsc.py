## @file
# Store a Platform class object to an INF file.
#
# Copyright (c) 2007 - 2009, Intel Corporation
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
from LoadFpd import LoadFpd
from CommonDataClass.PlatformClass import *
from CommonDataClass.FdfClass import *
from Common.MigrationUtilities import *
from Common.ToolDefClassObject import *
from Common.TargetTxtClassObject import *

## Store Defines section
#
# Write [Defines] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Defines section
# @param  Platform               An input Platform class object
#
def StorePlatformDefinesSection(DscFile, Platform):
    PlatformHeader = Platform.Header
    
    DefinesTupleList = []
    
    if PlatformHeader.Name != "":
        DefinesTupleList.append(("PLATFORM_NAME", PlatformHeader.Name))

    if PlatformHeader.Guid != "":
        DefinesTupleList.append(("PLATFORM_GUID", PlatformHeader.Guid))

    if PlatformHeader.Version != "":
        DefinesTupleList.append(("PLATFORM_VERSION", PlatformHeader.Version))
    for key in PlatformHeader.Specification.keys():
        SpecificationValue = PlatformHeader.Specification.get(key)
        DefinesTupleList.append(("DSC_ SPECIFICATION", SpecificationValue))
    
    if PlatformHeader.OutputDirectory != "":
        DefinesTupleList.append(("OUTPUT_DIRECTORY", PlatformHeader.OutputDirectory))

    if PlatformHeader.SupArchList != "":
        String = "|".join(PlatformHeader.SupArchList)
        DefinesTupleList.append(("SUPPORTED_ARCHITECTURES", String))

    if PlatformHeader.BuildTargets != "":
        String = "|".join(PlatformHeader.BuildTargets)
        DefinesTupleList.append(("BUILD_TARGETS", String))

    if PlatformHeader.SkuIdName != "":
        #DefinesTupleList.append(("SKUID_IDENTIFIER", PlatformHeader.SkuIdName))
        String = "|".join(PlatformHeader.SkuIdName)
        if String != "":
            DefinesTupleList.append(("SKUID_IDENTIFIER", String))
        
	String = Platform.FlashDefinitionFile.FilePath
	if String != "":
	    DefinesTupleList.append(("FLASH_DEFINITION", String))

    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Defines Section - statements that will be processed to create a Makefile.")
    List.append("#")
    List.append("################################################################################")
    Section = "\n".join(List)
    Section += "\n"
    StoreTextFile(DscFile, Section)
    
    StoreDefinesSection(DscFile, DefinesTupleList)

## Store SkuIds section
#
# Write [SkuIds] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Library Classes section
# @param  Platform               An input Platform class object
#
def StorePlatformSkuIdsSection(DscFile, Platform):
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# SKU Identification section - list of all SKU IDs supported by this Platform.")
    List.append("#")
    List.append("################################################################################")
    Section = "\n".join(List)
    Section += "\n"
    
    Section += "[SkuIds]" + '\n'
    
    List = Platform.SkuInfos.SkuInfoList
    for Item in List:
        Section = Section + "%s" % Item[0] + '|' + "%s" % Item[1] + '\n'
    Section = Section + '\n'
    
    StoreTextFile(DscFile, Section)

## Store Build Options section
#
# Write [BuildOptions] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Build Options section
# @param  Platform               An input Platform class object
#
def StorePlatformBuildOptionsSection(DscFile, Platform):
    # which is from tools_def.txt
    StandardBuildTargets = ["DEBUG", "RELEASE"]
    SupportedArches = ["COMMON", "IA32", "X64", "IPF", "EBC", "ARM"]
    Target = TargetTxtClassObject()
    WorkSpace = os.getenv('WORKSPACE')
    Target.LoadTargetTxtFile(WorkSpace + '\\Conf\\target.txt')
    ToolDef = ToolDefClassObject()
    ToolDef.LoadToolDefFile(WorkSpace + '\\' + Target.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF])
    # Now we have got ToolDef object
    #ToolDef.ToolsDefTxtDictionary
    Dict = ToolDef.ToolsDefTxtDatabase

    Dict1 = ToolDef.ToolsDefTxtDictionary # we care the info in this Dict
    #
    # We only support *(DEBUG/RELEASE) and *(All Arch: IA32, X64, IPF and EBC) for now
    #
    SectionWINDDK = ''
    SectionVS2003 = ''
    SectionVS2005EXP = ''
    SectionVS2005STD = ''
    SectionVS2005PRO = ''
    SectionVS2005TEAMSUITE = ''
    SectionUNIXGCC = ''
    SectionCYGWINGCC = ''
    SectionELFGCC = ''
    SectionICC = ''
    SectionMYTOOLS = ''
    for key in Dict1.keys():
        if key.find("_CC_FLAGS") != -1:
            if key.find('WINDDK3790x1830') != -1:
                SectionWINDDK = "  =  " + Dict1.get(key) + "\n"
            elif key.find('VS2003') != -1:
                SectionVS2003 = "  =  " + Dict1.get(key)+ "\n"
            elif key.find('VS2005EXP') != -1:
                SectionVS2005EXP = "  =  " + Dict1.get(key) + "\n"
            elif key.find('VS2005STD') != -1:
                SectionVS2005STD = "  =  " + Dict1.get(key) + "\n"
            elif key.find('VS2005PRO') != -1:
                SectionVS2005PRO = "  =  " + Dict1.get(key) + "\n"
            elif key.find('VS2005TEAMSUITE') != -1:
                SectionVS2005TEAMSUITE = "  =  " + Dict1.get(key) + "\n"
            elif key.find('UNIXGCC') != -1:
                SectionUNIXGCC = "  =  " + Dict1.get(key) + "\n"
            elif key.find('CYGWINGCC') != -1:
                SectionCYGWINGCC = "  =  " + Dict1.get(key) + "\n"
            elif key.find('ELFGCC') != -1:
                SectionELFGCC = "  =  " + Dict1.get(key) + "\n"
            elif key.find('ICC') != -1:
                SectionICC = "  =  " + Dict1.get(key) + "\n"
            elif key.find('MYTOOLS') != -1:
                SectionMYTOOLS = "  =  " + Dict1.get(key) + "\n"
            else:
                print "Error!"

    #
    # First need to check which arch
    #
    Archs = Platform.Header.SupArchList
    BuildTargets = Platform.Header.BuildTargets
    #if BuildTargets == StandardBuildTargets:
        #print "Debug and Release both support" # skip debug/release string search
    #else:
        #print "need to search debug/release string"

    if len(Archs) == 4:
        Arch = "*"
        SectionName = "[BuildOptions.Common]\n"
    else:
        for Arch in Archs:
            if Arch == 'IA32':
                SectionName = "[BuildOptions.IA32]\n"
            elif Arch == 'X64':
                SectionName = "[BuildOptions.X64]\n"
            elif Arch == 'IPF':
                SectionName = "[BuildOptions.IPF]\n"
            elif Arch == 'EBC':
                SectionName = "[BuildOptions.EBC]\n"
            else:
                print 'Error!'
    Section = ""
    if SectionWINDDK != "":
        SectionWINDDK = "*_WINDDK3790x1830_" + Arch + "_CC_FLAGS" + SectionWINDDK
        Section += SectionWINDDK
    if SectionVS2003 != "":
        SectionVS2003 = "*_VS2003_" + Arch + "_CC_FLAGS" + SectionVS2003
        Section += SectionVS2003
    if SectionVS2005EXP != "":
        SectionVS2005EXP = "*_VS2005EXP_" + Arch + "_CC_FLAGS" + SectionVS2005EXP
        Section += SectionVS2005EXP
    if SectionVS2005STD != "":
        SectionVS2005STD = "*_VS2005STD_" + Arch + "_CC_FLAGS" + SectionVS2005STD
        Section += SectionVS2005STD
    if SectionVS2005PRO != "":
        SectionVS2005PRO = "*_VS2005PRO_" + Arch + "_CC_FLAGS" + SectionVS2005PRO
        Section += SectionVS2005PRO
    if SectionVS2005TEAMSUITE != "":
        SectionVS2005TEAMSUITE = "*_VS2005TEAMSUITE_" + Arch + "_CC_FLAGS" + SectionVS2005TEAMSUITE
        Section += SectionVS2005TEAMSUITE
    if SectionUNIXGCC != "":
        SectionUNIXGCC = "*_UNIXGCC_" + Arch + "_CC_FLAGS" + SectionUNIXGCC
        Section += SectionUNIXGCC
    if SectionCYGWINGCC != "":
        SectionCYGWINGCC = "*_CYGWINGCC_" + Arch + "_CC_FLAGS" + SectionCYGWINGCC
        Section += SectionCYGWINGCC
    if SectionELFGCC != "":
        SectionELFGCC = "*_ELFGCC_" + Arch + "_CC_FLAGS" + SectionELFGCC
        Section += SectionELFGCC
    if SectionICC != "":
        SectionICC = "*_ICC_" + Arch + "_CC_FLAGS" + SectionICC
        Section += SectionICC
    if SectionMYTOOLS != "":
        SectionMYTOOLS = "*_MYTOOLS_" + Arch + "_CC_FLAGS" + SectionMYTOOLS
        Section += SectionMYTOOLS

    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Build Options section - list of all Build Options supported by this Platform.")
    List.append("#")
    List.append("################################################################################")
    SectionHeader = "\n".join(List)
    SectionHeader += "\n"
    
    Section = SectionHeader + SectionName + Section
    Section += "\n"
    StoreTextFile(DscFile, Section)

## Store Libraries section
#
# Write [Libraries] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Library Classes section
# @param  Platform               An input Platform class object
#
def StorePlatformLibrariesSection(DscFile,Platform):
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Libraries section - list of all Libraries needed by this Platform.")
    List.append("#")
    List.append("################################################################################")
    SectionHeader = "\n".join(List)
    SectionHeader += "\n"
    
    Section = SectionHeader + '[Libraries]\n\n'
    StoreTextFile(DscFile, Section)

## Return a Platform Library Class Item
#
# Read the input LibraryClass class object and return one line of Library Class Item.
#
# @param  LibraryClass         An input LibraryClass class object
#
# @retval LibraryClassItem     A Module Library Class Item
#
def GetPlatformLibraryClassItem(LibraryClass):
    LibraryClassList = []
    LibraryClassList.append(LibraryClass.Name)
    LibraryClassList.append(LibraryClass.FilePath)

    return "|$(WORKSPACE)/".join(LibraryClassList).rstrip("|")

## Add item to a LibraryClass section
#
# Add an Item with specific Module Type to section dictionary.
# The possible duplication is ensured to be removed.
#
# @param  Section            Section dictionary indexed by CPU architecture
# @param  SupModuleList      LibraryClass SupModuleList: BASE, SEC, PEI_CORE, PEIM, etc
# @param  Item               The Item to be added to section dictionary
#
def AddToLibraryClassSection(Section, SupModuleList, Item):
    for ModuleType in SupModuleList:
        SectionModule = Section.get(ModuleType, [])
        if Item not in SectionModule:
            SectionModule.append(Item)
            Section[ModuleType] = SectionModule

## Get Library Classes section contents
#
# Return the content of section named SectionName.
# the contents is based on Methods and ObjectLists.
#
# @param  SectionName        The name of the section
# @param  Method             A function returning a string item of an object
# @param  ObjectList         The list of object
#
# @retval Section            The string content of a section
#
def GetLibraryClassesSection(SectionName, Method, ObjectList):
    SupportedArches = ["COMMON", "IA32", "X64", "IPF", "EBC"]
    ModuleTypes = ["BASE","SEC","PEI_CORE","PEIM","DXE_CORE","DXE_DRIVER","DXE_SMM_DRIVER","DXE_SAL_DRIVER","DXE_RUNTIME_DRIVER","UEFI_DRIVER","UEFI_APPLICATION"]
    SectionCommonDict = {}
    SectionIA32Dict = {}
    SectionX64Dict = {}
    SectionIPFDict = {}
    SectionEBCDict = {}
    #ObjectList = list(set(ObjectList)) # delete the same element in the list
    for Object in ObjectList:
        if Object == None:
            continue
        Item = Method(Object)
        if Item == "":
            continue
        Item = "  %s" % Item
        Arches = Object.SupArchList
        if len(Arches) == 4:
            ModuleType = Object.ModuleType
            # [LibraryClasses.Common.ModuleType]
            if ModuleType == "BASE":
                SupModuleList = ["BASE"]
                AddToLibraryClassSection(SectionCommonDict, SupModuleList, Item)
            else:
                #
                SupModuleList = Object.SupModuleList
                #AddToSection(SectionDict, "|".join(SupModuleList), Item)
                AddToLibraryClassSection(SectionCommonDict, SupModuleList, Item)
        else:
            # Arch
            for Arch in SupportedArches:
                if Arch.upper() in Arches:
                    if Arch == "IA32":
                        # [LibraryClasses.IA32.ModuleType]
                        ModuleType = Object.ModuleType
                        if ModuleType == "BASE":
                            SupModuleList = ["BASE"]
                            AddToLibraryClassSection(SectionIA32Dict, SupModuleList, Item)
                        else:
                            SupModuleList = Object.SupModuleList
                            AddToLibraryClassSection(SectionIA32Dict, SupModuleList, Item)
                    elif Arch == "X64":
                        # [LibraryClasses.X64.ModuleType]
                        ModuleType = Object.ModuleType
                        if ModuleType == "BASE":
                            SupModuleList = ["BASE"]
                            AddToLibraryClassSection(SectionX64Dict, SupModuleList, Item)
                        else:
                            SupModuleList = Object.SupModuleList
                            AddToLibraryClassSection(SectionX64Dict, SupModuleList, Item)
                    elif Arch == "IPF":
                        # [LibraryClasses.IPF.ModuleType]
                        ModuleType = Object.ModuleType
                        if ModuleType == "BASE":
                            SupModuleList = ["BASE"]
                            AddToLibraryClassSection(SectionIPFDict, SupModuleList, Item)
                        else:
                            SupModuleList = Object.SupModuleList
                            AddToLibraryClassSection(SectionIPFDict, SupModuleList, Item)
                    elif Arch == "EBC":
                        # [LibraryClasses.EBC.ModuleType]
                        ModuleType = Object.ModuleType
                        if ModuleType == "BASE":
                            SupModuleList = ["BASE"]
                            AddToLibraryClassSection(SectionEBCDict, SupModuleList, Item)
                        else:
                            SupModuleList = Object.SupModuleList
                            AddToLibraryClassSection(SectionEBCDict, SupModuleList, Item)

    Section = ""
    for ModuleType in ModuleTypes:
        SectionCommonModule = "\n".join(SectionCommonDict.get(ModuleType, []))
        if SectionCommonModule != "":
            Section += "[%s.Common.%s]\n%s\n" % (SectionName, ModuleType, SectionCommonModule)
            Section += "\n"
    for ModuleType in ModuleTypes:
        ListIA32 = SectionIA32Dict.get(ModuleType, [])
        if ListIA32 != []:
            SectionIA32Module = "\n".join(SectionIA32Dict.get(ModuleType, []))
            if SectionIA32Module != "":
                Section += "[%s.IA32.%s]\n%s\n" % (SectionName, ModuleType, SectionIA32Module)
                Section += "\n"
        ListX64 = SectionX64Dict.get(ModuleType, [])
        if ListX64 != []:
            SectionX64Module = "\n".join(SectionX64Dict.get(ModuleType, []))
            if SectionX64Module != "":
                Section += "[%s.X64.%s]\n%s\n" % (SectionName, ModuleType, SectionX64Module)
                Section += "\n"
        ListIPF = SectionIPFDict.get(ModuleType, [])
        if ListIPF != []:
            SectionIPFModule = "\n".join(SectionIPFDict.get(ModuleType, []))
            if SectionIPFModule != "":
                Section += "[%s.IPF.%s]\n%s\n" % (SectionName, ModuleType, SectionIPFModule)
                Section += "\n"
        ListEBC = SectionEBCDict.get(ModuleType, [])
        if ListEBC != []:
            SectionEBCModule = "\n".join(SectionEBCDict.get(ModuleType, []))
            if SectionEBCModule != "":
                Section += "[%s.EBC.%s]\n%s\n" % (SectionName, ModuleType, SectionEBCModule)
                Section += "\n"

    if Section != "":
        Section += "\n"
    return Section

## Store Library Classes section
#
# Write [LibraryClasses] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Library Classes section
# @param  Platform               An input Platform class object
#
def StorePlatformLibraryClassesSection(DscFile, Platform):
    Section = GetLibraryClassesSection("LibraryClasses", GetPlatformLibraryClassItem, Platform.LibraryClasses.LibraryList)
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Library Class section - list of all Library Classes needed by this Platform.")
    List.append("#")
    List.append("################################################################################")
    SectionHeader = "\n".join(List)
    SectionHeader += "\n"
    Section = SectionHeader + Section
    StoreTextFile(DscFile, Section)

## Store Pcd section
#
# Write [Pcd] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Build Options section
# @param  Platform               An input Platform class object
#
def StorePlatformPcdSection(DscFile, Platform):
    # {PcdsFixedAtBuild:String1, PcdsFixedAtBuild:String2, PcdsPatchableInModule:String3}
    SectionDict = {}
    #
    # [PcdsFixedAtBuild], [PcdsPatchableInModule] and [PcdsFeatureFlag] are from platform.modules
    # [PcdsDynamic] is from platform.DynamicPcdBuildDefinitions
    #
    Modules = Platform.Modules.ModuleList # it's a list of modules
    for Module in Modules:
        PcdBuildDefinitions = Module.PcdBuildDefinitions # it's a list of PcdData
        for PcdData in PcdBuildDefinitions:
            if PcdData.ItemType == "FEATURE_FLAG":
                List = []
                List.append(PcdData.TokenSpaceGuidCName + "." + PcdData.C_NAME)
                List.append(PcdData.Value)
                String = "|".join(List)
                ItemType = PcdData.ItemType
                SectionPcdsFeatureFlag = SectionDict.get(ItemType, [])
                if String not in SectionPcdsFeatureFlag:
                    SectionPcdsFeatureFlag.append(String)
                    SectionDict[ItemType] = SectionPcdsFeatureFlag
            else:
                List = []
                List.append(PcdData.TokenSpaceGuidCName + "." + PcdData.C_NAME)
                List.append(PcdData.Value)
                List.append(PcdData.Token)
                List.append(PcdData.DatumType)
                List.append(PcdData.MaxDatumSize)
                String = "|".join(List)
                ItemType = PcdData.ItemType
                if PcdData.ItemType == "FIXED_AT_BUILD":
                    SectionPcdsFixedAtBuild = SectionDict.get(ItemType, [])
                    if String not in SectionPcdsFixedAtBuild:
                        SectionPcdsFixedAtBuild.append(String)
                        SectionDict[ItemType] = SectionPcdsFixedAtBuild
                #elif PcdData.ItemType == "FEATURE_FLAG":
                    #SectionPcdsFeatureFlag = SectionDict.get(ItemType, [])
                    #if String not in SectionPcdsFeatureFlag:
                        #SectionPcdsFeatureFlag.append(String)
                        #SectionDict[ItemType] = SectionPcdsFeatureFlag
                elif PcdData.ItemType == "PATCHABLE_IN_MODULE":
                    SectionPcdsPatchableInModule = SectionDict.get(ItemType, [])
                    if String not in SectionPcdsPatchableInModule:
                        SectionPcdsPatchableInModule.append(String)
                        SectionDict[ItemType] = SectionPcdsPatchableInModule
                elif PcdData.ItemType == "DYNAMIC":
                    SectionPcdsDynamic = SectionDict.get(ItemType, [])
                    if String not in SectionPcdsDynamic:
                        SectionPcdsDynamic.append(String)
                        SectionDict[ItemType] = SectionPcdsDynamic

    DynamicPcdBuildDefinitions = Platform.DynamicPcdBuildDefinitions # It's a list
    for PcdBuildData in DynamicPcdBuildDefinitions:
        List = []
        List.append(PcdData.TokenSpaceGuidCName + "." + PcdData.C_NAME)
        List.append(PcdData.Token)
        List.append(PcdData.DatumType)
        List.append(PcdData.MaxDatumSize)
        String = "|".join(List)
        if PcdBuildData.ItemType == "DYNAMIC":
            ItemType = PcdBuildData.ItemType
            SectionPcdsDynamic = SectionDict.get(ItemType, [])
            if String not in SectionPcdsDynamic:
                SectionPcdsDynamic.append(String)
                SectionDict[ItemType] = SectionPcdsDynamic
    ItemType = "FIXED_AT_BUILD"
    Section = "[PcdsFixedAtBuild]\n  " + "\n  ".join(SectionDict.get(ItemType, []))
    ItemType = "FEATURE_FLAG"
    Section += "\n\n[PcdsFeatureFlag]\n  " + "\n  ".join(SectionDict.get(ItemType, []))
    ItemType = "PATCHABLE_IN_MODULE"
    Section += "\n\n[PcdsPatchableInModule]\n  " + "\n  ".join(SectionDict.get(ItemType, []))
    Section += "\n\n"
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform.")
    List.append("#")
    List.append("################################################################################")
    String = "\n".join(List)
    Section += String
    ItemType = "DYNAMIC"
    Section += "\n\n[PcdsDynamic]\n  " + "\n  ".join(SectionDict.get(ItemType, []))
    Section += "\n\n"
    
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Pcd Section - list of all EDK II PCD Entries defined by this Platform.")
    List.append("#")
    List.append("################################################################################")
    SectionHeader = "\n".join(List)
    SectionHeader += "\n"
    Section = SectionHeader + Section
    StoreTextFile(DscFile, Section)

## Add item to a section
#
# Add an Item with specific CPU architecture to section dictionary.
# The possible duplication is ensured to be removed.
#
# @param  Section            Section dictionary indexed by CPU architecture
# @param  Arch               CPU architecture: Ia32, X64, Ipf, Ebc or Common
# @param  Item               The Item to be added to section dictionary
#
def AddToSection(Section, Arch, Item):
    SectionArch = Section.get(Arch, [])
    if Item not in SectionArch:
        SectionArch.append(Item)
        Section[Arch] = SectionArch

## Get section contents
#
# Return the content of section named SectionName.
# the contents is based on Methods and ObjectLists.
#
# @param  SectionName        The name of the section
# @param  Method             A function returning a string item of an object
# @param  ObjectList         The list of object
#
# @retval Section            The string content of a section
#
def GetSection(SectionName, Method, ObjectList):
    SupportedArches = ["COMMON", "IA32", "X64", "IPF", "EBC"]
    SectionDict = {}
    for Object in ObjectList:
        if Object.FilePath == "":
            continue
        Item = Method(Object)
        if Item == "":
            continue
        Item = "  %s" % Item
        Arches = Object.SupArchList
        if len(Arches) == 4:
            AddToSection(SectionDict, "common", Item)
        else:
            for Arch in SupportedArches:
                if Arch.upper() in Arches:
                    AddToSection(SectionDict, Arch, Item)

    Section = ""
    for Arch in SupportedArches:
        SectionArch = "\n".join(SectionDict.get(Arch, []))
        if SectionArch != "":
            Section += "[%s.%s]\n%s\n" % (SectionName, Arch, SectionArch)
            Section += "\n"
    if Section != "":
        Section += "\n"
    return Section

## Return a Platform Component Item
#
# Read the input Platform Component object and return one line of Platform Component Item.
#
# @param  Component         An input Platform Component class object
#
# @retval ComponentItem     A Platform Component Item
#
def GetPlatformComponentItem(Component):
    List = []
    Section = {}

    List.append("$(WORKSPACE)/" + Component.FilePath)

    LibraryClasses = Component.LibraryClasses
    if LibraryClasses != []:
        List = []
        List.append("$(WORKSPACE)/" + Component.FilePath + " {")
        List.append("<LibraryClasses>")
        for LibraryClass in LibraryClasses:
            if LibraryClass == ["", ""]:
                continue
            List.append("  " + LibraryClass[0] + "|$(WORKSPACE)/" + LibraryClass[1])
            
    PcdBuildDefinitions = Component.PcdBuildDefinitions
    for PcdData in PcdBuildDefinitions:
        if PcdData.ItemType == "FEATURE_FLAG":
            List1 = []
            List1.append(PcdData.TokenSpaceGuidCName + "." + PcdData.C_NAME)
            List1.append(PcdData.Value)
            String = "|".join(List1)
            ItemType = PcdData.ItemType
            SectionPcd = Section.get(ItemType, [])
            if String not in SectionPcd:
                SectionPcd.append(String)
            Section[ItemType] = SectionPcd
        else:
            List1 = []
            List1.append(PcdData.TokenSpaceGuidCName + "." + PcdData.C_NAME)
            List1.append(PcdData.Value)
            List1.append(PcdData.Token)
            List1.append(PcdData.DatumType)
            List1.append(PcdData.MaxDatumSize)
            String = "|".join(List1)
            ItemType = PcdData.ItemType
            if ItemType == "FIXED_AT_BUILD":
                SectionPcd = Section.get(ItemType, [])
                if String not in SectionPcd:
                    SectionPcd.append(String)
                Section[ItemType] = SectionPcd
            #elif ItemType == "FEATURE_FLAG":
                #SectionPcd = Section.get(ItemType, [])
                #if String not in SectionPcd:
                    #SectionPcd.append(String)
                #Section[ItemType] = SectionPcd
            elif ItemType == "PATCHABLE_IN_MODULE":
                SectionPcd = Section.get(ItemType, [])
                if String not in SectionPcd:
                    SectionPcd.append(String)
                Section[ItemType] = SectionPcd
            elif ItemType == "DYNAMIC":
                SectionPcd = Section.get(ItemType, [])
                if String not in SectionPcd:
                    SectionPcd.append(String)
                Section[ItemType] = SectionPcd

    ItemType = "FIXED_AT_BUILD"
    if Section.get(ItemType, []) != []:
        List.append("<PcdsFixedAtBuild>")
        List.append("  " + "\n    ".join(Section.get(ItemType,[])))
    ItemType = "FEATURE_FLAG"
    if Section.get(ItemType, []) != []:
        List.append("<PcdsFeatureFlag>")
        List.append("  " + "\n    ".join(Section.get(ItemType,[])))
    ItemType = "PATCHABLE_IN_MODULE"
    if Section.get(ItemType, []) != []:
        List.append("<PcdsPatchableInModule>")
        List.append("  " + "\n    ".join(Section.get(ItemType,[])))
    ItemType = "DYNAMIC"
    if Section.get(ItemType, []) != []:
        List.append("<PcdsDynamic>")
        List.append("  " + "\n    ".join(Section.get(ItemType,[])))

    ListOption = []
    SectionOption = ""
    ListBuildOptions = Component.BuildOptions # a list
    if ListBuildOptions != []:
        SectionOption += "\n  <BuildOptions>\n"
        for BuildOptions in ListBuildOptions:
            Options = BuildOptions.Options
            for Option in Options:
                for Item in Option.BuildTargetList:
                    ListOption.append(Item)
                List.append(Option.ToolChainFamily)
                for Item in Option.SupArchList:
                    ListOption.append(Item)
                ListOption.append(Option.ToolCode)
                ListOption.append("FLAGS")
                #print ListOption
                SectionOption += "  " + "_".join(List) + "    =  " + Option.Option + "\n"
                ListOption = []
    if SectionOption != "":
        List.append(SectionOption)
    if List != ["$(WORKSPACE)/" + Component.FilePath]:
        List.append("}\n")

    return "\n  ".join(List)

## Store Components section.
#
# Write [Components] section to the DscFile based on Platform class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the Components section
# @param  Platform               An input Platform class object
#
def StorePlatformComponentsSection(DscFile, Platform):
    Section = GetSection("Components", GetPlatformComponentItem, Platform.Modules.ModuleList)
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# Components Section - list of all EDK II Modules needed by this Platform.")
    List.append("#")
    List.append("################################################################################")
    SectionHeader = "\n".join(List)
    SectionHeader += "\n"
    Section = SectionHeader + Section
    StoreTextFile(DscFile, Section)

## Store User Extensions section.
#
# Write [UserExtensions] section to the InfFile based on Module class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DscFile                The output DSC file to store the User Extensions section
# @param  Platform               An input Platform class object
#
def StorePlatformUserExtensionsSection(DscFile, Platform):
    Section = "".join(map(GetUserExtensions, Platform.UserExtensions))
    List = []
    List.append("################################################################################")
    List.append("#")
    List.append("# User Extensions Section - list of all User Extensions specified by user.")
    List.append("#")
    List.append("################################################################################")
    SectionHeader = "\n".join(List)
    SectionHeader += "\n"
    Section = SectionHeader + Section
    StoreTextFile(DscFile, Section)
    
## Store a Platform class object to a new DSC file.
#
# Read an input Platform class object and save the contents to a new DSC file.
#
# @param  DSCFileName             The output DSC file
# @param  Platform                An input Platform class object
#
def StoreDsc(DscFileName, Platform):
    DscFile = open(DscFileName, "w+")
    EdkLogger.info("Save file to %s" % DscFileName)

    StoreHeader(DscFile, Platform.Header)
    StorePlatformDefinesSection(DscFile, Platform)
    StorePlatformBuildOptionsSection(DscFile,Platform)
    StorePlatformSkuIdsSection(DscFile,Platform)
    StorePlatformLibrariesSection(DscFile,Platform) # new in dsc, Edk I components, list of INF files
    StorePlatformLibraryClassesSection(DscFile, Platform) # LibraryClasses are from Modules
    StorePlatformPcdSection(DscFile, Platform)
    #StorePlatformPcdDynamicSection(DscFile, Platform)
    StorePlatformComponentsSection(DscFile,Platform)
    StorePlatformUserExtensionsSection(DscFile,Platform)
    DscFile.close()
    
if __name__ == '__main__':
    pass
