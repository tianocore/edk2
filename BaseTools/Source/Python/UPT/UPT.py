## @file
#
# This file is the main entry for UPT 
#
# Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

'''
UPT
'''

## import modules
#
from Core import FileHook
import sys
import os.path
from sys import platform
import platform as pf
from optparse import OptionParser
from traceback import format_exc
from platform import python_version

from Logger import StringTable as ST
import Logger.Log as Logger
from Logger.StringTable import MSG_VERSION
from Logger.StringTable import MSG_DESCRIPTION
from Logger.StringTable import MSG_USAGE
from Logger.ToolError import FILE_NOT_FOUND
from Logger.ToolError import OPTION_MISSING
from Logger.ToolError import FILE_TYPE_MISMATCH
from Logger.ToolError import OPTION_CONFLICT
from Logger.ToolError import FatalError
from Logger.ToolError import UPT_ALREADY_INSTALLED_ERROR

import MkPkg
import InstallPkg
import RmPkg
import InventoryWs
import ReplacePkg
from Library.Misc import GetWorkspace
from Library import GlobalData
from Core.IpiDb import IpiDatabase
from BuildVersion import gBUILD_VERSION

## CheckConflictOption
#
# CheckConflictOption
#
def CheckConflictOption(Opt):
    if (Opt.PackFileToCreate or Opt.PackFileToInstall or Opt.PackFileToRemove or Opt.PackFileToReplace) \
    and Opt.InventoryWs:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_L_OA_EXCLUSIVE)
    elif Opt.PackFileToReplace and (Opt.PackFileToCreate or Opt.PackFileToInstall or Opt.PackFileToRemove):
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_U_ICR_EXCLUSIVE)
    elif (Opt.PackFileToCreate and Opt.PackFileToInstall and Opt.PackFileToRemove):
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_REQUIRE_I_C_R_OPTION)
    elif Opt.PackFileToCreate and Opt.PackFileToInstall:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_I_C_EXCLUSIVE)
    elif Opt.PackFileToInstall and Opt.PackFileToRemove:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_I_R_EXCLUSIVE)
    elif Opt.PackFileToCreate and  Opt.PackFileToRemove:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_C_R_EXCLUSIVE)

    if Opt.CustomPath and Opt.UseGuidedPkgPath:
        Logger.Warn("UPT", ST.WARN_CUSTOMPATH_OVERRIDE_USEGUIDEDPATH)
        Opt.UseGuidedPkgPath = False

## SetLogLevel
#
def SetLogLevel(Opt):
    if Opt.opt_verbose:
        Logger.SetLevel(Logger.VERBOSE)
    elif Opt.opt_quiet:
        Logger.SetLevel(Logger.QUIET + 1)
    elif Opt.debug_level != None:
        if Opt.debug_level < 0 or Opt.debug_level > 9:
            Logger.Warn("UPT", ST.ERR_DEBUG_LEVEL)
            Logger.SetLevel(Logger.INFO)
        else:
            Logger.SetLevel(Opt.debug_level + 1)
    elif Opt.opt_slient:
        Logger.SetLevel(Logger.SILENT)
    else:
        Logger.SetLevel(Logger.INFO)

## Main
#
# Main
#
def Main():
    Logger.Initialize()

    Parser = OptionParser(version=(MSG_VERSION + ' ' + gBUILD_VERSION), description=MSG_DESCRIPTION,
                          prog="UPT.exe", usage=MSG_USAGE)

    Parser.add_option("-d", "--debug", action="store", type="int", dest="debug_level", help=ST.HLP_PRINT_DEBUG_INFO)

    Parser.add_option("-v", "--verbose", action="store_true", dest="opt_verbose",
                      help=ST.HLP_PRINT_INFORMATIONAL_STATEMENT)

    Parser.add_option("-s", "--silent", action="store_true", dest="opt_slient", help=ST.HLP_RETURN_NO_DISPLAY)

    Parser.add_option("-q", "--quiet", action="store_true", dest="opt_quiet", help=ST.HLP_RETURN_AND_DISPLAY)

    Parser.add_option("-i", "--install", action="store", type="string", dest="Install_Distribution_Package_File",
                      help=ST.HLP_SPECIFY_PACKAGE_NAME_INSTALL)

    Parser.add_option("-c", "--create", action="store", type="string", dest="Create_Distribution_Package_File",
                      help=ST.HLP_SPECIFY_PACKAGE_NAME_CREATE)

    Parser.add_option("-r", "--remove", action="store", type="string", dest="Remove_Distribution_Package_File",
                      help=ST.HLP_SPECIFY_PACKAGE_NAME_REMOVE)

    Parser.add_option("-t", "--template", action="store", type="string", dest="Package_Information_Data_File",
                      help=ST.HLP_SPECIFY_TEMPLATE_NAME_CREATE)

    Parser.add_option("-p", "--dec-filename", action="append", type="string", dest="EDK2_DEC_Filename",
                      help=ST.HLP_SPECIFY_DEC_NAME_CREATE)

    Parser.add_option("-m", "--inf-filename", action="append", type="string", dest="EDK2_INF_Filename",
                      help=ST.HLP_SPECIFY_INF_NAME_CREATE)

    Parser.add_option("-l", "--list", action="store_true", dest="List_Dist_Installed",
                      help=ST.HLP_LIST_DIST_INSTALLED)

    Parser.add_option("-f", "--force", action="store_true", dest="Yes", help=ST.HLP_DISABLE_PROMPT)

    Parser.add_option("-n", "--custom-path", action="store_true", dest="CustomPath", help=ST.HLP_CUSTOM_PATH_PROMPT)

    Parser.add_option("-x", "--free-lock", action="store_true", dest="SkipLock", help=ST.HLP_SKIP_LOCK_CHECK)

    Parser.add_option("-u", "--replace", action="store", type="string", dest="Replace_Distribution_Package_File",
                      help=ST.HLP_SPECIFY_PACKAGE_NAME_REPLACE)

    Parser.add_option("-o", "--original", action="store", type="string", dest="Original_Distribution_Package_File",
                      help=ST.HLP_SPECIFY_PACKAGE_NAME_TO_BE_REPLACED)

    Parser.add_option("--use-guided-paths", action="store_true", dest="Use_Guided_Paths", help=ST.HLP_USE_GUIDED_PATHS)

    Opt = Parser.parse_args()[0]

    Var2Var = [
        ("PackageInformationDataFile", Opt.Package_Information_Data_File),
        ("PackFileToInstall", Opt.Install_Distribution_Package_File),
        ("PackFileToCreate", Opt.Create_Distribution_Package_File),
        ("PackFileToRemove", Opt.Remove_Distribution_Package_File),
        ("PackageFileList", Opt.EDK2_DEC_Filename),
        ("ModuleFileList", Opt.EDK2_INF_Filename),
        ("InventoryWs", Opt.List_Dist_Installed),
        ("PackFileToReplace", Opt.Replace_Distribution_Package_File),
        ("PackFileToBeReplaced", Opt.Original_Distribution_Package_File),
        ("UseGuidedPkgPath", Opt.Use_Guided_Paths),
    ]

    for Var in Var2Var:
        setattr(Opt, Var[0], Var[1])

    try:
        GlobalData.gWORKSPACE = GetWorkspace()
    except FatalError, XExcept:
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + format_exc())
        return XExcept.args[0]

    # Start *********************************************
    # Support WORKSPACE is a long path
    # Only work well on windows
    # Linux Solution TBD
    if pf.system() == 'Windows':
        os.system('@echo off\nsubst b: /D')
        os.system('subst b: "%s"' % GlobalData.gWORKSPACE)
        GlobalData.gWORKSPACE = 'B:\\'
    # End ***********************************************

    WorkspaceDir = GlobalData.gWORKSPACE

    SetLogLevel(Opt)

    Mgr = FileHook.RecoverMgr(WorkspaceDir)
    FileHook.SetRecoverMgr(Mgr)

    GlobalData.gDB = IpiDatabase(os.path.normpath(os.path.join(WorkspaceDir, \
                                                               "Conf/DistributionPackageDatabase.db")), WorkspaceDir)
    GlobalData.gDB.InitDatabase(Opt.SkipLock)

    #
    # Make sure the Db will get closed correctly
    #
    try:
        ReturnCode = 0
        CheckConflictOption(Opt)

        RunModule = None
        if Opt.PackFileToCreate:
            if Opt.PackageInformationDataFile:
                if not os.path.exists(Opt.PackageInformationDataFile):
                    if not os.path.exists(os.path.join(WorkspaceDir, Opt.PackageInformationDataFile)):
                        Logger.Error("\nUPT", FILE_NOT_FOUND, ST.ERR_NO_TEMPLATE_FILE % Opt.PackageInformationDataFile)
                    else:
                        Opt.PackageInformationDataFile = os.path.join(WorkspaceDir, Opt.PackageInformationDataFile)
            else:
                Logger.Error("UPT", OPTION_MISSING, ExtraData=ST.ERR_REQUIRE_T_OPTION)
            if not Opt.PackFileToCreate.endswith('.dist'):
                Logger.Error("CreatePkg", FILE_TYPE_MISMATCH, ExtraData=ST.ERR_DIST_EXT_ERROR % Opt.PackFileToCreate)
            RunModule = MkPkg.Main

        elif Opt.PackFileToInstall:
            if not Opt.PackFileToInstall.endswith('.dist'):
                Logger.Error("InstallPkg", FILE_TYPE_MISMATCH, ExtraData=ST.ERR_DIST_EXT_ERROR % Opt.PackFileToInstall)

            AbsPath = GetFullPathDist(Opt.PackFileToInstall, WorkspaceDir)
            if not AbsPath:
                Logger.Error("InstallPkg", FILE_NOT_FOUND, ST.ERR_INSTALL_DIST_NOT_FOUND % Opt.PackFileToInstall)

            Opt.PackFileToInstall = AbsPath
            setattr(Opt, 'PackageFile', Opt.PackFileToInstall)
            RunModule = InstallPkg.Main

        elif Opt.PackFileToRemove:
            if not Opt.PackFileToRemove.endswith('.dist'):
                Logger.Error("RemovePkg", FILE_TYPE_MISMATCH, ExtraData=ST.ERR_DIST_EXT_ERROR % Opt.PackFileToRemove)
            head, tail = os.path.split(Opt.PackFileToRemove)
            if head or not tail:
                Logger.Error("RemovePkg",
                             FILE_TYPE_MISMATCH,
                             ExtraData=ST.ERR_DIST_FILENAME_ONLY_FOR_REMOVE % Opt.PackFileToRemove)

            setattr(Opt, 'DistributionFile', Opt.PackFileToRemove)
            RunModule = RmPkg.Main
        elif Opt.InventoryWs:
            RunModule = InventoryWs.Main

        elif Opt.PackFileToBeReplaced and not Opt.PackFileToReplace:
            Logger.Error("ReplacePkg", OPTION_MISSING, ExtraData=ST.ERR_REQUIRE_U_OPTION)

        elif Opt.PackFileToReplace:
            if not Opt.PackFileToReplace.endswith('.dist'):
                Logger.Error("ReplacePkg", FILE_TYPE_MISMATCH, ExtraData=ST.ERR_DIST_EXT_ERROR % Opt.PackFileToReplace)
            if not Opt.PackFileToBeReplaced:
                Logger.Error("ReplacePkg", OPTION_MISSING, ExtraData=ST.ERR_REQUIRE_O_OPTION)
            if not Opt.PackFileToBeReplaced.endswith('.dist'):
                Logger.Error("ReplacePkg",
                             FILE_TYPE_MISMATCH,
                             ExtraData=ST.ERR_DIST_EXT_ERROR % Opt.PackFileToBeReplaced)

            head, tail = os.path.split(Opt.PackFileToBeReplaced)
            if head or not tail:
                Logger.Error("ReplacePkg",
                             FILE_TYPE_MISMATCH,
                             ExtraData=ST.ERR_DIST_FILENAME_ONLY_FOR_REPLACE_ORIG % Opt.PackFileToBeReplaced)

            AbsPath = GetFullPathDist(Opt.PackFileToReplace, WorkspaceDir)
            if not AbsPath:
                Logger.Error("ReplacePkg", FILE_NOT_FOUND, ST.ERR_REPLACE_DIST_NOT_FOUND % Opt.PackFileToReplace)

            Opt.PackFileToReplace = AbsPath
            RunModule = ReplacePkg.Main

        else:
            Parser.print_usage()
            return OPTION_MISSING

        ReturnCode = RunModule(Opt)
    except FatalError, XExcept:
        ReturnCode = XExcept.args[0]
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + \
                         format_exc())
    finally:
        try:
            if ReturnCode != 0 and ReturnCode != UPT_ALREADY_INSTALLED_ERROR:
                Logger.Quiet(ST.MSG_RECOVER_START)
                GlobalData.gDB.RollBack()
                Mgr.rollback()
                Logger.Quiet(ST.MSG_RECOVER_DONE)
            else:
                GlobalData.gDB.Commit()
                Mgr.commit()
        except StandardError:
            Logger.Quiet(ST.MSG_RECOVER_FAIL)
        GlobalData.gDB.CloseDb()
        if pf.system() == 'Windows':
            os.system('subst b: /D')

    return ReturnCode

## GetFullPathDist
#
#  This function will check DistFile existence, if not absolute path, then try current working directory,
#  then $(WORKSPACE),and return the AbsPath. If file doesn't find, then return None
#
# @param DistFile:       The distribution file in either relative path or absolute path
# @param WorkspaceDir:   Workspace Directory
# @return AbsPath:       The Absolute path of the distribution file if existed, None else
#
def GetFullPathDist(DistFile, WorkspaceDir):
    if os.path.isabs(DistFile):
        if not (os.path.exists(DistFile) and os.path.isfile(DistFile)):
            return None
        else:
            return DistFile
    else:
        AbsPath = os.path.normpath(os.path.join(os.getcwd(), DistFile))
        if not (os.path.exists(AbsPath) and os.path.isfile(AbsPath)):
            AbsPath = os.path.normpath(os.path.join(WorkspaceDir, DistFile))
            if not (os.path.exists(AbsPath) and os.path.isfile(AbsPath)):
                return None

        return AbsPath

if __name__ == '__main__':
    RETVAL = Main()
    #
    # 0-127 is a safe return range, and 1 is a standard default error
    #
    if RETVAL < 0 or RETVAL > 127:
        RETVAL = 1
    sys.exit(RETVAL)
