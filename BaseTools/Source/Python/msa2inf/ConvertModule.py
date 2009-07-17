## @file
# Convert an MSA Module class object ot an INF Module class object by filling
# several info required by INF file.
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
from StoreInf import StoreInf
from Common.MigrationUtilities import *
from EdkIIWorkspaceGuidsInfo import gEdkIIWorkspaceGuidsInfo

#The default INF version number tool generates.
gInfVersion = "0x00010005"

## Add required version information.
#
# Add the default INF version, EFI specificiation version and EDK release
# version to Module class object.
#
# @param  Module              An input Module class object.
#
def AddModuleMiscVersion(Module):
    Version = gInfVersion
    Module.Header.InfVersion = Version

    Version = Module.Header.Specification.get("EFI_SPECIFICATION_VERSION", "")
    Module.Header.EfiSpecificationVersion = Version
    
    Version = Module.Header.Specification.get("EDK_RELEASE_VERSION", "")
    Module.Header.EdkReleaseVersion = Version


## Add Module produced library class.
#
# Add the produced library class from library class list whose usage type is
# always produced.
#
# @param  Module              An input Module class object.
#
def AddModuleProducedLibraryClass(Module):
    for LibraryClass in Module.LibraryClasses:
        if "ALWAYS_PRODUCED" in LibraryClass.Usage:
            Module.Header.LibraryClass.append(LibraryClass)


## Add Module Package Dependency path.
#
# Translate Package Dependency Guid to a file path relative to workspace.
#
# @param  Module              An input Module class object.
#
def AddModulePackageDependencyPath(Module):
    for PackageDependency in Module.PackageDependencies:
        PackageGuid = PackageDependency.PackageGuid
        PackageVersion = PackageDependency.PackageVersion
        
        GuidToFilePath = gEdkIIWorkspaceGuidsInfo.ResolvePackageFilePath
        PackageFilePath = GuidToFilePath(PackageGuid, PackageVersion)
        PackageDependency.FilePath = PackageFilePath


## Add Module Recommended Library Instance path.
#
# Translate Module Recommened Library Instance Guid to a file path relative to
# workspace.
#
# @param  Module              An input Module class object.
#
def AddModuleRecommonedLibraryInstancePath(Module):
    for LibraryClass in Module.LibraryClasses:
        if "ALWAYS_PRODUCED" in LibraryClass.Usage:
            continue

        if LibraryClass.RecommendedInstanceGuid == "":
            continue
        
        LibraryGuid = LibraryClass.RecommendedInstanceGuid
        LibraryVersion = LibraryClass.RecommendedIntanceVersion
        
        GuidToFilePath = gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath
        LibraryInstance = GuidToFilePath(LibraryGuid, LibraryVersion)
        LibraryClass.RecommendedIntance = LibraryInstance


## Convert MSA Module class object to INF Module class object.
#
# Convert MSA module class ojbect to INF Module class object by filling in
# several information required by INF file.
#
# @param  Module              An input Module class object.
#
def ConvertMsaModuleToInfModule(Module):
    AddModuleMiscVersion(Module)
    AddModuleProducedLibraryClass(Module)
    AddModulePackageDependencyPath(Module)
    AddModuleRecommonedLibraryInstancePath(Module)


if __name__ == '__main__':
    pass
    