## @file
# Replace distribution package.
#
# Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
"""
Replace a distribution package
"""
##
# Import Modules
#
from shutil import rmtree
from traceback import format_exc
from platform import python_version
from sys import platform
from Logger import StringTable as ST
from Logger.ToolError import UNKNOWN_ERROR
from Logger.ToolError import FatalError
from Logger.ToolError import ABORT_ERROR
from Logger.ToolError import CODE_ERROR
from Logger.ToolError import UPT_ALREADY_INSTALLED_ERROR
import Logger.Log as Logger

from Core.DependencyRules import DependencyRules
from Library import GlobalData
from InstallPkg import UnZipDp
from InstallPkg import InstallDp
from RmPkg import GetInstalledDpInfo
from RmPkg import RemoveDist

## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @param  Options: command Options
#
def Main(Options = None):
    ContentZipFile, DistFile = None, None
    try:
        DataBase = GlobalData.gDB
        WorkspaceDir = GlobalData.gWORKSPACE
        Dep = DependencyRules(DataBase)
        DistPkg, ContentZipFile, DpPkgFileName, DistFile = UnZipDp(WorkspaceDir, Options.PackFileToReplace)

        StoredDistFile, OrigDpGuid, OrigDpVersion = GetInstalledDpInfo(Options.PackFileToBeReplaced, \
                                                                       Dep, DataBase, WorkspaceDir)

        #
        # check dependency
        #
        CheckReplaceDpx(Dep, DistPkg, OrigDpGuid, OrigDpVersion)

        #
        # Remove the old distribution
        #
        RemoveDist(OrigDpGuid, OrigDpVersion, StoredDistFile, DataBase, WorkspaceDir, Options.Yes)

        #
        # Install the new distribution
        #
        InstallDp(DistPkg, DpPkgFileName, ContentZipFile, Options, Dep, WorkspaceDir, DataBase)
        ReturnCode = 0

    except FatalError as XExcept:
        ReturnCode = XExcept.args[0]
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(),
                platform) + format_exc())
    except KeyboardInterrupt:
        ReturnCode = ABORT_ERROR
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(),
                platform) + format_exc())
    except:
        ReturnCode = CODE_ERROR
        Logger.Error(
                    "\nReplacePkg",
                    CODE_ERROR,
                    ST.ERR_UNKNOWN_FATAL_REPLACE_ERR % (Options.PackFileToReplace, Options.PackFileToBeReplaced),
                    ExtraData=ST.MSG_SEARCH_FOR_HELP % ST.MSG_EDKII_MAIL_ADDR,
                    RaiseError=False
                    )
        Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(),
            platform) + format_exc())

    finally:
        Logger.Quiet(ST.MSG_REMOVE_TEMP_FILE_STARTED)
        if DistFile:
            DistFile.Close()
        if ContentZipFile:
            ContentZipFile.Close()
        for TempDir in GlobalData.gUNPACK_DIR:
            rmtree(TempDir)
        GlobalData.gUNPACK_DIR = []
        Logger.Quiet(ST.MSG_REMOVE_TEMP_FILE_DONE)

    if ReturnCode == 0:
        Logger.Quiet(ST.MSG_FINISH)

    return ReturnCode

def CheckReplaceDpx(Dep, DistPkg, OrigDpGuid, OrigDpVersion):
    NewDpPkgList = []
    for PkgInfo in DistPkg.PackageSurfaceArea:
        Guid, Version = PkgInfo[0], PkgInfo[1]
        NewDpPkgList.append((Guid, Version))

    NewDpInfo = "%s %s" % (DistPkg.Header.GetGuid(), DistPkg.Header.GetVersion())
    OrigDpInfo = "%s %s" % (OrigDpGuid, OrigDpVersion)

    #
    # check whether new distribution is already installed and not replacing itself
    #
    if (NewDpInfo != OrigDpInfo):
        if Dep.CheckDpExists(DistPkg.Header.GetGuid(), DistPkg.Header.GetVersion()):
            Logger.Error("\nReplacePkg", UPT_ALREADY_INSTALLED_ERROR,
                ST.WRN_DIST_PKG_INSTALLED,
                ExtraData=ST.MSG_REPLACE_ALREADY_INSTALLED_DP)

    #
    # check whether the original distribution could be replaced by new distribution
    #
    Logger.Verbose(ST.MSG_CHECK_DP_FOR_REPLACE%(NewDpInfo, OrigDpInfo))
    DepInfoResult = Dep.CheckDpDepexForReplace(OrigDpGuid, OrigDpVersion, NewDpPkgList)
    Replaceable = DepInfoResult[0]
    if not Replaceable:
        Logger.Error("\nReplacePkg", UNKNOWN_ERROR,
            ST.ERR_PACKAGE_NOT_MATCH_DEPENDENCY)

    #
    # check whether new distribution could be installed by dependency rule
    #
    Logger.Verbose(ST.MSG_CHECK_DP_FOR_INSTALL%str(NewDpInfo))
    if not Dep.ReplaceCheckNewDpDepex(DistPkg, OrigDpGuid, OrigDpVersion):
        Logger.Error("\nReplacePkg", UNKNOWN_ERROR,
            ST.ERR_PACKAGE_NOT_MATCH_DEPENDENCY,
            ExtraData=DistPkg.Header.Name)

