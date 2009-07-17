## @file
# This file is used to define a class object to describe a distribution package
#
# Copyright (c) 2008, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

##
# Import Modules
#
import os.path
from CommonClass import *
from Common.Misc import sdict
from Common.Misc import GetFiles
from Common.DecClassObjectLight import Dec
from Common.InfClassObjectLight import Inf
from Common.XmlParser import *

## DistributionPackageHeaderClass
#
class DistributionPackageHeaderClass(IdentificationClass, CommonHeaderClass):
    def __init__(self):
        IdentificationClass.__init__(self)
        CommonHeaderClass.__init__(self)
        self.ReadOnly = 'False'
        self.RePackage = 'True'
        self.Vendor = ''
        self.Date = ''
        self.Signature = 'Md5Sum'
        self.XmlSpecification = ''

## DistributionPackageClass
#
#
class DistributionPackageClass(object):
    def __init__(self):
        self.Header = DistributionPackageHeaderClass()
        self.PackageSurfaceArea = sdict() # {(Guid, Version, Path) : PackageObj}
        self.ModuleSurfaceArea = sdict()  # {(Guid, Version, Path) : ModuleObj}
        self.Tools = MiscFileClass()
        self.MiscellaneousFiles = MiscFileClass()
        self.UserExtensions = []
    
    ## Get all included packages and modules for a distribution package
    # 
    # @param WorkspaceDir:  WorkspaceDir
    # @param PackageList:   A list of all packages
    # @param ModuleList:    A list of all modules
    #
    def GetDistributionPackage(self, WorkspaceDir, PackageList, ModuleList):
        AllGuidVersionDict = {}
        # Get Packages
        if PackageList:
            for PackageFile in PackageList:
                PackageFileFullPath = os.path.normpath(os.path.join(WorkspaceDir, PackageFile))
                DecObj = Dec(PackageFileFullPath, True, WorkspaceDir)
                PackageObj = DecObj.Package
                AllGuidVersionDict[PackageFileFullPath] = [PackageObj.PackageHeader.Guid, PackageObj.PackageHeader.Version]
                
                # Parser inf file one bye one
                for File in PackageObj.MiscFiles.Files:
                    Filename = os.path.normpath(os.path.join(PackageObj.PackageHeader.RelaPath, File.Filename))
                    (Name, ExtName) = os.path.splitext(Filename)
                    if ExtName.upper() == '.INF':
                        InfObj = Inf(Filename, True, WorkspaceDir, DecObj.Identification.PackagePath)
                        ModuleObj = InfObj.Module
                        AllGuidVersionDict[File] = [ModuleObj.ModuleHeader.Guid, ModuleObj.ModuleHeader.Version]
                        # Find and update Guid/Version of LibraryClass
                        for Item in ModuleObj.LibraryClasses:
                            if Item.RecommendedInstance:
                                LibClassIns = os.path.normpath(os.path.join(WorkspaceDir, Item.RecommendedInstance))
                                Guid, Version = '', ''
                                if LibClassIns in AllGuidVersionDict:
                                    Guid = AllGuidVersionDict[LibClassIns][0]
                                    Version = AllGuidVersionDict[LibClassIns][1]
                                else:
                                    Lib = Inf(LibClassIns, True, WorkspaceDir)
                                    Guid = Lib.Module.ModuleHeader.Guid
                                    Version = Lib.Module.ModuleHeader.Version
                                    AllGuidVersionDict[LibClassIns] = [Guid, Version]
                                Item.RecommendedInstanceGuid = Guid
                                Item.RecommendedInstanceVersion = Version
                        # Find and update Guid/Version of 
                        for Item in ModuleObj.PackageDependencies:
                            if Item.FilePath:
                                PackageFilePath = os.path.normpath(os.path.join(WorkspaceDir, Item.FilePath))
                                Guid, Version = '', ''
                                if PackageFilePath in AllGuidVersionDict:
                                    Guid = AllGuidVersionDict[PackageFilePath][0]
                                    Version = AllGuidVersionDict[PackageFilePath][1]
                                else:
                                    PackageDependencies = Dec(PackageFilePath, True, WorkspaceDir)
                                    Guid = PackageDependencies.Package.PackageHeader.Guid
                                    Version = PackageDependencies.Package.PackageHeader.Version
                                    AllGuidVersionDict[PackageFilePath] = [Guid, Version]
                                Item.PackageGuid = Guid
                                Item.PackageVersion = Version

                        # Add module to package
                        PackageObj.Modules[(ModuleObj.ModuleHeader.Guid, ModuleObj.ModuleHeader.Version, ModuleObj.ModuleHeader.CombinePath)] = ModuleObj
                self.PackageSurfaceArea[(PackageObj.PackageHeader.Guid, PackageObj.PackageHeader.Version, PackageObj.PackageHeader.CombinePath)] = PackageObj

        # Get Modules
        if ModuleList:
            for ModuleFile in ModuleList:
                ModuleFileFullPath = os.path.normpath(os.path.join(WorkspaceDir, ModuleFile))
                InfObj = Inf(ModuleFileFullPath, True, WorkspaceDir)
                ModuleObj = InfObj.Module
                AllGuidVersionDict[ModuleFileFullPath] = [ModuleObj.ModuleHeader.Guid, ModuleObj.ModuleHeader.Version]
                # Find and update Guid/Version of LibraryClass
                for Item in ModuleObj.LibraryClasses:
                    if Item.RecommendedInstance:
                        LibClassIns = os.path.normpath(os.path.join(WorkspaceDir, Item.RecommendedInstance))
                        Guid, Version = '', ''
                        if LibClassIns in AllGuidVersionDict:
                            Guid = AllGuidVersionDict[LibClassIns][0]
                            Version = AllGuidVersionDict[LibClassIns][1]
                        else:
                            Lib = Inf(LibClassIns, True, WorkspaceDir)
                            Guid = Lib.Module.ModuleHeader.Guid
                            Version = Lib.Module.ModuleHeader.Version
                            AllGuidVersionDict[LibClassIns] = [Guid, Version]
                        Item.RecommendedInstanceGuid = Guid
                        Item.RecommendedInstanceVersion = Version
                # Find and update Guid/Version of 
                for Item in ModuleObj.PackageDependencies:
                    if Item.FilePath:
                        PackageFilePath = os.path.normpath(os.path.join(WorkspaceDir, Item.FilePath))
                        Guid, Version = '', ''
                        if PackageFilePath in AllGuidVersionDict:
                            Guid = AllGuidVersionDict[PackageFilePath][0]
                            Version = AllGuidVersionDict[PackageFilePath][1]
                        else:
                            PackageDependencies = Dec(PackageFilePath, True, WorkspaceDir)
                            Guid = PackageDependencies.Package.PackageHeader.Guid
                            Version = PackageDependencies.Package.PackageHeader.Version
                            AllGuidVersionDict[PackageFilePath] = [Guid, Version]
                        Item.PackageGuid = Guid
                        Item.PackageVersion = Version
                self.ModuleSurfaceArea[(ModuleObj.ModuleHeader.Guid, ModuleObj.ModuleHeader.Version, ModuleObj.ModuleHeader.CombinePath)] = ModuleObj

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    pass
    D = DistributionPackageClass()
    D.GetDistributionPackage(os.getenv('WORKSPACE'), ['MdePkg/MdePkg.dec', 'TianoModulePkg/TianoModulePkg.dec'], ['MdeModulePkg/Application/HelloWorld/HelloWorld.inf'])
    Xml = DistributionPackageXml()
    print Xml.ToXml(D)
    E = Xml.FromXml('C:\\2.xml')
    #print Xml.ToXml(E)
