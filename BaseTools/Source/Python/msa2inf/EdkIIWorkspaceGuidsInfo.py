## @file
# Collects the Guid Information in current workspace.
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
import fnmatch
from Common.EdkIIWorkspace import EdkIIWorkspace
from Common.MigrationUtilities import *

## A class for EdkII work space to resolve Guids.
#
# This class inherits from EdkIIWorkspace and collects the Guids information
# in current workspace. The Guids information is important to translate the
# package Guids and recommended library instances Guids to relative file path
# (to workspace directory) in MSA files.
#
class EdkIIWorkspaceGuidsInfo(EdkIIWorkspace):

    ## The classconstructor.
    #
    # The constructor initialize workspace directory. It does not collect
    # pakage and module Guids info at initialization; instead, it collects them
    # on the fly.
    #
    # @param  self           The object pointer.
    #
    def __init__(self):
        # Initialize parent class.
        EdkIIWorkspace.__init__(self)
        # The internal map from Guid to FilePath.
        self.__GuidToFilePath = {}
        # The internal package directory list.
        self.__PackageDirList = []
        # The internal flag to indicate whether package Guids info has been
        # to avoid re-collection collected.
        self.__PackageGuidInitialized = False
        # The internal flag to indicate whether module Guids info has been
        # to avoid re-collection collected.
        self.__ModuleGuidInitialized = False

    ## Add Guid, Version and FilePath to Guids database.
    #
    # Add Guid, Version and FilePath to Guids database. It constructs a map
    # table from Guid, Version to FilePath internally. If also detects possible
    # Guid collision. For now, the version information is simply ignored and
    # Guid value itself acts as master key.
    #
    # @param  self           The object pointer.
    # @param  Guid           The Guid Value.
    # @param  Version        The version information
    #
    # @retval True           The Guid value is successfully added to map table.
    # @retval False          The Guid is an empty string or the map table
    #                        already contains a same Guid.
    #
    def __AddGuidToFilePath(self, Guid, Version, FilePath):
        if Guid == "":
            EdkLogger.info("Cannot find Guid in file %s" % FilePath)
            return False
        #Add the Guid value to map table to ensure case insensitive comparison.
        OldFilePath = self.__GuidToFilePath.setdefault(Guid.lower(), FilePath)
        if OldFilePath == FilePath:
            EdkLogger.verbose("File %s has new Guid '%s'" % (FilePath, Guid))
            return True
        else:
            EdkLogger.info("File %s has duplicate Guid with & %s" % (FilePath, OldFilePath))
            return False
        

    ## Gets file information from a module description file.
    #
    # Extracts Module Name, File Guid and Version number from INF, MSA and NMSA
    # file. It supports to exact such information from text based INF file or
    # XML based (N)MSA file.
    #
    # @param  self           The object pointer.
    # @param  FileName       The input module file name.
    #
    # @retval True           This module file represents a new module discovered
    #                        in current workspace.
    # @retval False          This module file is not regarded as a valid module.
    #                        The File Guid cannot be extracted or the another
    #                        file with the same Guid already exists
    #
    def __GetModuleFileInfo(self, FileName):
        if fnmatch.fnmatch(FileName, "*.inf"):
            TagTuple = ("BASE_NAME", "FILE_GUID", "VERSION_STRING")
            (Name, Guid, Version) = GetTextFileInfo(FileName, TagTuple)
        else :
            XmlTag1 = "ModuleSurfaceArea/MsaHeader/ModuleName"
            XmlTag2 = "ModuleSurfaceArea/MsaHeader/GuidValue"
            XmlTag3 = "ModuleSurfaceArea/MsaHeader/Version"
            TagTuple = (XmlTag1, XmlTag2, XmlTag3)
            (Name, Guid, Version) = GetXmlFileInfo(FileName, TagTuple)

        return self.__AddGuidToFilePath(Guid, Version, FileName)
    
    
    ## Gets file information from a package description file.
    #
    # Extracts Package Name, File Guid and Version number from INF, SPD and NSPD
    # file. It supports to exact such information from text based DEC file or
    # XML based (N)SPD file. EDK Compatibility Package is hardcoded to be
    # ignored since no EDKII INF file depends on that package.
    #
    # @param  self           The object pointer.
    # @param  FileName       The input package file name.
    #
    # @retval True           This package file represents a new package
    #                        discovered in current workspace.
    # @retval False          This package is not regarded as a valid package.
    #                        The File Guid cannot be extracted or the another
    #                        file with the same Guid already exists
    #
    def __GetPackageFileInfo(self, FileName):
        if fnmatch.fnmatch(FileName, "*.dec"):
            TagTuple = ("PACKAGE_NAME", "PACKAGE_GUID", "PACKAGE_VERSION")
            (Name, Guid, Version) = GetTextFileInfo(FileName, TagTuple)
        else:
            XmlTag1 = "PackageSurfaceArea/SpdHeader/PackageName"
            XmlTag2 = "PackageSurfaceArea/SpdHeader/GuidValue"
            XmlTag3 = "PackageSurfaceArea/SpdHeader/Version"
            TagTuple = (XmlTag1, XmlTag2, XmlTag3)
            (Name, Guid, Version) = GetXmlFileInfo(FileName, TagTuple)
                
        if Name == "EdkCompatibilityPkg":
            # Do not scan EDK compatibitilty package to avoid Guid collision
            # with those in EDK Glue Library.
            EdkLogger.verbose("Bypass EDK Compatibility Pkg")
            return False
        
        return self.__AddGuidToFilePath(Guid, Version, FileName)

    ## Iterate on all package files listed in framework database file.
    #
    # Yields all package description files listed in framework database files.
    # The framework database file describes the packages current workspace
    # includes.
    #
    # @param  self           The object pointer.
    #
    def __FrameworkDatabasePackageFiles(self):
        XmlFrameworkDb = XmlParseFile(self.WorkspaceFile)
        XmlTag = "FrameworkDatabase/PackageList/Filename"
        for PackageFile in XmlElementList(XmlFrameworkDb, XmlTag):
            yield os.path.join(self.WorkspaceDir, PackageFile)
    
    
    ## Iterate on all package files in current workspace directory.
    #
    # Yields all package description files listed in current workspace
    # directory. This happens when no framework database file exists.
    #
    # @param  self           The object pointer.
    #
    def __TraverseAllPackageFiles(self):
        for Path, Dirs, Files in os.walk(self.WorkspaceDir):
            # Ignore svn version control directory.
            if ".svn" in Dirs:
                Dirs.remove(".svn")
            if "Build" in Dirs:
                Dirs.remove("Build")
            # Assume priority from high to low: DEC, NSPD, SPD.
            PackageFiles = fnmatch.filter(Files, "*.dec")
            if len(PackageFiles) == 0:
                PackageFiles = fnmatch.filter(Files, "*.nspd")
                if len(PackageFiles) == 0:
                    PackageFiles = fnmatch.filter(Files, "*.spd")

            for File in PackageFiles:
                # Assume no more package decription file in sub-directory.
                del Dirs[:]
                yield os.path.join(Path, File)

    ## Iterate on all module files in current package directory.
    #
    # Yields all module description files listed in current package
    # directory.
    #
    # @param  self           The object pointer.
    #
    def __TraverseAllModuleFiles(self):
        for PackageDir in self.__PackageDirList:
            for Path, Dirs, Files in os.walk(PackageDir):
                # Ignore svn version control directory.
                if ".svn" in Dirs:
                    Dirs.remove(".svn")
                # Assume priority from high to low: INF, NMSA, MSA.
                ModuleFiles = fnmatch.filter(Files, "*.inf")
                if len(ModuleFiles) == 0:
                    ModuleFiles = fnmatch.filter(Files, "*.nmsa")
                    if len(ModuleFiles) == 0:
                        ModuleFiles = fnmatch.filter(Files, "*.msa")

                for File in ModuleFiles:
                    yield os.path.join(Path, File)

    ## Initialize package Guids info mapping table.
    #
    # Collects all package guids map to package decription file path. This
    # function is invokes on demand to avoid unnecessary directory scan.
    #
    # @param  self           The object pointer.
    #
    def __InitializePackageGuidInfo(self):
        if self.__PackageGuidInitialized:
            return

        EdkLogger.verbose("Start to collect Package Guids Info.")
   
        WorkspaceFile = os.path.join("Conf", "FrameworkDatabase.db")
        self.WorkspaceFile = os.path.join(self.WorkspaceDir, WorkspaceFile)
        
        # Try to find the frameworkdatabase file to discover package lists
        if os.path.exists(self.WorkspaceFile):
            TraversePackage = self.__FrameworkDatabasePackageFiles
            EdkLogger.verbose("Package list bases on: %s" % self.WorkspaceFile)
        else:
            TraversePackage = self.__TraverseAllPackageFiles
            EdkLogger.verbose("Package list in: %s" % self.WorkspaceDir)

        for FileName in TraversePackage():
            if self.__GetPackageFileInfo(FileName):
                PackageDir = os.path.dirname(FileName)
                EdkLogger.verbose("Find new package directory %s" % PackageDir)
                self.__PackageDirList.append(PackageDir)
                
        self.__PackageGuidInitialized = True

    ## Initialize module Guids info mapping table.
    #
    # Collects all module guids map to module decription file path. This
    # function is invokes on demand to avoid unnecessary directory scan.
    #
    # @param  self           The object pointer.
    #
    def __InitializeModuleGuidInfo(self):
        if self.__ModuleGuidInitialized:
            return
        EdkLogger.verbose("Start to collect Module Guids Info")
        
        self.__InitializePackageGuidInfo()
        for FileName in self.__TraverseAllModuleFiles():
            if self.__GetModuleFileInfo(FileName):
                EdkLogger.verbose("Find new module %s" % FileName)
                
        self.__ModuleGuidInitialized = True

    ## Get Package file path by Package guid and Version.
    #
    # Translates the Package Guid and Version to a file path relative
    # to workspace directory. If no package in current workspace match the
    # input Guid, an empty file path is returned. For now, the version
    # value is simply ignored.
    #
    # @param  self           The object pointer.
    # @param  Guid           The Package Guid value to look for.
    # @param  Version        The Package Version value to look for.
    #
    def ResolvePackageFilePath(self, Guid, Version = ""):
        self.__InitializePackageGuidInfo()
        
        EdkLogger.verbose("Resolve Package Guid '%s'" % Guid)
        FileName = self.__GuidToFilePath.get(Guid.lower(), "")
        if FileName == "":
            EdkLogger.info("Cannot resolve Package Guid '%s'" % Guid)
        else:
            FileName = self.WorkspaceRelativePath(FileName)
            FileName = os.path.splitext(FileName)[0] + ".dec"
            FileName = FileName.replace("\\", "/")
        return FileName

    ## Get Module file path by Package guid and Version.
    #
    # Translates the Module Guid and Version to a file path relative
    # to workspace directory. If no module in current workspace match the
    # input Guid, an empty file path is returned. For now, the version
    # value is simply ignored.
    #
    # @param  self           The object pointer.
    # @param  Guid           The Module Guid value to look for.
    # @param  Version        The Module Version value to look for.
    #
    def ResolveModuleFilePath(self, Guid, Version = ""):
        self.__InitializeModuleGuidInfo()
        
        EdkLogger.verbose("Resolve Module Guid '%s'" % Guid)
        FileName = self.__GuidToFilePath.get(Guid.lower(), "")
        if FileName == "":
            EdkLogger.info("Cannot resolve Module Guid '%s'" % Guid)
        else:
            FileName = self.WorkspaceRelativePath(FileName)
            FileName = os.path.splitext(FileName)[0] + ".inf"
            FileName = FileName.replace("\\", "/")
        return FileName

# A global class object of EdkIIWorkspaceGuidsInfo for external reference.
gEdkIIWorkspaceGuidsInfo = EdkIIWorkspaceGuidsInfo()

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    # Test the translation of package Guid.
    MdePkgGuid = "1E73767F-8F52-4603-AEB4-F29B510B6766"
    OldMdePkgGuid = "5e0e9358-46b6-4ae2-8218-4ab8b9bbdcec"
    print gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(MdePkgGuid)
    print gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(OldMdePkgGuid)
    
    # Test the translation of module Guid.
    UefiLibGuid = "3a004ba5-efe0-4a61-9f1a-267a46ae5ba9"
    UefiDriverModelLibGuid = "52af22ae-9901-4484-8cdc-622dd5838b09"
    print gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(UefiLibGuid)
    print gEdkIIWorkspaceGuidsInfo.ResolveModuleFilePath(UefiDriverModelLibGuid)
