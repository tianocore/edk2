## @file
# Install distribution package.
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
#

'''
RmPkg
'''

##
# Import Modules
#
import os.path
from stat import S_IWUSR
from traceback import format_exc
from platform import python_version
import md5
from sys import stdin
from sys import platform

from Core.DependencyRules import DependencyRules
from Library import GlobalData
from Logger import StringTable as ST
import Logger.Log as Logger
from Logger.ToolError import OPTION_MISSING
from Logger.ToolError import UNKNOWN_ERROR
from Logger.ToolError import ABORT_ERROR
from Logger.ToolError import CODE_ERROR
from Logger.ToolError import FatalError


## CheckDpDepex
#
# Check if the Depex is satisfied
# @param Dep: Dep
# @param Guid: Guid of Dp
# @param Version: Version of Dp
# @param WorkspaceDir: Workspace Dir
#
def CheckDpDepex(Dep, Guid, Version, WorkspaceDir):
    (Removable, DependModuleList) = Dep.CheckDpDepexForRemove(Guid, Version)
    if not Removable:
        Logger.Info(ST.MSG_CONFIRM_REMOVE)
        Logger.Info(ST.MSG_USER_DELETE_OP)
        Input = stdin.readline()
        Input = Input.replace('\r', '').replace('\n', '')
        if Input.upper() != 'Y':
            Logger.Error("RmPkg", UNKNOWN_ERROR, ST.ERR_USER_INTERRUPT)
            return 1
        else:
            #
            # report list of modules that are not valid due to force 
            # remove,
            # also generate a log file for reference
            #
            Logger.Info(ST.MSG_INVALID_MODULE_INTRODUCED)
            LogFilePath = os.path.normpath(os.path.join(WorkspaceDir, GlobalData.gINVALID_MODULE_FILE))
            Logger.Info(ST.MSG_CHECK_LOG_FILE % LogFilePath)
            try:
                LogFile = open(LogFilePath, 'w')
                try:
                    for ModulePath in DependModuleList:
                        LogFile.write("%s\n"%ModulePath)
                        Logger.Info(ModulePath)
                except IOError:
                    Logger.Warn("\nRmPkg", ST.ERR_FILE_WRITE_FAILURE, 
                                File=LogFilePath)
            except IOError:
                Logger.Warn("\nRmPkg", ST.ERR_FILE_OPEN_FAILURE, 
                            File=LogFilePath)
            finally:                    
                LogFile.close()

## Remove Path
#
# removing readonly file on windows will get "Access is denied"
# error, so before removing, change the mode to be writeable
#
# @param Path: The Path to be removed 
#
def RemovePath(Path):
    Logger.Info(ST.MSG_REMOVE_FILE % Path)
    if not os.access(Path, os.W_OK):
        os.chmod(Path, S_IWUSR)
    os.remove(Path)
    try:
        os.removedirs(os.path.split(Path)[0])
    except OSError:
        pass
## GetCurrentFileList
#
# @param DataBase: DataBase of UPT
# @param Guid: Guid of Dp
# @param Version: Version of Dp
# @param WorkspaceDir: Workspace Dir
#
def GetCurrentFileList(DataBase, Guid, Version, WorkspaceDir):
    NewFileList = []
    for Dir in  DataBase.GetDpInstallDirList(Guid, Version):
        RootDir = os.path.normpath(os.path.join(WorkspaceDir, Dir))
        for Root, Dirs, Files in os.walk(RootDir):
            Logger.Debug(0, Dirs)
            for File in Files:
                FilePath = os.path.join(Root, File)
                if FilePath not in NewFileList:
                    NewFileList.append(FilePath)
    return NewFileList


## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @param  Options: command option 
#
def Main(Options = None):

    try:
        DataBase = GlobalData.gDB        
        if not Options.DistributionFile:
            Logger.Error("RmPkg", 
                         OPTION_MISSING, 
                         ExtraData=ST.ERR_SPECIFY_PACKAGE)
        WorkspaceDir = GlobalData.gWORKSPACE
        #
        # Prepare check dependency
        #
        Dep = DependencyRules(DataBase)
        
        #
        # Get the Dp information
        #
        StoredDistFile, Guid, Version = GetInstalledDpInfo(Options.DistributionFile, Dep, DataBase, WorkspaceDir)

        # 
        # Check Dp depex
        #
        CheckDpDepex(Dep, Guid, Version, WorkspaceDir)

        # 
        # remove distribution
        #
        RemoveDist(Guid, Version, StoredDistFile, DataBase, WorkspaceDir, Options.Yes)

        Logger.Quiet(ST.MSG_FINISH)
        
        ReturnCode = 0
        
    except FatalError, XExcept:
        ReturnCode = XExcept.args[0]        
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + \
                         format_exc())
    except KeyboardInterrupt:
        ReturnCode = ABORT_ERROR
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + \
                         format_exc())
    except:
        Logger.Error(
                    "\nRmPkg",
                    CODE_ERROR,
                    ST.ERR_UNKNOWN_FATAL_REMOVING_ERR,
                    ExtraData=ST.MSG_SEARCH_FOR_HELP,
                    RaiseError=False
                    )
        Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + \
                     format_exc())
        ReturnCode = CODE_ERROR
    return ReturnCode

## GetInstalledDpInfo method
#
# Get the installed distribution information
#
# @param  DistributionFile: the name of the distribution
# @param  Dep: the instance of DependencyRules
# @param  DataBase: the internal database
# @param  WorkspaceDir: work space directory
# @retval StoredDistFile: the distribution file that backed up
# @retval Guid: the Guid of the distribution
# @retval Version: the Version of distribution
#
def GetInstalledDpInfo(DistributionFile, Dep, DataBase, WorkspaceDir):
    (Guid, Version, NewDpFileName) = DataBase.GetDpByName(os.path.split(DistributionFile)[1])
    if not Guid:
        Logger.Error("RmPkg", UNKNOWN_ERROR, ST.ERR_PACKAGE_NOT_INSTALLED % DistributionFile)

    #
    # Check Dp existing
    #
    if not Dep.CheckDpExists(Guid, Version):
        Logger.Error("RmPkg", UNKNOWN_ERROR, ST.ERR_DISTRIBUTION_NOT_INSTALLED)
    #
    # Check for Distribution files existence in /conf/upt, if not exist, 
    # Warn user and go on.
    #
    StoredDistFile = os.path.normpath(os.path.join(WorkspaceDir, GlobalData.gUPT_DIR, NewDpFileName))
    if not os.path.isfile(StoredDistFile):
        Logger.Warn("RmPkg", ST.WRN_DIST_NOT_FOUND%StoredDistFile)
        StoredDistFile = None

    return StoredDistFile, Guid, Version

## RemoveDist method
#
# remove a distribution
#
# @param  Guid: the Guid of the distribution
# @param  Version: the Version of distribution
# @param  StoredDistFile: the distribution file that backed up
# @param  DataBase: the internal database
# @param  WorkspaceDir: work space directory
# @param  ForceRemove: whether user want to remove file even it is modified
#
def RemoveDist(Guid, Version, StoredDistFile, DataBase, WorkspaceDir, ForceRemove):
    #
    # Get Current File List
    #
    NewFileList = GetCurrentFileList(DataBase, Guid, Version, WorkspaceDir)

    #
    # Remove all files
    #
    MissingFileList = []
    for (Path, Md5Sum) in DataBase.GetDpFileList(Guid, Version):
        if os.path.isfile(Path):
            if Path in NewFileList:
                NewFileList.remove(Path)
            if not ForceRemove:
                #
                # check whether modified by users
                #
                Md5Sigature = md5.new(open(str(Path), 'rb').read())
                if Md5Sum != Md5Sigature.hexdigest():
                    Logger.Info(ST.MSG_CONFIRM_REMOVE2 % Path)
                    Input = stdin.readline()
                    Input = Input.replace('\r', '').replace('\n', '')
                    if Input.upper() != 'Y':
                        continue
            RemovePath(Path)
        else:
            MissingFileList.append(Path)
    
    for Path in NewFileList:
        if os.path.isfile(Path):
            if (not ForceRemove) and (not os.path.split(Path)[1].startswith('.')):
                Logger.Info(ST.MSG_CONFIRM_REMOVE3 % Path)
                Input = stdin.readline()
                Input = Input.replace('\r', '').replace('\n', '')
                if Input.upper() != 'Y':
                    continue
            RemovePath(Path)

    #
    # Remove distribution files in /Conf/.upt
    #
    if StoredDistFile is not None:
        os.remove(StoredDistFile)

    #
    # update database
    #
    Logger.Quiet(ST.MSG_UPDATE_PACKAGE_DATABASE)
    DataBase.RemoveDpObj(Guid, Version)
