## @file
# Open an MSA file and load all its contents to a ModuleClass object.
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
from CommonDataClass.ModuleClass import *
from Common.XmlRoutines import *
from Common.MigrationUtilities import *


## Load a list of Module Cloned Records.
#
# Read an input Module XML DOM object and return a list of Cloned Records
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel ClonedRecords        A list of Cloned Records loaded from XmlMsa.
#
def LoadModuleClonedRecords(XmlMsa):
    XmlTag = "ModuleSurfaceArea/ModuleDefinitions/ClonedFrom/Cloned"
    return map(LoadClonedRecord, XmlList(XmlMsa, XmlTag))

## Load Module Header.
#
# Read an input Module XML DOM object and return Module Header class object
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
# @param  MsaFileName          The file path of MSA File.
#
# @retvel ModuleHeader         A new Module Header object loaded from XmlMsa.
#
def LoadModuleHeader(XmlMsa, MsaFileName):
    ModuleHeader = ModuleHeaderClass()
    
    XmlTag = "ModuleSurfaceArea/MsaHeader"
    MsaHeader = XmlNode(XmlMsa, XmlTag)
    
    SetIdentification(ModuleHeader, MsaHeader, "ModuleName", MsaFileName)
    SetCommonHeader(ModuleHeader, MsaHeader)

    XmlTag = "ModuleSurfaceArea/ModuleDefinitions/SupportedArchitectures"
    ModuleHeader.SupArchList = XmlElement(XmlMsa, XmlTag).split()

    XmlTag = "ModuleSurfaceArea/ModuleDefinitions/BinaryModule"
    if XmlElement(XmlMsa, XmlTag).lower() == "true":
        ModuleHeader.BinaryModule = True

    XmlTag = "ModuleSurfaceArea/ModuleDefinitions/OutputFileBasename"
    ModuleHeader.OutputFileBasename = XmlElement(XmlMsa, XmlTag)
    
    XmlTag = "ModuleSurfaceArea/ModuleDefinitions/ClonedForm"
    ModuleHeader.ClonedFrom = LoadModuleClonedRecords(XmlMsa)

    XmlTag = "ModuleSurfaceArea/Externs/PcdDriverTypes"
    ModuleHeader.PcdIsDriver = XmlElement(XmlMsa, XmlTag)
    
    XmlTag = "ModuleSurfaceArea/Externs/TianoR8FlashMap_h"
    if XmlElement(XmlMsa, XmlTag).lower() == "true":
        ModuleHeader.TianoR8FlashMap_h = True

    XmlTag = "ModuleSurfaceArea/Externs/Specification"
    for Specification in XmlElementList(XmlMsa, XmlTag):
        AddToSpecificationDict(ModuleHeader.Specification, Specification)
        
    return ModuleHeader


## Load a list of Module Library Classes.
#
# Read an input Module XML DOM object and return a list of Library Classes
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel LibraryClasses       A list of Library Classes loaded from XmlMsa.
#
def LoadModuleLibraryClasses(XmlMsa):
    XmlTag = "ModuleSurfaceArea/LibraryClassDefinitions/LibraryClass"
    return map(LoadLibraryClass, XmlList(XmlMsa, XmlTag))
    

## Load a new Module Source class object.
#
# Read an input XML Source DOM object and return an object of Source
# contained in the DOM object.
#
# @param  XmlFilename          A child XML DOM object in Module XML DOM.
#
# @retvel ModuleSource         A new Source object created by XmlFilename.
#
def LoadModuleSource(XmlFilename):
    ModuleSource = ModuleSourceFileClass()
    
    ModuleSource.SourceFile = XmlElementData(XmlFilename)

    XmlTag = "TagName"
    ModuleSource.TagName = XmlAttribute(XmlFilename, XmlTag)
    
    XmlTag = "ToolCode"
    ModuleSource.ToolCode = XmlAttribute(XmlFilename, XmlTag)
    
    XmlTag = "ToolChainFamily"
    ModuleSource.ToolChainFamily = XmlAttribute(XmlFilename, XmlTag)
    
    SetCommon(ModuleSource, XmlFilename)
    
    return ModuleSource


## Load a list of Module Sources.
#
# Read an input Module XML DOM object and return a list of Sources
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Sources              A list of Sources loaded from XmlMsa.
#
def LoadModuleSources(XmlMsa):
    XmlTag = "ModuleSurfaceArea/SourceFiles/Filename"
    return map(LoadModuleSource, XmlList(XmlMsa, XmlTag))


## Load a new Module Binary class object.
#
# Read an input XML Binary DOM object and return an object of Binary
# contained in the DOM object.
#
# @param  XmlFilename          A child XML DOM object in Module XML DOM.
#
# @retvel ModuleBinary         A new Binary object created by XmlFilename.
#
def LoadModuleBinary(XmlFilename):
    ModuleBinary = ModuleBinaryFileClass()
    
    ModuleBinary.BinaryFile = XmlElementData(XmlFilename)
    
    XmlTag = "FileType"
    ModuleBinary.FileType = XmlElementAttribute(XmlFilename, XmlTag)
    
    SetCommon(ModuleBinary, XmlFilename)
    

## Load a list of Module Binaries.
#
# Read an input Module XML DOM object and return a list of Binaries
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Binaries             A list of Binaries loaded from XmlMsa.
#
def LoadModuleBinaries(XmlMsa):
    XmlTag = "ModuleSurfaceArea/BinaryFiles/Filename"
    return map(LoadModuleBinary, XmlList(XmlMsa, XmlTag))


## Load a list of Module Non Processed Files.
#
# Read an input Module XML DOM object and return a list of Non Processed Files
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel NonProcessedFiles    A list of Non Processed Files loaded from XmlMsa.
#
def LoadModuleNonProcessedFiles(XmlMsa):
    XmlTag = "ModuleSurfaceArea/NonProcessedFiles/Filename"
    return XmlElementList(XmlMsa, XmlTag)


## Load a new Module Package Dependency class object.
#
# Read an input XML PackageDependency DOM object and return an object of Package Dependency
# contained in the DOM object.
#
# @param  XmlPackage              A child XML DOM object in Module XML DOM.
#
# @retvel ModulePackageDependency A new Package Dependency object created by XmlPackage.
#
def LoadModulePackageDependency(XmlPackage):
    ModulePackageDependency = ModulePackageDependencyClass()

    XmlTag = "PackageGuid"
    PackageKey = XmlAttribute(XmlPackage, XmlTag)
    
    #
    #TODO: Add resolution for Package name, package Version
    #
    ModulePackageDependency.PackageGuid = PackageKey
    SetCommon(ModulePackageDependency, XmlPackage)

    return ModulePackageDependency
    

## Load a list of Module Package Dependencies.
#
# Read an input Module XML DOM object and return a list of Package Dependencies
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel PackageDependencies  A list of Package Dependencies loaded from XmlMsa.
#
def LoadModulePackageDependencies(XmlMsa):
    XmlTag = "ModuleSurfaceArea/PackageDependencies/Package"
    return map(LoadModulePackageDependency, XmlList(XmlMsa, XmlTag))


## Load a list of Module Protocols.
#
# Read an input Module XML DOM object and return a list of Protocols
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Protocols            A list of Protocols loaded from XmlMsa.
#
def LoadModuleProtocols(XmlMsa):
    XmlTag = "ModuleSurfaceArea/Protocols/Protocol"
    XmlProtocolList = XmlList(XmlMsa, XmlTag)

    XmlTag = "ModuleSurfaceArea/Protocols/ProtocolNotify"
    XmlProtocolList += XmlList(XmlMsa, XmlTag)

    return map(LoadGuidProtocolPpiCommon, XmlProtocolList)


## Load a list of Module Ppis.
#
# Read an input Module XML DOM object and return a list of Ppis
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Ppis                 A list of Ppis loaded from XmlMsa.
#
def LoadModulePpis(XmlMsa):
    XmlTag = "ModuleSurfaceArea/PPIs/Ppi"
    XmlPpiList = XmlList(XmlMsa, XmlTag)
    
    XmlTag = "ModuleSurfaceArea/PPIs/PpiNotify"
    XmlPpiList += XmlList(XmlMsa, XmlTag)
    
    return map(LoadGuidProtocolPpiCommon, XmlPpiList)


## Load a new Module Event class object.
#
# Read an input XML Event DOM object and return an object of Event
# contained in the DOM object.
#
# @param  XmlEvent             A child XML DOM object in Module XML DOM.
# @param  Type                 Specify the event type: SIGNAL_EVENT or CREATE_EVENT.
#
# @retvel ModuleEvent          A new Event object created by XmlEvent.
#
def LoadModuleEvent(XmlEvent, Type):
    ModuleEvent = ModuleEventClass()
    
    XmlTag = "EventTypes/EventType"
    ModuleEvent.CName = XmlElement(XmlEvent, XmlTag)
    
    XmlTag = "EventGuidCName"
    ModuleEvent.GuidCName = XmlAttribute(XmlEvent, XmlTag)
    
    ModuleEvent.Type = Type
    
    SetCommon(ModuleEvent, XmlEvent)
    
    return ModuleEvent
    

## Load a list of Module Events.
#
# Read an input Module XML DOM object and return a list of Events
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Events               A list of Events loaded from XmlMsa.
#
def LoadModuleEvents(XmlMsa):
    ModuleEvents = []
    
    XmlTag = "ModuleSurfaceArea/Events/CreateEvents/EventTypes"
    for XmlCreateEvent in XmlList(XmlMsa, XmlTag):
        ModuleEvent = LoadModuleEvent(XmlCreateEvent, "CREATE_EVENT")
        ModuleEvents.append(ModuleEvent)

    XmlTag = "ModuleSurfaceArea/Events/SignalEvents/EventTypes"
    for XmlCreateEvent in XmlList(XmlMsa, XmlTag):
        ModuleEvent = LoadModuleEvent(XmlCreateEvent, "SIGNAL_EVENT")
        ModuleEvents.append(ModuleEvent)

    return ModuleEvents


## Load a new Module Hob class object.
#
# Read an input XML Hob DOM object and return an object of Hob
# contained in the DOM object.
#
# @param  XmlHob               A child XML DOM object in Module XML DOM.
#
# @retvel ModuleHob            A new Hob object created by XmlHob.
#
def LoadModuleHob(XmlHob):
    ModuleHob = ModuleHobClass()
    
    XmlTag = "HobTypes/HobType"
    ModuleHob.Type = XmlElement(XmlHob, XmlTag)
    
    XmlTag = "HobGuidCName"
    ModuleHob.GuidCName = XmlAttribute(XmlHob, XmlTag)
    
    SetCommon(ModuleHob, XmlHob)
    
    return ModuleHob


## Load a list of Module Hobs.
#
# Read an input Module XML DOM object and return a list of Hobs
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Hobs                 A list of Hobs loaded from XmlMsa.
#
def LoadModuleHobs(XmlMsa):
    XmlTag = "ModuleSurfaceArea/Hobs/HobTypes"
    return map(LoadModuleHob, XmlList(XmlMsa, XmlTag))


## Load a new Module Variable class object.
#
# Read an input XML Variable DOM object and return an object of Variable
# contained in the DOM object.
#
# @param  XmlVariable          A child XML DOM object in Module XML DOM.
#
# @retvel ModuleVariable       A new Variable object created by XmlVariable.
#
def LoadModuleVariable(XmlVariable):
    ModuleVariable = ModuleVariableClass()
    
    XmlTag = "Variable/VariableName"
    HexWordArray = XmlElement(XmlVariable, XmlTag).split()
    try:
        ModuleVariable.Name = "".join([unichr(int(a, 16)) for a in HexWordArray])
    except:
        ModuleVariable.Name = ""
        
    XmlTag = "Variable/GuidC_Name"
    ModuleVariable.GuidCName = XmlElement(XmlVariable, XmlTag)
    
    SetCommon(ModuleVariable, XmlVariable)
    
    return ModuleVariable


## Load a list of Module Variables.
#
# Read an input Module XML DOM object and return a list of Variables
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Variables            A list of Variables loaded from XmlMsa.
#
def LoadModuleVariables(XmlMsa):
    XmlTag = "ModuleSurfaceArea/Variables/Variable"
    return map(LoadModuleVariable, XmlList(XmlMsa, XmlTag))


## Load a new Module Boot Mode class object.
#
# Read an input XML BootMode DOM object and return an object of Boot Mode
# contained in the DOM object.
#
# @param  XmlBootMode          A child XML DOM object in Module XML DOM.
#
# @retvel ModuleBootMode       A new Boot Mode object created by XmlBootMode.
#
def LoadModuleBootMode(XmlBootMode):
    ModuleBootMode = ModuleBootModeClass()
    
    XmlTag = "BootModeName"
    ModuleBootMode.Name = XmlAttribute(XmlBootMode, XmlTag)
    
    SetCommon(ModuleBootMode, XmlBootMode)
    
    return ModuleBootMode


## Load a list of Module Boot Modes.
#
# Read an input Module XML DOM object and return a list of Boot Modes
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel BootModes            A list of Boot Modes loaded from XmlMsa.
#
def LoadModuleBootModes(XmlMsa):
    XmlTag = "ModuleSurfaceArea/BootModes/BootMode"
    return map(LoadModuleBootMode, XmlList(XmlMsa, XmlTag))


## Load a new Module System Table class object.
#
# Read an input XML SystemTable DOM object and return an object of System Table
# contained in the DOM object.
#
# @param  XmlSystemTable       A child XML DOM object in Module XML DOM.
#
# @retvel ModuleSystemTable    A new System Table object created by XmlSystemTable.
#
def LoadModuleSystemTable(XmlSystemTable):
    ModuleSystemTable = ModuleSystemTableClass()
    
    XmlTag = "SystemTable/SystemTableCName"
    ModuleSystemTable.CName = XmlElement(XmlSystemTable, XmlTag)
    
    SetCommon(ModuleSystemTable, XmlSystemTable)
    
    return ModuleSystemTable
    

## Load a list of Module System Tables.
#
# Read an input Module XML DOM object and return a list of System Tables
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel SystemTables         A list of System Tables loaded from XmlMsa.
#
def LoadModuleSystemTables(XmlMsa):
    XmlTag = "ModuleSurfaceArea/SystemTables/SystemTableCNames"
    return map(LoadModuleSystemTable, XmlList(XmlMsa, XmlTag))


## Load a new Module Data Hub class object.
#
# Read an input XML DataHub DOM object and return an object of Data Hub
# contained in the DOM object.
#
# @param  XmlDataHub           A child XML DOM object in Module XML DOM.
#
# @retvel ModuleDataHub        A new Data Hub object created by XmlDataHub.
#
def LoadModuleDataHub(XmlDataHub):
    ModuleDataHub = ModuleDataHubClass()
    
    XmlTag = "DataHub/DataHubCName"
    ModuleDataHub.CName = XmlElement(XmlDataHub, "DataHubCName")
    
    SetCommon(ModuleDataHub, XmlDataHub)

    return ModuleDataHub


## Load a list of Module Data Hubs.
#
# Read an input Module XML DOM object and return a list of Data Hubs
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel DataHubs             A list of Data Hubs loaded from XmlMsa.
#
def LoadModuleDataHubs(XmlMsa):
    XmlTag = "ModuleSurfaceArea/DataHubs/DataHubRecord"
    return map(LoadModuleDataHub, XmlList(XmlMsa, XmlTag))


## Load a new Module Hii Package class object.
#
# Read an input XML HiiPackage DOM object and return an object of Hii Package
# contained in the DOM object.
#
# @param  XmlHiiPackage        A child XML DOM object in Module XML DOM.
#
# @retvel ModuleHiiPackage     A new Hii Package object created by XmlHiiPackage.
#
def LoadModuleHiiPackage(XmlHiiPackage):
    ModuleHiiPackage = ModuleHiiPackageClass()

    XmlTag = "HiiPackage/HiiPackageCName"
    ModuleHiiPackage.CName = XmlElement(XmlHiiPackage, "HiiCName")

    SetCommon(ModuleHiiPackage, XmlHiiPackage)

    return ModuleHiiPackage


## Load a list of Module Hii Packages.
#
# Read an input Module XML DOM object and return a list of Hii Packages
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel HiiPackages          A list of Hii Packages loaded from XmlMsa.
#
def LoadModuleHiiPackages(XmlMsa):
    XmlTag = "ModuleSurfaceArea/HiiPackages/HiiPackage"
    return map(LoadModuleHiiPackage, XmlList(XmlMsa, XmlTag))


## Load a list of Module Guids.
#
# Read an input Module XML DOM object and return a list of Guids
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel Guids                A list of Guids loaded from XmlMsa.
#
def LoadModuleGuids(XmlMsa):
    XmlTag = "ModuleSurfaceArea/Guids/GuidCNames"
    return map(LoadGuidProtocolPpiCommon, XmlList(XmlMsa, XmlTag))


## Load a list of Module Pcd Codes.
#
# Read an input Module XML DOM object and return a list of Pcd Codes
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel PcdCodes             A list of Pcd Codes loaded from XmlMsa.
#
def LoadModulePcdCodes(XmlMsa):
    XmlTag = "ModuleSurfaceArea/PcdCoded/PcdEntry"
    return map(LoadPcd, XmlList(XmlMsa, XmlTag))


## Load a list of Module Extern Images.
#
# Read an input Module XML DOM object and return a list of Extern Images
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel ExternImages         A list of Extern Images loaded from XmlMsa.
#
def LoadModuleExternImages(XmlMsa):
    ModuleExternImages = []
    
    XmlTag = "ModuleSurfaceArea/Externs/Extern"
    for XmlExtern in XmlList(XmlMsa, XmlTag):
        XmlTag = "Extern/ModuleEntryPoint"
        ModuleEntryPoint = XmlElement(XmlExtern, XmlTag)
        XmlTag = "Extern/ModuleUnloadImage"
        ModuleUnloadImage = XmlElement(XmlExtern, XmlTag)
        if ModuleEntryPoint == "" and ModuleUnloadImage == "":
            continue

        ModuleExtern = ModuleExternImageClass()
        ModuleExtern.ModuleEntryPoint = ModuleEntryPoint
        ModuleExtern.ModuleUnloadImage = ModuleUnloadImage
        ModuleExternImages.append(ModuleExtern)

    return ModuleExternImages


## Load a list of Module Extern Libraries.
#
# Read an input Module XML DOM object and return a list of Extern Libraries
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel ExternLibraries      A list of Extern Libraries loaded from XmlMsa.
#
def LoadModuleExternLibraries(XmlMsa):
    ModuleExternLibraries = []
    
    XmlTag = "ModuleSurfaceArea/Externs/Extern"
    for XmlExtern in XmlList(XmlMsa, XmlTag):
        XmlTag = "Extern/Constructor"
        Constructor = XmlElement(XmlExtern, XmlTag)
        XmlTag = "Extern/Destructor"
        Destructor = XmlElement(XmlExtern, XmlTag)
        if Constructor == "" and Destructor == "":
            continue
        
        ModuleExtern = ModuleExternLibraryClass()
        ModuleExtern.Constructor = Constructor
        ModuleExtern.Destructor = Destructor
        ModuleExternLibraries.append(ModuleExtern)

    return ModuleExternLibraries


## Load a list of Module Extern Drivers.
#
# Read an input Module XML DOM object and return a list of Extern Drivers
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel ExternDrivers        A list of Extern Drivers loaded from XmlMsa.
#
def LoadModuleExternDrivers(XmlMsa):
    ModuleExternDrivers = []
    
    XmlTag = "ModuleSurfaceArea/Externs/Extern"
    for XmlExtern in XmlList(XmlMsa, XmlTag):
        XmlTag = "Extern/DriverBinding"
        DriverBinding = XmlElement(XmlExtern, XmlTag)
        XmlTag = "Extern/ComponentName"
        ComponentName = XmlElement(XmlExtern, XmlTag)
        XmlTag = "Extern/DriverConfig"
        DriverConfig = XmlElement(XmlExtern, XmlTag)
        XmlTag = "Extern/DriverDiag"
        DriverDiag = XmlElement(XmlExtern, XmlTag)
        if DriverBinding == "":
            continue
        
        ModuleExtern = ModuleExternDriverClass()
        ModuleExtern.DriverBinding = DriverBinding
        ModuleExtern.ComponentName = ComponentName
        ModuleExtern.DriverConfig = DriverConfig
        ModuleExtern.DriverDiag = DriverDiag
        ModuleExternDrivers.append(ModuleExtern)

    return ModuleExternDrivers


## Load a list of Module Extern Call Backs.
#
# Read an input Module XML DOM object and return a list of Extern Call Backs
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel ExternCallBacks      A list of Extern Call Backs loaded from XmlMsa.
#
def LoadModuleExternCallBacks(XmlMsa):
    ModuleExternCallBacks = []
    
    XmlTag = "ModuleSurfaceArea/Externs/Extern"
    for XmlExtern in XmlList(XmlMsa, XmlTag):
        XmlTag = "Extern/SetVirtualAddressMapCallBack"
        SetVirtualAddressMap = XmlElement(XmlExtern, XmlTag)
        XmlTag = "Extern/ExitBootServicesCallBack"
        ExitBootServices = XmlElement(XmlExtern, XmlTag)
        if SetVirtualAddressMap == "" and ExitBootServices == "":
            continue
        
        ModuleExtern = ModuleExternCallBackClass()
        ModuleExtern.ExitBootServicesCallBack = ExitBootServices
        ModuleExtern.SetVirtualAddressMapCallBack = SetVirtualAddressMap
        ModuleExternCallBacks.append(ModuleExtern)

    return ModuleExternCallBacks


## Load a list of Module Build Options.
#
# Read an input Module XML DOM object and return a list of Build Options
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel BuildOptions         A list of Build Options loaded from XmlMsa.
#
def LoadModuleBuildOptions(XmlMsa):
    XmlTag = "ModuleSurfaceArea/ModuleBuildOptions/Options/Option"
    return map(LoadBuildOption, XmlList(XmlMsa, XmlTag))


## Load a list of Module User Extensions.
#
# Read an input Module XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlMsa               An XML DOM object read from MSA file.
#
# @retvel UserExtensions       A list of User Extensions loaded from XmlMsa.
#
def LoadModuleUserExtensions(XmlMsa):
    XmlTag = "ModuleSurfaceArea/UserExtensions"
    return map(LoadUserExtensions, XmlList(XmlMsa, XmlTag))

## Load a new Module class object.
#
# Read an input MSA File and return a new Module class Object.
#
# @param  MsaFileName          An XML DOM object read from MSA file.
#
# @retvel Module               A new Module class object loaded from MSA File.
#
def LoadMsa(MsaFileName):
    XmlMsa = XmlParseFile(MsaFileName)
    EdkLogger.verbose("Load MSA File: %s" % MsaFileName)
    
    Module = ModuleClass()
    Module.Header = LoadModuleHeader(XmlMsa, MsaFileName)
    Module.LibraryClasses = LoadModuleLibraryClasses(XmlMsa)
    Module.Sources = LoadModuleSources(XmlMsa)
    Module.BinaryFiles = LoadModuleBinaries(XmlMsa)
    Module.NonProcessedFiles = LoadModuleNonProcessedFiles(XmlMsa)
    Module.PackageDependencies = LoadModulePackageDependencies(XmlMsa)
    Module.Protocols = LoadModuleProtocols(XmlMsa)
    Module.Ppis = LoadModulePpis(XmlMsa)
    Module.Events = LoadModuleEvents(XmlMsa)
    Module.Hobs = LoadModuleHobs(XmlMsa)
    Module.Variables = LoadModuleVariables(XmlMsa)
    Module.BootModes = LoadModuleBootModes(XmlMsa)
    Module.SystemTables = LoadModuleSystemTables(XmlMsa)
    Module.DataHubs = LoadModuleDataHubs(XmlMsa)
    Module.HiiPackages = LoadModuleHiiPackages(XmlMsa)
    Module.Guids = LoadModuleGuids(XmlMsa)
    Module.PcdCodes = LoadModulePcdCodes(XmlMsa)
    Module.ExternImages = LoadModuleExternImages(XmlMsa)
    Module.ExternLibraries = LoadModuleExternLibraries(XmlMsa)
    Module.ExternDrivers = LoadModuleExternDrivers(XmlMsa)
    Module.ExternCallBacks = LoadModuleExternCallBacks(XmlMsa)
    Module.BuildOptions = LoadModuleBuildOptions(XmlMsa)
    Module.UserExtensions = LoadModuleUserExtensions(XmlMsa)
    
    return Module


# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    pass