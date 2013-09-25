## @file
#
# This file is the main entry for UPT 
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
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
import sys
import os.path
from os import environ
from sys import platform
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
import MkPkg
import InstallPkg
import RmPkg
from Library.Misc import CheckEnvVariable
from Library import GlobalData
from Core.IpiDb import IpiDatabase
from BuildVersion import gBUILD_VERSION

##
# Version and Copyright
#
#VersionNumber = "1.0"
#__version__ = "Revision " + VersionNumber
#__copyright__ = "Copyright (c) 2011 Intel Corporation All Rights Reserved."

## CheckConflictOption
#
# CheckConflictOption
#
def CheckConflictOption(Opt):
    if (Opt.PackFileToCreate and Opt.PackFileToInstall and Opt.PackFileToRemove):
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_REQUIRE_I_C_R_OPTION)
    elif Opt.PackFileToCreate and Opt.PackFileToInstall:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_I_C_EXCLUSIVE)
    elif Opt.PackFileToInstall and Opt.PackFileToRemove:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_I_R_EXCLUSIVE)
    elif Opt.PackFileToCreate and  Opt.PackFileToRemove:
        Logger.Error("UPT", OPTION_CONFLICT, ExtraData=ST.ERR_C_R_EXCLUSIVE)

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

    Parser.add_option("-f", "--force", action="store_true", dest="Yes", help=ST.HLP_DISABLE_PROMPT)

    Parser.add_option("-n", "--custom-path", action="store_true", dest="CustomPath", help=ST.HLP_CUSTOM_PATH_PROMPT)

    Parser.add_option("-x", "--free-lock", action="store_true", dest="SkipLock", help=ST.HLP_SKIP_LOCK_CHECK)

    Opt = Parser.parse_args()[0]

    Var2Var = [
        ("PackageInformationDataFile", Opt.Package_Information_Data_File),
        ("PackFileToInstall", Opt.Install_Distribution_Package_File),
        ("PackFileToCreate", Opt.Create_Distribution_Package_File),
        ("PackFileToRemove", Opt.Remove_Distribution_Package_File),
        ("PackageFileList", Opt.EDK2_DEC_Filename),
        ("ModuleFileList", Opt.EDK2_INF_Filename)
    ]
    for Var in Var2Var:
        setattr(Opt, Var[0], Var[1])

    try:
        CheckEnvVariable()
    except FatalError, XExcept:
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + format_exc())
        return XExcept.args[0]

    GlobalData.gWORKSPACE = os.path.normpath(environ["WORKSPACE"])
    WorkspaceDir = GlobalData.gWORKSPACE

    SetLogLevel(Opt)

    GlobalData.gDB = IpiDatabase(os.path.normpath(os.path.join(WorkspaceDir, "Conf/DistributionPackageDatabase.db")))
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
            
            #
            # check file existence, if not absolute path, then try current working directory, then $(WORKSPACE) 
            #
            Existed = True
            if os.path.isabs(Opt.PackFileToInstall):
                if not (os.path.exists(Opt.PackFileToInstall) and os.path.isfile(Opt.PackFileToInstall)):
                    Existed = False
            else:
                AbsPath = os.path.normpath(os.path.join(os.getcwd(), Opt.PackFileToInstall))
                if not (os.path.exists(AbsPath) and os.path.isfile(AbsPath)):
                    AbsPath = os.path.normpath(os.path.join(WorkspaceDir, Opt.PackFileToInstall))
                    if not (os.path.exists(AbsPath) and os.path.isfile(AbsPath)):
                        Existed = False
                
                if Existed:
                    Opt.PackFileToInstall = AbsPath
            
            if not Existed:
                Logger.Error("InstallPkg", FILE_NOT_FOUND, ST.ERR_INSTALL_DIST_NOT_FOUND % Opt.PackFileToInstall)

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
        GlobalData.gDB.CloseDb()

    return ReturnCode

if __name__ == '__main__':
    RETVAL = Main()
    #
    # 0-127 is a safe return range, and 1 is a standard default error
    #
    if RETVAL < 0 or RETVAL > 127:
        RETVAL = 1
    sys.exit(RETVAL)
