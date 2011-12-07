## @file
# This file is for installed package information database operations
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

'''
Dependency
'''

##
# Import Modules
#
from os import getenv
from os import environ
from os.path import dirname

import Logger.Log as Logger
from Logger import StringTable as ST
from Library.Parsing import GetWorkspacePackage
from Library.Parsing import GetWorkspaceModule
from PomAdapter.InfPomAlignment import InfPomAlignment
from Logger.ToolError import FatalError
from Logger.ToolError import EDK1_INF_ERROR
from Logger.ToolError import UNKNOWN_ERROR
(DEPEX_CHECK_SUCCESS, DEPEX_CHECK_MODULE_NOT_FOUND, \
DEPEX_CHECK_PACKAGE_NOT_FOUND, DEPEX_CHECK_DP_NOT_FOUND) = (0, 1, 2, 3)


## IpiDb
#
# This class represents the installed package information database
# Add/Remove/Get installed distribution package information here.
# 
# 
# @param object:      Inherited from object class
#
class DependencyRules(object):
    def __init__(self, Datab):
        self.IpiDb = Datab
        self.WsPkgList = GetWorkspacePackage()
        self.WsModuleList = GetWorkspaceModule()

    ## Check whether a module exists in current workspace.
    #
    # @param Guid:  Guid of a module
    # @param Version: Version of a module
    #
    def CheckModuleExists(self, Guid, Version, Name, Path, ReturnCode=DEPEX_CHECK_SUCCESS):
        if ReturnCode:
            pass
        Logger.Verbose(ST.MSG_CHECK_MODULE_EXIST)
        ModuleList = self.IpiDb.GetModInPackage(Guid, Version, Name, Path)
        ModuleList.extend(self.IpiDb.GetStandaloneModule(Guid, Version, Name, Path))
        Logger.Verbose(ST.MSG_CHECK_MODULE_EXIST_FINISH)
        if len(ModuleList) > 0:
            return True
        else:
            return False
        
    ## Check whether a module depex satisfied by current workspace or dist.
    #
    # @param ModuleObj: A module object
    # @param DpObj: A depex object
    #
    def CheckModuleDepexSatisfied(self, ModuleObj, DpObj=None, \
                                  ReturnCode=DEPEX_CHECK_SUCCESS):
        if ReturnCode:
            pass
        Logger.Verbose(ST.MSG_CHECK_MODULE_DEPEX_START)
        Result = True
        Dep = None
        if ModuleObj.GetPackageDependencyList():
            Dep = ModuleObj.GetPackageDependencyList()[0]
        for Dep in ModuleObj.GetPackageDependencyList():
            #
            # first check whether the dependency satisfied by current workspace
            #
            Exist = self.CheckPackageExists(Dep.GetGuid(), Dep.GetVersion())
            #
            # check whether satisfied by current distribution 
            #
            if not Exist:
                if DpObj == None:
                    Result = False
                    break
                for GuidVerPair in DpObj.PackageSurfaceArea.keys():
                    if Dep.GetGuid() == GuidVerPair[0]:
                        if Dep.GetVersion() == None or \
                        len(Dep.GetVersion()) == 0:
                            Result = True
                            break
                        if Dep.GetVersion() == GuidVerPair[1]:
                            Result = True
                            break
                else:
                    Result = False
                    break
        
        if not Result:
            Logger.Error("CheckModuleDepex", UNKNOWN_ERROR, \
                         ST.ERR_DEPENDENCY_NOT_MATCH % (ModuleObj.GetName(), \
                                                        Dep.GetPackageFilePath(), \
                                                        Dep.GetGuid(), \
                                                        Dep.GetVersion()))
        return Result
            
    ## Check whether a package exists in current workspace.
    #
    # @param Guid: Guid of a package
    # @param Version: Version of a package
    #
    def CheckPackageExists(self, Guid, Version):
        Logger.Verbose(ST.MSG_CHECK_PACKAGE_START)
        for (PkgName, PkgGuid, PkgVer, PkgPath) in self.WsPkgList:
            if PkgName or PkgPath:
                pass
            if (PkgGuid == Guid):
                #
                # if version is not empty and not equal, then not match
                #
                if Version and (PkgVer != Version):
                    return False
                else:
                    return True
        else:
            return False
                        
        Logger.Verbose(ST.MSG_CHECK_PACKAGE_FINISH)
         
    ## Check whether a package depex satisfied by current workspace.
    #
    # @param PkgObj: A package object
    # @param DpObj: A package depex object
    #
    def CheckPackageDepexSatisfied(self, PkgObj, DpObj=None, \
                                   ReturnCode=DEPEX_CHECK_SUCCESS):
        
        ModuleDict = PkgObj.GetModuleDict()
        for ModKey in ModuleDict.keys():
            ModObj = ModuleDict[ModKey]
            if self.CheckModuleDepexSatisfied(ModObj, DpObj, ReturnCode):
                continue
            else:
                return False
        return True
        
    ## Check whether a DP exists in current workspace.
    #
    # @param Guid: Guid of a module
    # @param Version: Version of a module
    #
    def CheckDpExists(self, Guid, Version, ReturnCode=DEPEX_CHECK_SUCCESS):
        if ReturnCode:
            pass
        Logger.Verbose(ST.MSG_CHECK_DP_START)
        DpList = self.IpiDb.GetDp(Guid, Version)
        if len(DpList) > 0:
            return True
        else:
            return False
            
        Logger.Verbose(ST.MSG_CHECK_DP_FINISH) 
        
    ## Check whether a DP depex satisfied by current workspace.
    #
    # @param DpObj:  Depex object
    # @param ReturnCode: ReturnCode
    #
    def CheckDpDepexSatisfied(self, DpObj, ReturnCode=DEPEX_CHECK_SUCCESS):
        
        for PkgKey in DpObj.PackageSurfaceArea.keys():
            PkgObj = DpObj.PackageSurfaceArea[PkgKey]
            if self.CheckPackageDepexSatisfied(PkgObj, DpObj, ReturnCode):
                continue
            else:
                return False
            
        for ModKey in DpObj.ModuleSurfaceArea.keys():
            ModObj = DpObj.ModuleSurfaceArea[ModKey]
            if self.CheckModuleDepexSatisfied(ModObj, DpObj, ReturnCode):
                continue
            else:
                return False
        
        return True
    
    ## Check whether a DP depex satisfied by current workspace. Return False 
    # if Can not remove (there is dependency), True else
    #
    # @param DpGuid:  File's guid
    # @param DpVersion: File's version
    # @param ReturnCode: ReturnCode
    # 
    def CheckDpDepexForRemove(self, DpGuid, DpVersion, \
                              ReturnCode=DEPEX_CHECK_SUCCESS):
        if ReturnCode:
            pass
        Removable = True
        DependModuleList = []
        WsModuleList = self.WsModuleList
        #
        # remove modules that included in current DP
        # List of item (FilePath)
        DpModuleList = self.IpiDb.GetDpModuleList(DpGuid, DpVersion) 
        for Module in DpModuleList:
            if Module in WsModuleList:
                WsModuleList.remove(Module)
            else:
                Logger.Warn("UPT\n",
                            ST.ERR_MODULE_NOT_INSTALLED % Module)
        #
        # get packages in current Dp and find the install path
        # List of item (PkgGuid, PkgVersion, InstallPath)
        DpPackageList = self.IpiDb.GetPackageListFromDp(DpGuid, DpVersion) 
        DpPackagePathList = []
        WorkSP = environ["WORKSPACE"]
        for (PkgName, PkgGuid, PkgVersion, DecFile) in self.WsPkgList:
            if PkgName:
                pass
            DecPath = dirname(DecFile)
            if DecPath.find(WorkSP) > -1:
                InstallPath = DecPath[DecPath.find(WorkSP) + len(WorkSP) + 1:]
                DecFileRelaPath = \
                DecFile[DecFile.find(WorkSP) + len(WorkSP) + 1:]
            else:
                InstallPath = DecPath
                DecFileRelaPath = DecFile
                
            if (PkgGuid, PkgVersion, InstallPath) in DpPackageList:
                DpPackagePathList.append(DecFileRelaPath)
                DpPackageList.remove((PkgGuid, PkgVersion, InstallPath))
        
        #
        # the left items in DpPackageList are the packages that installed but not found anymore
        #
        for (PkgGuid, PkgVersion, InstallPath) in DpPackageList:
            Logger.Warn("UPT",
                        ST.WARN_INSTALLED_PACKAGE_NOT_FOUND%(PkgGuid, PkgVersion, InstallPath))
        
        #
        # check modules to see if has dependency on package of current DP
        #
        for Module in WsModuleList:
            if (CheckModuleDependFromInf(Module, DpPackagePathList)):
                Removable = False
                DependModuleList.append(Module)
        return (Removable, DependModuleList)


## check whether module depends on packages in DpPackagePathList, return True 
# if found, False else
#
# @param Path: a module path
# @param DpPackagePathList: a list of Package Paths
#
def CheckModuleDependFromInf(Path, DpPackagePathList):
    
    #  
    # use InfParser to parse inf, then get the information for now,
    # later on, may consider only parse to get the package dependency info 
    # (Need to take care how to deal wit Macros)
    #
    WorkSP = getenv('WORKSPACE')
    
    try:
        PomAli = InfPomAlignment(Path, WorkSP, Skip=True)

        for Item in PomAli.GetPackageDependencyList():
            if Item.GetPackageFilePath() in DpPackagePathList:
                Logger.Info(ST.MSG_MODULE_DEPEND_ON % (Path, Item.GetPackageFilePath()))
                return True
        else:
            return False
    except FatalError, ErrCode:
        if ErrCode.message == EDK1_INF_ERROR:
            Logger.Warn("UPT",
                        ST.WRN_EDK1_INF_FOUND%Path)
            return False
        else:
            return False
        


