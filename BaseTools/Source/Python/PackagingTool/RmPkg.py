## @file
# Install distribution package.
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
import sys
import traceback
import platform
from optparse import OptionParser

import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
from Common.Misc import *
from Common.XmlParser import *

from IpiDb import *
from DependencyRules import *

# Version and Copyright
VersionNumber = "0.1"
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2008, Intel Corporation  All rights reserved."

## Check environment variables
#
#  Check environment variables that must be set for build. Currently they are
#
#   WORKSPACE           The directory all packages/platforms start from
#   EDK_TOOLS_PATH      The directory contains all tools needed by the build
#   PATH                $(EDK_TOOLS_PATH)/Bin/<sys> must be set in PATH
#
#   If any of above environment variable is not set or has error, the build
#   will be broken.
#
def CheckEnvVariable():
    # check WORKSPACE
    if "WORKSPACE" not in os.environ:
        EdkLogger.error("RmPkg", ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                        ExtraData="WORKSPACE")

    WorkspaceDir = os.path.normpath(os.environ["WORKSPACE"])
    if not os.path.exists(WorkspaceDir):
        EdkLogger.error("RmPkg", FILE_NOT_FOUND, "WORKSPACE doesn't exist", ExtraData="%s" % WorkspaceDir)
    elif ' ' in WorkspaceDir:
        EdkLogger.error("RmPkg", FORMAT_NOT_SUPPORTED, "No space is allowed in WORKSPACE path", 
                        ExtraData=WorkspaceDir)
    os.environ["WORKSPACE"] = WorkspaceDir

## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
#   @retval Opt   A optparse.Values object containing the parsed options
#   @retval Args  Target of build command
#
def MyOptionParser():
    UsageString = "%prog -g <guid> -n <version> [-y] [-q | -v] [-h]"

    Parser = OptionParser(description=__copyright__,version=__version__,prog="RmPkg",usage=UsageString)

    Parser.add_option("-?", action="help", help="show this help message and exit")

#    Parser.add_option("-f", "--force", action="store_true", type=None, dest="ForceRemove",
#            help="Force creation - overwrite existing one.")

    Parser.add_option("-y", "--yes", action="store_true", dest="Yes",
            help="Not asking for confirmation when deleting files.")

    Parser.add_option("-n", "--package-version", action="store", type="string", dest="PackageVersion",
            help="The version of distribution package to be removed.")

    Parser.add_option("-g", "--package-guid", action="store", type="string", dest="PackageGuid",
            help="The GUID of distribution package to be removed.")

    Parser.add_option("-q", "--quiet", action="store_const", dest="LogLevel", const=EdkLogger.QUIET,
            help="Disable all messages except FATAL ERRORS.")

    Parser.add_option("-v", "--verbose", action="store_const", dest="LogLevel", const=EdkLogger.VERBOSE,
            help="Turn on verbose output")

    Parser.add_option("-d", "--debug", action="store", type="int", dest="LogLevel",
            help="Enable debug messages at specified level.")

    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    (Opt, Args)=Parser.parse_args()

    return Opt

## Remove all empty dirs under the path
def RemoveEmptyDirs(Path):
    # Remove all sub dirs
    for Root, Dirs, Files in os.walk(Path):
        for Dir in Dirs:
            FullPath = os.path.normpath(os.path.join(Root, Dir))
            if os.path.isdir(FullPath):
                if os.listdir(FullPath) == []:
                    os.rmdir(FullPath)
                else:
                    RemoveEmptyDirs(FullPath)
    # Remove itself
    if os.path.isdir(Path) and os.listdir(Path) == []:
        os.rmdir(Path)
        

## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
#   @retval 0     Tool was successful
#   @retval 1     Tool failed
#
def Main():
    EdkLogger.Initialize()
    Options = MyOptionParser()
    try:
        if not Options.PackageGuid and not Options.PackageVersion:
            EdkLogger.error("RmPkg", OPTION_MISSING, ExtraData="The GUID and Version of distribution package must be specified")
        
        if Options.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.SetLevel(Options.LogLevel + 1)
        else:
            EdkLogger.SetLevel(Options.LogLevel)

        CheckEnvVariable()
        WorkspaceDir = os.environ["WORKSPACE"]

        # Prepare check dependency
        Db = IpiDatabase(os.path.normpath(os.path.join(WorkspaceDir, "Conf/DistributionPackageDatabase.db")))
        Db.InitDatabase()
        Dep = DependencyRules(Db)
        
        Guid = Options.PackageGuid
        Version = Options.PackageVersion
        
        # Check Dp existing
        if not Dep.CheckDpExists(Guid, Version):
            EdkLogger.error("RmPkg", UNKNOWN_ERROR, "This distribution package are not installed!")
        
        # Check Dp depex
        if not Dep.CheckDpDepexForRemove(Guid, Version):
            print "Some packages/modules are depending on this distribution package, do you really want to remove it?"
            print "Press Y to delete all files or press other keys to quit:"
            Input = Input = sys.stdin.readline()
            Input = Input.replace('\r', '').replace('\n', '')
            if Input.upper() != 'Y':
                EdkLogger.error("RmPkg", UNKNOWN_ERROR, "User interrupt")

        # Remove all files
        if not Options.Yes:
            print "All files of the distribution package will be removed, do you want to continue?"
            print "Press Y to remove all files or press other keys to quit:"
            Input = Input = sys.stdin.readline()
            Input = Input.replace('\r', '').replace('\n', '')
            if Input.upper() != 'Y':
                EdkLogger.error("RmPkg", UNKNOWN_ERROR, "User interrupt")
        
        # Remove all files
        MissingFileList = []
        for Item in Db.GetDpFileList(Guid, Version):
            if os.path.isfile(Item):
                print "Removing file [%s] ..." % Item
                os.remove(Item)
            else:
                MissingFileList.append(Item)
        
        # Remove all empty dirs of package
        for Item in Db.GetPackageListFromDp(Guid, Version):
            Dir = os.path.dirname(Item[2])
            RemoveEmptyDirs(Dir)

        # Remove all empty dirs of module
        for Item in Db.GetStandaloneModuleInstallPathListFromDp(Guid, Version):
            Dir = os.path.dirname(Item)
            RemoveEmptyDirs(Dir)
        
        # update database
        EdkLogger.quiet("Update Distribution Package Database ...")
        Db.RemoveDpObj(Guid, Version)
        EdkLogger.quiet("DONE")
        
    except FatalError, X:
        if Options and Options.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = X.args[0]
    except KeyboardInterrupt:
        ReturnCode = ABORT_ERROR
        if Options and Options.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
    except:
        EdkLogger.error(
                    "\nRmPkg",
                    CODE_ERROR,
                    "Unknown fatal error when removing package",
                    ExtraData="\n(Please send email to dev@buildtools.tianocore.org for help, attaching following call stack trace!)\n",
                    RaiseError=False
                    )
        EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = CODE_ERROR
    finally:
        Progressor.Abort()

if __name__ == '__main__':
    sys.exit(Main())
