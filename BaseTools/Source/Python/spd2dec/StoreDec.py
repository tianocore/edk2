## @file
# Store a Package class object to a DEC file.
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
from Common.MigrationUtilities import *
from LoadSpd import LoadSpd
from CommonDataClass.PackageClass import *


## Store Defines section.
#
# Write [Defines] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile            The output DEC file to store the Defines section.
# @param  Package            An input Package class object.
#
def StorePackageDefinesSection(DecFile, Package):
    DefinesTupleList = []
    DefinesTupleList.append(("DEC_VERSION", Package.Header.DecSpecification))
    DefinesTupleList.append(("PACKAGE_NAME", Package.Header.Name))
    DefinesTupleList.append(("PACKAGE_GUID", Package.Header.Guid))
    
    StoreDefinesSection(DecFile, DefinesTupleList)


## Return a Package Include Class Item.
#
# Read the input Include class object and return one Include Class Item.
#
# @param  Include            An input Include class object.
#
# @retval IncludeClassItem   A Package Include Class Item.
#
def GetPackageIncludeClassItem(Include):
    return Include.FilePath


## Store Includes section.
#
# Write [Includes] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile            The output DEC file to store the Includes section.
# @param  Package            An input Package class object.
#
def StorePackageIncludesSection(DecFile, Package):
    Includes = Package.Includes
    Section = GetSection("Includes", GetPackageIncludeClassItem, Includes)
    StoreTextFile(DecFile, Section)
    

## Return a Package Library Class Item.
#
# Read the input LibraryClass class object and return one Library Class Item.
#
# @param  LibraryClass       An input LibraryClass class object.
#
# @retval LibraryClassItem   A Package Library Class Item.
#
def GetPackageLibraryClassItem(LibraryClass):
    return "|".join((LibraryClass.LibraryClass, LibraryClass.IncludeHeader))


## Store Library Classes section.
#
# Write [LibraryClasses] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile            The output DEC file to store the Library Classes
#                            section.
# @param  Package            An input Package class object.
#
def StorePackageLibraryClassesSection(DecFile, Package):
    LibraryClasses = Package.LibraryClassDeclarations
    Section = GetSection("LibraryClasses", GetPackageLibraryClassItem, LibraryClasses)
    StoreTextFile(DecFile, Section)


## Return a Package Guid Declaration Item.
#
# Read the input Guid class object and return one line of Guid Declaration Item.
#
# @param  Guid                An input Guid class object.
#
# @retval GuidDeclarationItem A Package Guid Declaration Item.
#
def GetPackageGuidDeclarationItem(Guid):
    GuidCName = Guid.CName
    GuidValue = Guid.Guid.replace("-", "")
    GuidValueList = [GuidValue[0:8]]
    GuidValueList += [GuidValue[i : i + 4] for i in range(8, 16, 4)]
    GuidValueList += [GuidValue[i : i + 2] for i in range(16, 32, 2)]

    GuidCFormat = "{0x%s" + ", 0x%s" * 2 + ", {0x%s" + ", 0x%s" * 7 + "}}"
    GuidCValue = GuidCFormat % tuple(GuidValueList)
    return "%-30s = %s" % (GuidCName, GuidCValue)


## Store Protocols section.
#
# Write [Protocols] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile            The output DEC file to store the Protocols section.
# @param  Package            An input Package class object.
#
def StorePackageProtocolsSection(DecFile, Package):
    Protocols = Package.ProtocolDeclarations
    Section = GetSection("Protocols", GetPackageGuidDeclarationItem, Protocols)
    StoreTextFile(DecFile, Section)
    

## Store Ppis section.
#
# Write [Ppis] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile            The output DEC file to store the Ppis section.
# @param  Package            An input Package class object.
#
def StorePackagePpisSection(DecFile, Package):
    Ppis = Package.PpiDeclarations
    Section = GetSection("Ppis", GetPackageGuidDeclarationItem, Ppis)
    StoreTextFile(DecFile, Section)


## Store Guids section.
#
# Write [Guids] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile            The output DEC file to store the Guids section.
# @param  Package            An input Package class object.
#
def StorePackageGuidsSection(DecFile, Package):
    Guids = Package.GuidDeclarations
    Section = GetSection("Guids", GetPackageGuidDeclarationItem, Guids)
    StoreTextFile(DecFile, Section)


## Return a Package Pcd Item.
#
# Read the input Pcd class object and return one line of Pcd Item.
#
# @param  Pcd                An input Pcd class object.
#
# @retval PcdItem            A Package Pcd Item.
#
def GetPackagePcdItem(Pcd):
    PcdPair = "%s.%s" % (Pcd.TokenSpaceGuidCName, Pcd.CName)
    DatumType = Pcd.DatumType
    DefaultValue = Pcd.DefaultValue
    Token = Pcd.Token
    PcdList = [PcdPair, DefaultValue, DatumType, Token]
    return "|".join(PcdList)


## DEC Pcd Section Name dictionary indexed by PCD Item Type.
mDecPcdSectionNameDict = {
    "FEATURE_FLAG" : "PcdsFeatureFlag",
    "FIXED_AT_BUILD" : "PcdsFixedAtBuild",
    "PATCHABLE_IN_MODULE" : "PcdsPatchableInModule",
    "DYNAMIC" : "PcdsDynamic",
    "DYNAMIC_EX" : "PcdsDynamicEx"
    }

## Store Pcds section.
#
# Write [Pcds*] section to the DecFile based on Package class object.
# Different CPU architectures are specified in the subsection if possible.
#
# @param  DecFile              The output DEC file to store the Pcds section.
# @param  Package              An input Package class object.
#
def StorePackagePcdsSection(DecFile, Package):
    PcdsDict = {}
    for Pcd in Package.PcdDeclarations:
        for PcdItemType in Pcd.ValidUsage:
            PcdSectionName = mDecPcdSectionNameDict.get(PcdItemType)
            if PcdSectionName:
                PcdsDict.setdefault(PcdSectionName, []).append(Pcd)
            else:
                EdkLogger.info("Unknown Pcd Item Type: %s" % PcdItemType)

    Section = ""
    for PcdSectionName in PcdsDict:
        Pcds = PcdsDict[PcdSectionName]
        Section += GetSection(PcdSectionName, GetPackagePcdItem, Pcds)

    StoreTextFile(DecFile, Section)


## Store User Extensions section.
#
# Write [UserExtensions] section to the DecFile based on Package class object.
#
# @param  DecFile              The output DEC file to store the User Extensions section.
# @param  Package              An input Package class object.
#
def StorePackageUserExtensionsSection(DecFile, Package):
    Section = "".join(map(GetUserExtensions, Package.UserExtensions))
    StoreTextFile(DecFile, Section)


## Store a Package class object to a new DEC file.
#
# Read an input Package class object and ave the contents to a new DEC file.
#
# @param  DecFileName          The output DEC file.
# @param  Package              An input Package class object.
#
def StoreDec(DecFileName, Package):
    DecFile = open(DecFileName, "w+")
    EdkLogger.info("Save file to %s" % DecFileName)
    
    StoreHeader(DecFile, Package.Header)
    StorePackageDefinesSection(DecFile, Package)
    StorePackageIncludesSection(DecFile, Package)
    StorePackageLibraryClassesSection(DecFile, Package)
    StorePackageProtocolsSection(DecFile, Package)
    StorePackagePpisSection(DecFile, Package)
    StorePackageGuidsSection(DecFile, Package)
    StorePackagePcdsSection(DecFile, Package)
    StorePackageUserExtensionsSection(DecFile, Package)
    
    DecFile.close()

    
# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    pass
    