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
import os.path
import sys
import glob
import shutil
import traceback
import platform
from optparse import OptionParser
import md5
import time
import uuid

from PackageFile import *
import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
from Common.Misc import *
from Common.XmlParser import *
from CommonDataClass.DistributionPackageClass import *
from Common.DecClassObjectLight import Dec
from Common.InfClassObjectLight import Inf

from PackageFile import *

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
        EdkLogger.error("MkPkg", ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                        ExtraData="WORKSPACE")

    WorkspaceDir = os.path.normpath(os.environ["WORKSPACE"])
    if not os.path.exists(WorkspaceDir):
        EdkLogger.error("MkPkg", FILE_NOT_FOUND, "WORKSPACE doesn't exist", ExtraData="%s" % WorkspaceDir)
    elif ' ' in WorkspaceDir:
        EdkLogger.error("MkPkg", FORMAT_NOT_SUPPORTED, "No space is allowed in WORKSPACE path", 
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
    UsageString = "%prog -m <module_file> -p <package_file> [-o distribution_file] " + \
                   "[-x xml-file-header] [-t tools-directory] [-f misc-files] [-q | -v] [-h]"

    Parser = OptionParser(description=__copyright__,version=__version__,prog="MkPkg",usage=UsageString)

    Parser.add_option("-?", action="help", help="show this help message and exit")

    Parser.add_option("-o", "--output-file", action="store", type="string", dest="DistributionFile",
            help="Specify the distribution file to be created.")

    Parser.add_option("-f", "--misc-files", action="append", type="string", dest="MiscFiles",
            help="Specify any misc files.")

    Parser.add_option("-x", "--xml-file-header", action="store", type=None, dest="TemplateFile",
            help="Specify the xml file which includes header information for creating the distribution file.")

    Parser.add_option("-t", "--tools-directory", action="store", type=None, dest="ToolsDir",
            help="Specify the directory name of tools.")

    Parser.add_option("-m", "--module", action="append", type="string", dest="ModuleFileList",
            help="The inf file of module to be distributed standalone.")

    Parser.add_option("-p", "--package", action="append", type="string", dest="PackageFileList",
            help="The dec file of package to be distributed.")

    Parser.add_option("-q", "--quiet", action="store_const", dest="LogLevel", const=EdkLogger.QUIET,
            help="Disable all messages except FATAL ERRORS.")

    Parser.add_option("-v", "--verbose", action="store_const", dest="LogLevel", const=EdkLogger.VERBOSE,
            help="Turn on verbose output")

    Parser.add_option("-d", "--debug", action="store", type="int", dest="LogLevel",
            help="Enable debug messages at specified level.")

    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    (Opt, Args)=Parser.parse_args()
    # error check
    if not Opt.ModuleFileList and not Opt.PackageFileList:
        EdkLogger.error("MkPkg", OPTION_NOT_SUPPORTED, ExtraData="At least one package file or module file must be specified")
    if Opt.TemplateFile:
        if not os.path.exists(Opt.TemplateFile):
            EdkLogger.error(
                            "\nMkPkg",
                            FILE_NOT_FOUND,
                            "Template file [%s] not found" % Opt.TemplateFile
                            )
    return Opt

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
        if Options.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.SetLevel(Options.LogLevel + 1)
        else:
            EdkLogger.SetLevel(Options.LogLevel)

        CheckEnvVariable()
        WorkspaceDir = os.environ["WORKSPACE"]
        
        # Init DistributionFile
        if not Options.DistributionFile:
            Options.DistributionFile = "DistributionPackage.zip"
        
        # Check Tools Dir
        if Options.ToolsDir:
            if not os.path.isdir(os.path.normpath(os.path.join(WorkspaceDir, Options.ToolsDir))):
                EdkLogger.error(
                                "\nMkPkg",
                                FILE_NOT_FOUND,
                                "Tools directory [%s] not found" % Options.ToolsDir
                                )
        
        # Check misc files
        if Options.MiscFiles:
            for Item in Options.MiscFiles:
                FullPath = os.path.normpath(os.path.join(WorkspaceDir, Item))
                if not os.path.isfile(FullPath):
                    EdkLogger.error(
                                    "\nMkPkg",
                                    FILE_NOT_FOUND,
                                    "Misc file [%s] not found" % Item
                                    )
        
        #Check package file existing and valid
        if Options.PackageFileList:
            for Item in Options.PackageFileList:
                (Name, Ext) = os.path.splitext(Item)
                if Ext.upper() != '.DEC':
                    EdkLogger.error(
                    "\nMkPkg",
                    OPTION_VALUE_INVALID,
                    "[%s] is not a valid package name" % Item
                    )
                Path = os.path.normpath(os.path.join(WorkspaceDir, Item))
                if not os.path.exists(Path):
                    EdkLogger.error(
                        "\nMkPkg",
                        FILE_NOT_FOUND,
                        "[%s] not found" % Item
                        )
        #Check module file existing and valid
        if Options.ModuleFileList:
            for Item in Options.ModuleFileList:
                (Name, Ext) = os.path.splitext(Item)
                if Ext.upper() != '.INF':
                    EdkLogger.error(
                    "\nMkPkg",
                    OPTION_VALUE_INVALID,
                    "[%s] is not a valid module name" % Item
                    )
                Path = os.path.normpath(os.path.join(WorkspaceDir, Item))
                if not os.path.exists(Path):
                    EdkLogger.error(
                        "\nMkPkg",
                        FILE_NOT_FOUND,
                        "[%s] not found" % Item
                        )

        ContentFile = PackageFile("content.zip", "w")
        DistPkg = DistributionPackageClass()
        DistPkg.GetDistributionPackage(WorkspaceDir, Options.PackageFileList, Options.ModuleFileList)
        DistPkgXml = DistributionPackageXml()
        for Item in DistPkg.PackageSurfaceArea:
            ContentFile.Pack(os.path.dirname(os.path.normpath(os.path.join(WorkspaceDir,Item[2]))))
        for Item in DistPkg.ModuleSurfaceArea:
            ContentFile.Pack(os.path.dirname(os.path.normpath(os.path.join(WorkspaceDir,Item[2]))))

        # Add tools files and information
        if Options.ToolsDir:
            ToolsFiles = MiscFileClass()
            ToolsRoot = os.path.normpath(os.path.join(WorkspaceDir, Options.ToolsDir))
            ContentFile.Pack(ToolsRoot)
            ToolsFileList = GetFiles(ToolsRoot, ['CVS', '.svn'])
            for Item in ToolsFileList:
                OriPath = Item[len(WorkspaceDir)+1:]
                FileObj = FileClass()
                FileObj.Filename = OriPath
                (Name, Ext) = os.path.splitext(OriPath)
                if Ext.upper() in ['EXE', 'COM', 'EFI']:
                    FileObj.Executable = 'True'
                ToolsFiles.Files.append(FileObj)
            DistPkg.Tools = ToolsFiles
        
        # Add misc files and information
        if Options.MiscFiles:
            MiscFiles = MiscFileClass()
            for Item in Options.MiscFiles:
                ContentFile.PackFile(Item)
                FileObj = FileClass()
                FileObj.Filename = Item
                (Name, Ext) = os.path.splitext(Item)
                if Ext.upper() in ['EXE', 'COM', 'EFI']:
                    FileObj.Executable = 'True'
                MiscFiles.Files.append(FileObj)
            DistPkg.MiscellaneousFiles = MiscFiles
                
        print "Compressing Distribution Package File ..."
        ContentFile.Close()
        
        # Add temp distribution header
        if Options.TemplateFile:
            TempXML = DistributionPackageXml()
            DistPkg.Header = TempXML.FromXml(Options.TemplateFile).Header
        # Add init dp information
        else:
            DistPkg.Header.Name = 'Distribution Package'
            DistPkg.Header.Guid = str(uuid.uuid4())
            DistPkg.Header.Version = '1.0'
        
        # Add Md5Sigature
        Md5Sigature = md5.new(open(str(ContentFile)).read())
        DistPkg.Header.Signature = Md5Sigature.hexdigest()
        # Add current Date
        DistPkg.Header.Date = str(time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime()))

        # Finish final dp file
        DistPkgFile = PackageFile(Options.DistributionFile, "w")
        DistPkgFile.PackFile(str(ContentFile))
        DistPkgFile.PackData(DistPkgXml.ToXml(DistPkg), "dist.pkg")
        DistPkgFile.Close()
        print "DONE"

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
                    "\nMkPkg",
                    CODE_ERROR,
                    "Unknown fatal error when creating [%s]" % Options.DistributionFile,
                    ExtraData="\n(Please send email to dev@buildtools.tianocore.org for help, attaching following call stack trace!)\n",
                    RaiseError=False
                    )
        EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = CODE_ERROR
    finally:
        Progressor.Abort()

if __name__ == '__main__':
    sys.exit(Main())

