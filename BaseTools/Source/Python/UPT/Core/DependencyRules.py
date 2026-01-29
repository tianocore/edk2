## @file
# This file is for installed package information database operations
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Dependency
'''

##
# Import Modules
#
from os.path import dirname
import os

import Logger.Log as Logger
from Logger import StringTable as ST
from Library.Parsing import GetWorkspacePackage
from Library.Parsing import GetWorkspaceModule
from Library.Parsing import GetPkgInfoFromDec
from Library.Misc import GetRelativePath
from Library import GlobalData
from Logger.ToolError import FatalError
from Logger.ToolError import EDK1_INF_ERROR
from Logger.ToolError import UNKNOWN_ERROR
(DEPEX_CHECK_SUCCESS, DEPEX_CHECK_MODULE_NOT_FOUND, \
DEPEX_CHECK_PACKAGE_NOT_FOUND, DEPEX_CHECK_DP_NOT_FOUND) = (0, 1, 2, 3)


## DependencyRules
#
# This class represents the dependency rule check mechanism
#
# @param object:      Inherited from object class
#
class DependencyRules(object):
    def __init__(self, Datab, ToBeInstalledPkgList=None):
        self.IpiDb = Datab
        self.WsPkgList = GetWorkspacePackage()
        self.WsModuleList = GetWorkspaceModule()

        self.PkgsToBeDepend = [(PkgInfo[1], PkgInfo[2]) for PkgInfo in self.WsPkgList]

        # Add package info from the DIST to be installed.
        self.PkgsToBeDepend.extend(self.GenToBeInstalledPkgList(ToBeInstalledPkgList))

    def GenToBeInstalledPkgList(self, ToBeInstalledPkgList):
        if not ToBeInstalledPkgList:
            return []
        RtnList = []
        for Dist in ToBeInstalledPkgList:
            for Package in Dist.PackageSurfaceArea:
                RtnList.append((Package[0], Package[1]))

        return RtnList

    ## Check whether a module exists by checking the Guid+Version+Name+Path combination
    #
    # @param Guid:  Guid of a module
    # @param Version: Version of a module
    # @param Name: Name of a module
    # @param Path: Path of a module
    # @return:  True if module existed, else False
    #
    def CheckModuleExists(self, Guid, Version, Name, Path):
        Logger.Verbose(ST.MSG_CHECK_MODULE_EXIST)
        ModuleList = self.IpiDb.GetModInPackage(Guid, Version, Name, Path)
        ModuleList.extend(self.IpiDb.GetStandaloneModule(Guid, Version, Name, Path))
        Logger.Verbose(ST.MSG_CHECK_MODULE_EXIST_FINISH)
        if len(ModuleList) > 0:
            return True
        else:
            return False

    ## Check whether a module depex satisfied.
    #
    # @param ModuleObj: A module object
    # @param DpObj: A distribution object
    # @return: True if module depex satisfied
    #          False else
    #
    def CheckModuleDepexSatisfied(self, ModuleObj, DpObj=None):
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
                if DpObj is None:
                    Result = False
                    break
                for GuidVerPair in DpObj.PackageSurfaceArea.keys():
                    if Dep.GetGuid() == GuidVerPair[0]:
                        if Dep.GetVersion() is None or \
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

    ## Check whether a package exists in a package list specified by PkgsToBeDepend.
    #
    # @param Guid: Guid of a package
    # @param Version: Version of a package
    # @return: True if package exist
    #          False else
    #
    def CheckPackageExists(self, Guid, Version):
        Logger.Verbose(ST.MSG_CHECK_PACKAGE_START)
        Found = False
        for (PkgGuid, PkgVer) in self.PkgsToBeDepend:
            if (PkgGuid == Guid):
                #
                # if version is not empty and not equal, then not match
                #
                if Version and (PkgVer != Version):
                    Found = False
                    break
                else:
                    Found = True
                    break
        else:
            Found = False

        Logger.Verbose(ST.MSG_CHECK_PACKAGE_FINISH)
        return Found

    ## Check whether a package depex satisfied.
    #
    # @param PkgObj: A package object
    # @param DpObj: A distribution object
    # @return: True if package depex satisfied
    #          False else
    #
    def CheckPackageDepexSatisfied(self, PkgObj, DpObj=None):
        ModuleDict = PkgObj.GetModuleDict()
        for ModKey in ModuleDict.keys():
            ModObj = ModuleDict[ModKey]
            if self.CheckModuleDepexSatisfied(ModObj, DpObj):
                continue
            else:
                return False
        return True

    ## Check whether a DP exists.
    #
    # @param Guid: Guid of a Distribution
    # @param Version: Version of a Distribution
    # @return: True if Distribution exist
    #          False else
    def CheckDpExists(self, Guid, Version):
        Logger.Verbose(ST.MSG_CHECK_DP_START)
        DpList = self.IpiDb.GetDp(Guid, Version)
        if len(DpList) > 0:
            Found = True
        else:
            Found = False

        Logger.Verbose(ST.MSG_CHECK_DP_FINISH)
        return Found

    ## Check whether a DP depex satisfied by current workspace for Install
    #
    # @param DpObj:  A distribution object
    # @return: True if distribution depex satisfied
    #          False else
    #
    def CheckInstallDpDepexSatisfied(self, DpObj):
        return self.CheckDpDepexSatisfied(DpObj)

    # # Check whether multiple DP depex satisfied by current workspace for Install
    #
    # @param DpObjList:  A distribution object list
    # @return: True if distribution depex satisfied
    #          False else
    #
    def CheckTestInstallPdDepexSatisfied(self, DpObjList):
        for DpObj in DpObjList:
            if self.CheckDpDepexSatisfied(DpObj):
                for PkgKey in DpObj.PackageSurfaceArea.keys():
                    PkgObj = DpObj.PackageSurfaceArea[PkgKey]
                    self.PkgsToBeDepend.append((PkgObj.Guid, PkgObj.Version))
            else:
                return False, DpObj

        return True, DpObj


    ## Check whether a DP depex satisfied by current workspace
    #  (excluding the original distribution's packages to be replaced) for Replace
    #
    # @param DpObj:  A distribution object
    # @param OrigDpGuid: The original distribution's Guid
    # @param OrigDpVersion: The original distribution's Version
    #
    def ReplaceCheckNewDpDepex(self, DpObj, OrigDpGuid, OrigDpVersion):
        self.PkgsToBeDepend = [(PkgInfo[1], PkgInfo[2]) for PkgInfo in self.WsPkgList]
        OrigDpPackageList = self.IpiDb.GetPackageListFromDp(OrigDpGuid, OrigDpVersion)
        for OrigPkgInfo in OrigDpPackageList:
            Guid, Version = OrigPkgInfo[0], OrigPkgInfo[1]
            if (Guid, Version) in self.PkgsToBeDepend:
                self.PkgsToBeDepend.remove((Guid, Version))
        return self.CheckDpDepexSatisfied(DpObj)

    ## Check whether a DP depex satisfied by current workspace.
    #
    # @param DpObj:  A distribution object
    #
    def CheckDpDepexSatisfied(self, DpObj):
        for PkgKey in DpObj.PackageSurfaceArea.keys():
            PkgObj = DpObj.PackageSurfaceArea[PkgKey]
            if self.CheckPackageDepexSatisfied(PkgObj, DpObj):
                continue
            else:
                return False

        for ModKey in DpObj.ModuleSurfaceArea.keys():
            ModObj = DpObj.ModuleSurfaceArea[ModKey]
            if self.CheckModuleDepexSatisfied(ModObj, DpObj):
                continue
            else:
                return False

        return True

    ## Check whether a DP could be removed from current workspace.
    #
    # @param DpGuid:  File's guid
    # @param DpVersion: File's version
    # @retval Removable: True if distribution could be removed, False Else
    # @retval DependModuleList: the list of modules that make distribution can not be removed
    #
    def CheckDpDepexForRemove(self, DpGuid, DpVersion):
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
        WorkSP = GlobalData.gWORKSPACE
        for (PkgName, PkgGuid, PkgVersion, DecFile) in self.WsPkgList:
            if PkgName:
                pass
            DecPath = dirname(DecFile)
            if DecPath.find(WorkSP) > -1:
                InstallPath = GetRelativePath(DecPath, WorkSP)
                DecFileRelaPath = GetRelativePath(DecFile, WorkSP)
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
            if (not VerifyRemoveModuleDep(Module, DpPackagePathList)):
                Removable = False
                DependModuleList.append(Module)
        return (Removable, DependModuleList)


    ## Check whether a DP could be replaced by a distribution containing NewDpPkgList
    # from current workspace.
    #
    # @param OrigDpGuid:  original Dp's Guid
    # @param OrigDpVersion: original Dp's version
    # @param NewDpPkgList: a list of package information (Guid, Version) in new Dp
    # @retval Replaceable: True if distribution could be replaced, False Else
    # @retval DependModuleList: the list of modules that make distribution can not be replaced
    #
    def CheckDpDepexForReplace(self, OrigDpGuid, OrigDpVersion, NewDpPkgList):
        Replaceable = True
        DependModuleList = []
        WsModuleList = self.WsModuleList
        #
        # remove modules that included in current DP
        # List of item (FilePath)
        DpModuleList = self.IpiDb.GetDpModuleList(OrigDpGuid, OrigDpVersion)
        for Module in DpModuleList:
            if Module in WsModuleList:
                WsModuleList.remove(Module)
            else:
                Logger.Warn("UPT\n",
                            ST.ERR_MODULE_NOT_INSTALLED % Module)

        OtherPkgList = NewDpPkgList
        #
        # get packages in current Dp and find the install path
        # List of item (PkgGuid, PkgVersion, InstallPath)
        DpPackageList = self.IpiDb.GetPackageListFromDp(OrigDpGuid, OrigDpVersion)
        DpPackagePathList = []
        WorkSP = GlobalData.gWORKSPACE
        for (PkgName, PkgGuid, PkgVersion, DecFile) in self.WsPkgList:
            if PkgName:
                pass
            DecPath = dirname(DecFile)
            if DecPath.find(WorkSP) > -1:
                InstallPath = GetRelativePath(DecPath, WorkSP)
                DecFileRelaPath = GetRelativePath(DecFile, WorkSP)
            else:
                InstallPath = DecPath
                DecFileRelaPath = DecFile

            if (PkgGuid, PkgVersion, InstallPath) in DpPackageList:
                DpPackagePathList.append(DecFileRelaPath)
                DpPackageList.remove((PkgGuid, PkgVersion, InstallPath))
            else:
                OtherPkgList.append((PkgGuid, PkgVersion))

        #
        # the left items in DpPackageList are the packages that installed but not found anymore
        #
        for (PkgGuid, PkgVersion, InstallPath) in DpPackageList:
            Logger.Warn("UPT",
                        ST.WARN_INSTALLED_PACKAGE_NOT_FOUND%(PkgGuid, PkgVersion, InstallPath))

        #
        # check modules to see if it can be satisfied by package not belong to removed DP
        #
        for Module in WsModuleList:
            if (not VerifyReplaceModuleDep(Module, DpPackagePathList, OtherPkgList)):
                Replaceable = False
                DependModuleList.append(Module)
        return (Replaceable, DependModuleList)


## check whether module depends on packages in DpPackagePathList, return True
# if found, False else
#
# @param Path: a module path
# @param DpPackagePathList: a list of Package Paths
# @retval:  False: module depends on package in DpPackagePathList
#           True:  module doesn't depend on package in DpPackagePathList
#
def VerifyRemoveModuleDep(Path, DpPackagePathList):
    try:
        for Item in GetPackagePath(Path):
            if Item in DpPackagePathList:
                DecPath = os.path.normpath(os.path.join(GlobalData.gWORKSPACE, Item))
                Logger.Info(ST.MSG_MODULE_DEPEND_ON % (Path, DecPath))
                return False
        else:
            return True
    except FatalError as ErrCode:
        if ErrCode.message == EDK1_INF_ERROR:
            Logger.Warn("UPT",
                        ST.WRN_EDK1_INF_FOUND%Path)
            return True
        else:
            return True

# # GetPackagePath
#
# Get Dependency package path from an Inf file path
#
def GetPackagePath(InfPath):
    PackagePath = []
    if os.path.exists(InfPath):
        FindSection = False
        for Line in open(InfPath).readlines():
            Line = Line.strip()
            if not Line:
                continue
            if Line.startswith('#'):
                continue
            if Line.startswith('[Packages') and Line.endswith(']'):
                FindSection = True
                continue
            if Line.startswith('[') and Line.endswith(']') and FindSection:
                break
            if FindSection:
                PackagePath.append(os.path.normpath(Line))

    return PackagePath

## check whether module depends on packages in DpPackagePathList and can not be satisfied by OtherPkgList
#
# @param Path: a module path
# @param DpPackagePathList:  a list of Package Paths
# @param OtherPkgList:       a list of Package Information (Guid, Version)
# @retval:  False: module depends on package in DpPackagePathList and can not be satisfied by OtherPkgList
#           True:  either module doesn't depend on DpPackagePathList or module depends on DpPackagePathList
#                 but can be satisfied by OtherPkgList
#
def VerifyReplaceModuleDep(Path, DpPackagePathList, OtherPkgList):
    try:
        for Item in GetPackagePath(Path):
            if Item in DpPackagePathList:
                DecPath = os.path.normpath(os.path.join(GlobalData.gWORKSPACE, Item))
                Name, Guid, Version = GetPkgInfoFromDec(DecPath)
                if (Guid, Version) not in OtherPkgList:
                    Logger.Info(ST.MSG_MODULE_DEPEND_ON % (Path, DecPath))
                    return False
        else:
            return True
    except FatalError as ErrCode:
        if ErrCode.message == EDK1_INF_ERROR:
            Logger.Warn("UPT",
                        ST.WRN_EDK1_INF_FOUND%Path)
            return True
        else:
            return True
