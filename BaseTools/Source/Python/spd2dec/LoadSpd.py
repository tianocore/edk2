## @file
# Open an SPD file and load all its contents to a PackageClass object.
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
from Common.XmlRoutines import *
from Common.MigrationUtilities import *
from CommonDataClass.PackageClass import *


## Load a list of Package Cloned Records.
#
# Read an input Package XML DOM object and return a list of Cloned Records
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel ClonedRecords        A list of Cloned Records loaded from XmlSpd.
#
def LoadPackageClonedRecords(XmlSpd):
    XmlTag = "PackageSurfaceArea/PackageDefinitions/ClonedFrom/Cloned"
    return map(LoadClonedRecord, XmlList(XmlSpd, XmlTag))


## Load Package Header.
#
# Read an input Package XML DOM object and return Package Header class object
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
# @param  SpdFileName          The file path of SPD File.
#
# @retvel PackageHeader        A new Package Header object loaded from XmlSpd.
#
def LoadPackageHeader(XmlSpd, SpdFileName):
    PackageHeader = PackageHeaderClass()
    
    XmlTag = "PackageSurfaceArea/SpdHeader"
    SpdHeader = XmlNode(XmlSpd, XmlTag)
    
    SetIdentification(PackageHeader, SpdHeader, "PackageName", SpdFileName)
    SetCommonHeader(PackageHeader, SpdHeader)
    
    XmlTag = "PackageSurfaceArea/PackageDefinitions/ReadOnly"
    if XmlElement(XmlSpd, XmlTag).lower() == "true":
        PackageHeader.ReadOnly = True

    XmlTag = "PackageSurfaceArea/PackageDefinitions/RePackage"
    if XmlElement(XmlSpd, XmlTag).lower() == "true":
        PackageHeader.RePackage = True

    PackageHeader.ClonedFrom = LoadPackageClonedRecords(XmlSpd)
    
    return PackageHeader


## Load a list of Package Library Classes.
#
# Read an input Package XML DOM object and return a list of Library Classes
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel LibraryClasses       A list of Library Classes loaded from XmlSpd.
#
def LoadPackageLibraryClasses(XmlSpd):
    XmlTag = "PackageSurfaceArea/LibraryClassDeclarations/LibraryClass"
    return map(LoadLibraryClass, XmlList(XmlSpd, XmlTag))


## Load a new Package Industry Std Header class object.
#
# Read an input XML IndustryStdHeader DOM object and return an object of
# Industry Std Header contained in the DOM object.
#
# @param  XmlIndustryStdHeader     A child XML DOM object in Package XML DOM.
#
# @retvel PackageIndustryStdHeader A new Industry Std Header object created by XmlIndustryStdHeader.
#
def LoadPackageIndustryStdHeader(XmlIndustryStdHeader):
    PackageIndustryStdHeader = PackageIndustryStdHeaderClass()
    
    XmlTag = "Name"
    Name = XmlAttribute(XmlIndustryStdHeader, XmlTag)
    PackageIndustryStdHeader.Name = Name
    
    XmlTag = "IndustryStdHeader/IncludeHeader"
    IncludeHeader = XmlElement(XmlIndustryStdHeader, XmlTag)
    PackageIndustryStdHeader.IncludeHeader = IncludeHeader
    
    SetCommon(PackageIndustryStdHeader, XmlIndustryStdHeader)
    
    return PackageIndustryStdHeader


## Load a list of Package Industry Std Headers.
#
# Read an input Package XML DOM object and return a list of Industry Std Headers
# contained in the DOM object.
#
# @param  XmlSpd             An XML DOM object read from SPD file.
#
# @retvel IndustryStdHeaders A list of Industry Std Headers loaded from XmlSpd.
#
def LoadPackageIndustryStdHeaders(XmlSpd):
    XmlTag = "PackageSurfaceArea/IndustryStdIncludes/IndustryStdHeader"
    return map(LoadPackageIndustryStdHeader, XmlList(XmlSpd, XmlTag))


## Load a list of Package Module Files.
#
# Read an input Package XML DOM object and return a list of Module Files
# contained in the DOM object.
#
# @param  XmlSpd             An XML DOM object read from SPD file.
#
# @retvel ModuleFiles        A list of Module Files loaded from XmlSpd.
#
def LoadPackageModuleFiles(XmlSpd):
    XmlTag = "PackageSurfaceArea/MsaFiles/Filename"
    return XmlElementList(XmlSpd, XmlTag)


## Load a new Package Include Pkg Header class object.
#
# Read an input XML IncludePkgHeader DOM object and return an object of Include
# Package Header contained in the DOM object.
#
# @param  XmlPackageIncludeHeader A child XML DOM object in Package XML DOM.
#
# @retvel PackageIncludePkgHeader A new Include Pkg Header object created by
#                                 XmlPackageIncludeHeader.
#
def LoadPackageIncludePkgHeader(XmlPackageIncludeHeader):
    PackageIncludeHeader = PackageIncludePkgHeaderClass()
    
    IncludeHeader = XmlElementData(XmlPackageIncludeHeader)
    PackageIncludeHeader.IncludeHeader = IncludeHeader
    
    XmlTag = "ModuleType"
    ModuleTypes = XmlAttribute(XmlPackageIncludeHeader, XmlTag)
    PackageIncludeHeader.ModuleType = ModuleTypes.split()
    
    return PackageIncludeHeader


## Load a list of Package Include Pkg Headers.
#
# Read an input Package XML DOM object and return a list of Include Pkg Headers
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel IncludePkgHeaders    A list of Include Pkg Headers loaded from XmlSpd.
#
def LoadPackageIncludePkgHeaders(XmlSpd):
    XmlTag = "PackageSurfaceArea/PackageHeaders/IncludePkgHeader"
    return map(LoadPackageIncludePkgHeader, XmlList(XmlSpd, XmlTag))


## Load a list of Package Guid Declarations.
#
# Read an input Package XML DOM object and return a list of Guid Declarations
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel GuidDeclarations     A list of Guid Declarations loaded from XmlSpd.
#
def LoadPackageGuidDeclarations(XmlSpd):
    XmlTag = "PackageSurfaceArea/GuidDeclarations/Entry"
    return map(LoadGuidProtocolPpiCommon, XmlList(XmlSpd, XmlTag))


## Load a list of Package Protocol Declarations.
#
# Read an input Package XML DOM object and return a list of Protocol Declarations
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel ProtocolDeclarations A list of Protocol Declarations loaded from XmlSpd.
#
def LoadPackageProtocolDeclarations(XmlSpd):
    XmlTag = "PackageSurfaceArea/ProtocolDeclarations/Entry"
    return map(LoadGuidProtocolPpiCommon, XmlList(XmlSpd, XmlTag))


## Load a list of Package Ppi Declarations.
#
# Read an input Package XML DOM object and return a list of Ppi Declarations
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel PpiDeclarations      A list of Ppi Declarations loaded from XmlSpd.
#
def LoadPackagePpiDeclarations(XmlSpd):
    XmlTag = "PackageSurfaceArea/PpiDeclarations/Entry"
    return map(LoadGuidProtocolPpiCommon, XmlList(XmlSpd, XmlTag))


## Load a list of Package Pcd Declarations.
#
# Read an input Package XML DOM object and return a list of Pcd Declarations
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel PcdDeclarations      A list of Pcd Declarations loaded from XmlSpd.
#
def LoadPackagePcdDeclarations(XmlSpd):
    XmlTag = "PackageSurfaceArea/PcdDeclarations/PcdEntry"
    return map(LoadPcd, XmlList(XmlSpd, XmlTag))


## Load a list of Package User Extensions.
#
# Read an input Package XML DOM object and return a list of User Extensions
# contained in the DOM object.
#
# @param  XmlSpd               An XML DOM object read from SPD file.
#
# @retvel UserExtensions       A list of User Extensions loaded from XmlSpd.
#
def LoadPackageUserExtensions(XmlSpd):
    XmlTag = "PackageSurfaceArea/UserExtensions"
    return map(LoadUserExtensions, XmlList(XmlSpd, XmlTag))


## Load a new Package class object.
#
# Read an input SPD File and return a new Package class Object.
#
# @param  SpdFileName          An XML DOM object read from SPD file.
#
# @retvel Package              A new Module class object loaded from SPD File.
#
def LoadSpd(SpdFileName):
    XmlSpd = XmlParseFile(SpdFileName)
    EdkLogger.verbose("Xml Object loaded for file %s" % SpdFileName)

    Package = PackageClass()
    Package.Header = LoadPackageHeader(XmlSpd, SpdFileName)
    Package.LibraryClassDeclarations = LoadPackageLibraryClasses(XmlSpd)
    Package.IndustryStdHeaders = LoadPackageIndustryStdHeaders(XmlSpd)
    Package.ModuleFiles = LoadPackageModuleFiles(XmlSpd)
    Package.PackageIncludePkgHeaders = LoadPackageIncludePkgHeaders(XmlSpd)
    Package.GuidDeclarations = LoadPackageGuidDeclarations(XmlSpd)
    Package.ProtocolDeclarations = LoadPackageProtocolDeclarations(XmlSpd)
    Package.PpiDeclarations = LoadPackagePpiDeclarations(XmlSpd)
    Package.PcdDeclarations = LoadPackagePcdDeclarations(XmlSpd)
    Package.UserExtensions = LoadPackageUserExtensions(XmlSpd)
    
    return Package


# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    pass
