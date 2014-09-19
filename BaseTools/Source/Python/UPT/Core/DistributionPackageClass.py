## @file
# This file is used to define a class object to describe a distribution package
#
# Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

'''
DistributionPackageClass
'''

##
# Import Modules
#
import os.path

from Library.Misc import Sdict
from Library.Misc import GetNonMetaDataFiles
from PomAdapter.InfPomAlignment import InfPomAlignment
from PomAdapter.DecPomAlignment import DecPomAlignment
import Logger.Log as Logger
from Logger import StringTable as ST
from Logger.ToolError import OPTION_VALUE_INVALID
from Logger.ToolError import FatalError
from Logger.ToolError import EDK1_INF_ERROR
from Object.POM.CommonObject import IdentificationObject
from Object.POM.CommonObject import CommonHeaderObject
from Object.POM.CommonObject import MiscFileObject

## DistributionPackageHeaderClass
#
# @param IdentificationObject: Identification Object
# @param CommonHeaderObject: Common Header Object
#
class DistributionPackageHeaderObject(IdentificationObject, \
                                      CommonHeaderObject):
    def __init__(self):
        IdentificationObject.__init__(self)
        CommonHeaderObject.__init__(self)
        self.ReadOnly = ''
        self.RePackage = ''
        self.Vendor = ''
        self.Date = ''
        self.Signature = 'Md5Sum'
        self.XmlSpecification = ''
    
    def GetReadOnly(self):
        return self.ReadOnly
    
    def SetReadOnly(self, ReadOnly):
        self.ReadOnly = ReadOnly
    
    def GetRePackage(self):
        return self.RePackage
    
    def SetRePackage(self, RePackage):
        self.RePackage = RePackage
        
    def GetVendor(self):
        return self.Vendor
    
    def SetDate(self, Date):
        self.Date = Date
        
    def GetDate(self):
        return self.Date
    
    def SetSignature(self, Signature):
        self.Signature = Signature
        
    def GetSignature(self):
        return self.Signature
    
    def SetXmlSpecification(self, XmlSpecification):
        self.XmlSpecification = XmlSpecification
        
    def GetXmlSpecification(self):
        return self.XmlSpecification
    
## DistributionPackageClass
#
# @param object: DistributionPackageClass
# 
class DistributionPackageClass(object):
    def __init__(self):
        self.Header = DistributionPackageHeaderObject()
        #
        # {(Guid, Version, Path) : PackageObj}
        #
        self.PackageSurfaceArea = Sdict() 
        #
        # {(Guid, Version, Name, Path) : ModuleObj}
        #
        self.ModuleSurfaceArea = Sdict()  
        self.Tools = MiscFileObject()
        self.MiscellaneousFiles = MiscFileObject()
        self.UserExtensions = []
        self.FileList = []
    
    ## Get all included packages and modules for a distribution package
    # 
    # @param WorkspaceDir:  WorkspaceDir
    # @param PackageList:   A list of all packages
    # @param ModuleList:    A list of all modules
    #
    def GetDistributionPackage(self, WorkspaceDir, PackageList, ModuleList):
        #
        # Get Packages
        #
        if PackageList:
            for PackageFile in PackageList:
                PackageFileFullPath = \
                os.path.normpath(os.path.join(WorkspaceDir, PackageFile))
                DecObj = DecPomAlignment(PackageFileFullPath, WorkspaceDir, CheckMulDec = True)
                PackageObj = DecObj
                #
                # Parser inf file one bye one
                #
                ModuleInfFileList = PackageObj.GetModuleFileList()
                for File in ModuleInfFileList:
                    WsRelPath = os.path.join(PackageObj.GetPackagePath(), File)
                    WsRelPath = os.path.normpath(WsRelPath)
                    if ModuleList and WsRelPath in ModuleList:
                        Logger.Error("UPT",
                                     OPTION_VALUE_INVALID, 
                                     ST.ERR_NOT_STANDALONE_MODULE_ERROR%\
                                     (WsRelPath, PackageFile))
                    Filename = os.path.normpath\
                    (os.path.join(PackageObj.GetRelaPath(), File))
                    os.path.splitext(Filename)
                    #
                    # Call INF parser to generate Inf Object.
                    # Actually, this call is not directly call, but wrapped by 
                    # Inf class in InfPomAlignment.
                    #
                    try:
                        ModuleObj = InfPomAlignment(Filename, WorkspaceDir, \
                                                PackageObj.GetPackagePath())
     
                        #
                        # Add module to package
                        #
                        ModuleDict = PackageObj.GetModuleDict()
                        ModuleDict[(ModuleObj.GetGuid(), \
                                    ModuleObj.GetVersion(), \
                                    ModuleObj.GetName(), \
                                    ModuleObj.GetCombinePath())] = ModuleObj
                        PackageObj.SetModuleDict(ModuleDict)
                    except FatalError, ErrCode:
                        if ErrCode.message == EDK1_INF_ERROR:
                            Logger.Warn("UPT",
                                        ST.WRN_EDK1_INF_FOUND%Filename)
                        else:
                            raise
                
                self.PackageSurfaceArea\
                [(PackageObj.GetGuid(), PackageObj.GetVersion(), \
                  PackageObj.GetCombinePath())] = PackageObj

        #
        # Get Modules
        #
        if ModuleList:
            for ModuleFile in ModuleList:
                ModuleFileFullPath = \
                os.path.normpath(os.path.join(WorkspaceDir, ModuleFile))
                try:
                    ModuleObj = InfPomAlignment(ModuleFileFullPath, 
                                                WorkspaceDir)
                    ModuleKey = (ModuleObj.GetGuid(), 
                                 ModuleObj.GetVersion(), 
                                 ModuleObj.GetName(), 
                                 ModuleObj.GetCombinePath())
                    self.ModuleSurfaceArea[ModuleKey] = ModuleObj
                except FatalError, ErrCode:
                    if ErrCode.message == EDK1_INF_ERROR:
                        Logger.Error("UPT",
                                     EDK1_INF_ERROR,
                                     ST.WRN_EDK1_INF_FOUND%ModuleFileFullPath, 
                                     ExtraData=ST.ERR_NOT_SUPPORTED_SA_MODULE)
                    else:
                        raise                

    ## Get all files included for a distribution package, except tool/misc of 
    # distribution level
    # 
    # @retval DistFileList  A list of filepath for NonMetaDataFile, relative to workspace
    # @retval MetaDataFileList  A list of filepath for MetaDataFile, relative to workspace
    #
    def GetDistributionFileList(self):
        MetaDataFileList = []
        SkipModulesUniList = []
        
        for Guid, Version, Path in self.PackageSurfaceArea:
            Package = self.PackageSurfaceArea[Guid, Version, Path]
            PackagePath = Package.GetPackagePath()
            FullPath = Package.GetFullPath()
            MetaDataFileList.append(Path)
            IncludePathList = Package.GetIncludePathList()
            for IncludePath in IncludePathList:
                SearchPath = os.path.normpath(os.path.join(os.path.dirname(FullPath), IncludePath))
                AddPath = os.path.normpath(os.path.join(PackagePath, IncludePath))
                self.FileList += GetNonMetaDataFiles(SearchPath, ['CVS', '.svn'], False, AddPath)
            #
            # Add the miscellaneous files on DEC file
            #
            for MiscFileObj in Package.GetMiscFileList():
                for FileObj in MiscFileObj.GetFileList():
                    MiscFileFullPath = os.path.normpath(os.path.join(PackagePath, FileObj.GetURI()))
                    if MiscFileFullPath not in self.FileList:
                        self.FileList.append(MiscFileFullPath)
            
            Module = None
            ModuleDict = Package.GetModuleDict()
            for Guid, Version, Name, Path in ModuleDict:
                Module = ModuleDict[Guid, Version, Name, Path]
                ModulePath = Module.GetModulePath()
                FullPath = Module.GetFullPath()
                PkgRelPath = os.path.normpath(os.path.join(PackagePath, ModulePath))
                MetaDataFileList.append(Path)
                SkipList = ['CVS', '.svn']
                NonMetaDataFileList = []
                if Module.UniFileClassObject:
                    for UniFile in Module.UniFileClassObject.IncFileList:
                        OriPath = os.path.normpath(os.path.dirname(FullPath))
                        UniFilePath = os.path.normpath(os.path.join(PkgRelPath, UniFile.Path[len(OriPath) + 1:]))
                        if UniFilePath not in SkipModulesUniList:
                            SkipModulesUniList.append(UniFilePath)
                    for IncludeFile in Module.UniFileClassObject.IncludePathList:
                        if IncludeFile not in SkipModulesUniList:
                            SkipModulesUniList.append(IncludeFile)
                NonMetaDataFileList = GetNonMetaDataFiles(os.path.dirname(FullPath), SkipList, False, PkgRelPath)
                for NonMetaDataFile in NonMetaDataFileList:
                    if NonMetaDataFile not in self.FileList:
                        self.FileList.append(NonMetaDataFile)
        for Guid, Version, Name, Path in self.ModuleSurfaceArea:
            Module = self.ModuleSurfaceArea[Guid, Version, Name, Path]
            ModulePath = Module.GetModulePath()
            FullPath = Module.GetFullPath()
            MetaDataFileList.append(Path)
            SkipList = ['CVS', '.svn']
            NonMetaDataFileList = []
            if Module.UniFileClassObject:
                for UniFile in Module.UniFileClassObject.IncFileList:
                    OriPath = os.path.normpath(os.path.dirname(FullPath))
                    UniFilePath = os.path.normpath(os.path.join(ModulePath, UniFile.Path[len(OriPath) + 1:]))
                    if UniFilePath not in SkipModulesUniList:
                        SkipModulesUniList.append(UniFilePath)
            NonMetaDataFileList = GetNonMetaDataFiles(os.path.dirname(FullPath), SkipList, False, ModulePath)
            for NonMetaDataFile in NonMetaDataFileList:
                if NonMetaDataFile not in self.FileList:
                    self.FileList.append(NonMetaDataFile)
            
        for SkipModuleUni in SkipModulesUniList:
            if SkipModuleUni in self.FileList:
                self.FileList.remove(SkipModuleUni)

        return  self.FileList, MetaDataFileList

        

