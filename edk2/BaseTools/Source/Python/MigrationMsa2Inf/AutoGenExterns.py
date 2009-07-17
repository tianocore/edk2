#!/usr/bin/env python
#
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
# Import Modules
#
import re, os, glob
from Common.XmlRoutines import  *

#"ModuleType"=>(PackageGuid, headerFileName) List
HeaderFiles = {}
GuidList = []
GuidMap = {}
HeaderFileContents = {}
gTest = {}
GuidMacro2CName = {}
GuidAliasList = []

def collectIncludeFolder(pkgDirName, guidType, pkgName):
    includeFolder = os.path.join(pkgDirName, "Include", guidType)
    if os.path.exists(includeFolder) and os.path.isdir(includeFolder):
        for headerFileName in os.listdir(includeFolder):
            if headerFileName[-2:] == ".h":
                headerFile = open(os.path.join(includeFolder, headerFileName))
                HeaderFileContents[(guidType, headerFileName, pkgName)] = headerFile.read()
                headerFile.close()

GuidMacroReg = re.compile(r"\b(?!EFI_GUID\b)[A-Z0-9_]+_GUID\b")
GuidCNameReg = re.compile(r"\bg\w+Guid\b")
GuidAliasReg = re.compile(r"#define\s+([A-Z0-9_]+_GUID)\s+([A-Z0-9_]+_GUID)\b")

def collectPackageInfo(spdFileName):
    pkgDirName = os.path.dirname(spdFileName)

    spd = XmlParseFile(spdFileName)

    pkgName = XmlElement(spd, "/PackageSurfaceArea/SpdHeader/PackageName")
    pkgGuid = XmlElement(spd, "/PackageSurfaceArea/SpdHeader/GuidValue")

    
    for IncludePkgHeader in XmlList(spd, "/PackageSurfaceArea/PackageHeaders/IncludePkgHeader"):
        moduleType = XmlAttribute(IncludePkgHeader, "ModuleType")
        headerFilePath = XmlElementData(IncludePkgHeader)
        headerFilePath = re.sub("Include/", "", headerFilePath, 1)

        headerTuple = HeaderFiles.get(moduleType, [])
        headerTuple.append((pkgGuid, headerFilePath))
        HeaderFiles[moduleType] = headerTuple
        
    guidTypes = ["Guid", "Protocol", "Ppi"]

    for guidType in guidTypes:
        for guidEntry in XmlList(spd, "/PackageSurfaceArea/" + guidType + "Declarations/Entry"):
            guidCName = XmlElement(guidEntry, "Entry/C_Name")
            GuidList.append(guidCName)
                      
        collectIncludeFolder(pkgDirName, guidType, pkgName)   

    for DecFile in glob.glob(os.path.join(pkgDirName, "*.dec")):
        fileContents = open(DecFile).read()
        for GuidCNameMatch in GuidCNameReg.finditer(fileContents):
            GuidCName = GuidCNameMatch.group(0)
            if GuidCName not in GuidList:
                GuidList.append(GuidCName)

def AddGuidMacro2GuidCName(GuidMacros, GuidCNames):
   for GuidMacro in GuidMacros:
       GuessGuidCName = "g" + GuidMacro.lower().title().replace("_", "")
       if GuessGuidCName in GuidCNames:
           GuidMacro2CName[GuidMacro] = GuessGuidCName
       elif len(GuidCNames) == 1:
           GuidMacro2CName[GuidMacro] = GuidCNames[0]
       else:
           for GuidCName in GuidCNames:
               if GuidCName.lower() == GuessGuidCName.lower():
                   GuidMacro2CName[GuidMacro] = GuidCName
                   break
           else:
               pass
               #print "No matching GuidMacro %s" % GuidMacro
               

def TranslateGuid(GuidMacroMatch):
    GuidMacro = GuidMacroMatch.group(0)
    return GuidMacro2CName.get(GuidMacro, GuidMacro)

DepexReg = re.compile(r"DEPENDENCY_START(.*?)DEPENDENCY_END", re.DOTALL)

def TranslateDpxSection(fileContents):
    DepexMatch = DepexReg.search(fileContents)
    if not DepexMatch:
        return "", []
    
    fileContents = DepexMatch.group(1)
    fileContents = re.sub(r"\s+", " ", fileContents).strip()
    fileContents = GuidMacroReg.sub(TranslateGuid, fileContents)
    return fileContents, GuidMacroReg.findall(fileContents)

def InitializeAutoGen(workspace, db):
    
    
    for spdFile in XmlList(db, "/FrameworkDatabase/PackageList/Filename"):
        spdFileName = XmlElementData(spdFile)
        collectPackageInfo(os.path.join(workspace, spdFileName))


    BlockCommentReg = re.compile(r"/\*.*?\*/", re.DOTALL)
    LineCommentReg = re.compile(r"//.*")
    GuidReg = re.compile(r"\b(" + '|'.join(GuidList) + r")\b")

    for headerFile in HeaderFileContents:
        Contents = HeaderFileContents[headerFile]
        Contents = BlockCommentReg.sub("", Contents)
        Contents = LineCommentReg.sub("", Contents)
        
        FoundGuids = GuidReg.findall(Contents)
        for FoundGuid in FoundGuids:
            GuidMap[FoundGuid] = "%s/%s" % (headerFile[0], headerFile[1])
            #print "%-40s %s/%s" % (FoundGuid, headerFile[0], headerFile[1])

        GuidMacros = GuidMacroReg.findall(Contents)
        GuidCNames = GuidCNameReg.findall(Contents)
        
        for GuidAliasMatch in  GuidAliasReg.finditer(Contents):
            Name1, Name2 = GuidAliasMatch.group(1), GuidAliasMatch.group(2)
            GuidAliasList.append((Name1, Name2))

        AddGuidMacro2GuidCName(GuidMacros, GuidCNames)

def AddSystemIncludeStatement(moduleType, PackageList):
    IncludeStatement = "\n"

    headerList = HeaderFiles.get(moduleType, [])

    for pkgGuid in PackageList:
        
        for pkgTuple in headerList:
            if pkgTuple[0] == pkgGuid:
                IncludeStatement += "#include <%s>\n" % pkgTuple[1]

    return IncludeStatement
        
    
def AddLibraryClassStatement(LibraryClassList):
    IncludeStatement = "\n"
    for LibraryClass in LibraryClassList:
        IncludeStatement += "#include <Library/%s.h>\n" % LibraryClass

    return IncludeStatement

def AddGuidStatement(GuidList):
    IncludeStatement = "\n"
    GuidIncludeSet = {}
    for Guid in GuidList:
        if Guid in GuidMap:
            GuidIncludeSet[GuidMap[Guid]] = 1
        else:
            print "GUID CName: %s cannot be found in any public header file" % Guid

    for GuidInclude in GuidIncludeSet:
        IncludeStatement += "#include <%s>\n" % GuidInclude

    return IncludeStatement

DriverBindingMap = {
    "gEfiDriverBindingProtocolGuid" : "EFI_DRIVER_BINDING_PROTOCOL",
    "gEfiComponentNameProtocolGuid" : "EFI_COMPONENT_NAME_PROTOCOL",
    "gEfiDriverConfigurationProtocolGuid" : "EFI_DRIVER_CONFIGURATION_PROTOCOL",
    "gEfiDriverDiagnosticProtocolGuid" : "EFI_DRIVER_CONFIGURATION_PROTOCOL"
    }

def AddDriverBindingProtocolStatement(AutoGenDriverModel):
    InstallStatement   = "\n"
    DBindingHandle     = "ImageHandle"
    GlobalDeclaration  = "\n"
    
    
    for DriverModelItem in AutoGenDriverModel:
        
        if DriverModelItem[1] == "NULL" and DriverModelItem[2] == "NULL" and DriverModelItem[3] == "NULL":
            InstallStatement += "  Status = EfiLibInstallDriverBinding (\n"
            InstallStatement += "             ImageHandle,\n"
            InstallStatement += "             SystemTable,\n"
            InstallStatement += "             %s,\n" % DriverModelItem[0]
            InstallStatement += "             %s\n" % DBindingHandle
            InstallStatement += "             );\n"
        else:
            InstallStatement += "  Status = EfiLibInstallAllDriverProtocols (\n"
            InstallStatement += "             ImageHandle,\n"
            InstallStatement += "             SystemTable,\n"
            InstallStatement += "             %s,\n" % DriverModelItem[0]
            InstallStatement += "             %s,\n" % DBindingHandle
            InstallStatement += "             %s,\n" % DriverModelItem[1]
            InstallStatement += "             %s,\n" % DriverModelItem[2]
            InstallStatement += "             %s\n" % DriverModelItem[3]
            InstallStatement += "             );\n"

        InstallStatement += "  ASSERT_EFI_ERROR (Status);\n\n"
            
        GlobalDeclaration += "extern EFI_DRIVER_BINDING_PROTOCOL %s;\n" % DriverModelItem[0][1:]
        if (DriverModelItem[1] != "NULL"):
            GlobalDeclaration += "extern EFI_COMPONENT_NAME_PROTOCOL %s;\n" % DriverModelItem[1][1:]
        if (DriverModelItem[2] != "NULL"):
            GlobalDeclaration += "extern EFI_DRIVER_CONFIGURATION_PROTOCOL %s;\n" % DriverModelItem[2][1:]
        if (DriverModelItem[3] != "NULL"):
            GlobalDeclaration += "extern EFI_DRIVER_CONFIGURATION_PROTOCOL %s;\n" % DriverModelItem[3][1:]

        DBindingHandle = "NULL"
         
    return (InstallStatement, "", "", GlobalDeclaration)

EventDeclarationTemplate = """
//
// Declaration for callback Event.
//
VOID
EFIAPI
%s (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );
"""

def AddBootServiceEventStatement(EventList):
    FinalEvent = ""
    if len(EventList) > 1:
        
        print "Current prototype does not support multi boot service event"
    else:
        FinalEvent       = EventList[0]
    
    CreateStatement  = "\n"
    CreateStatement += "  Status = gBS->CreateEvent (\n"
    CreateStatement += "                  EVT_SIGNAL_EXIT_BOOT_SERVICES,\n"
    CreateStatement += "                  EFI_TPL_NOTIFY,\n"
    CreateStatement += "                  " + FinalEvent + ",\n"
    CreateStatement += "                  NULL,\n"
    CreateStatement += "                  &mExitBootServicesEvent\n"
    CreateStatement += "                  );\n"
    CreateStatement += "  ASSERT_EFI_ERROR (Status);\n"

    GlobalDefinition  = "\n"
    GlobalDefinition += "STATIC EFI_EVENT mExitBootServicesEvent = NULL;\n"

    GlobalDeclaration = EventDeclarationTemplate % FinalEvent
    
    DestroyStatement  = "\n"
    DestroyStatement += "  Status = gBS->CloseEvent (mExitBootServicesEvent);\n"
    DestroyStatement += "  ASSERT_EFI_ERROR (Status);\n"
    return (CreateStatement, "", GlobalDefinition, GlobalDeclaration)

def AddVirtualAddressEventStatement(EventList):
    FinalEvent = ""
    if len(EventList) > 1:
        print "Current prototype does not support multi virtual address change event"
    else:
        FinalEvent       = EventList[0]
    
    CreateStatement  = "\n"

    CreateStatement += "  Status = gBS->CreateEvent (\n"
    CreateStatement += "                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,\n"
    CreateStatement += "                  TPL_NOTIFY,\n"
    CreateStatement += "                  " + FinalEvent + ",\n"
    CreateStatement += "                  NULL,\n"
    CreateStatement += "                  &mVirtualAddressChangedEvent\n"
    CreateStatement += "                  );\n"
    CreateStatement += "  ASSERT_EFI_ERROR (Status);\n"

    GlobalDefinition  = "\n"
    GlobalDefinition += "STATIC EFI_EVENT mVirtualAddressChangedEvent = NULL;\n"
    
    GlobalDeclaration = EventDeclarationTemplate % FinalEvent

    DestroyStatement  = "\n"
    DestroyStatement += "  Status = gBS->CloseEvent (mVirtualAddressChangedEvent);\n"
    DestroyStatement += "  ASSERT_EFI_ERROR (Status);\n"
    
    return (CreateStatement, "", GlobalDefinition, GlobalDeclaration)
    

EntryPointDeclarationTemplate = """
//
// Declaration for original Entry Point. 
//
EFI_STATUS
EFIAPI
%s (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );
"""
    
EntryPointHeader = r"""
/**
  The user Entry Point for module %s. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
"""
def AddNewEntryPointContentsStatement (moduleName, EntryPoint, InstallStatement = ""):
    if EntryPoint != "Initialize%s" % moduleName:
        NewEntryPoint = "Initialize%s" % moduleName
    else:
        NewEntryPoint = "NewInitialize%s" % moduleName

    EntryPointContents  = EntryPointHeader % moduleName
    EntryPointContents += "EFI_STATUS\n"
    EntryPointContents += "EFIAPI\n"
    EntryPointContents += NewEntryPoint + "(\n"
    EntryPointContents += "  IN EFI_HANDLE           ImageHandle,\n"
    EntryPointContents += "  IN EFI_SYSTEM_TABLE     *SystemTable\n"
    EntryPointContents += "  )\n"
    EntryPointContents += "{\n"
    EntryPointContents += "  EFI_STATUS              Status;\n"
    EntryPointContents += InstallStatement + "\n"
    GlobalDeclaration   = ""

    if EntryPoint != "":
        EntryPointContents += "  //\n  // Call the original Entry Point\n  //\n"
        EntryPointContents += "  Status = %s (ImageHandle, SystemTable);\n\n" % EntryPoint
        GlobalDeclaration  += EntryPointDeclarationTemplate % EntryPoint

    EntryPointContents += "  return Status;\n"
    EntryPointContents += "}\n"

    return (NewEntryPoint, EntryPointContents, GlobalDeclaration)

reFileHeader = re.compile(r"^\s*/\*.*?\*/\s*", re.DOTALL)
reNext       = re.compile(r"#ifndef\s*(\w+)\s*#define\s*\1\s*")

def AddCommonInclusionStatement(fileContents, includeStatement):
    if includeStatement in fileContents:
        return fileContents

    insertPos       = 0
    matchFileHeader = reFileHeader.search(fileContents)
    if matchFileHeader:
        insertPos = matchFileHeader.end()

    matchFileHeader = reNext.search(fileContents, insertPos)
    if matchFileHeader:
        insertPos = matchFileHeader.end()
                          
    includeStatement = "\n%s\n\n" % includeStatement 
    fileContents = fileContents[0:insertPos] + includeStatement + fileContents[insertPos:] 
    return fileContents 

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

    pass
    
