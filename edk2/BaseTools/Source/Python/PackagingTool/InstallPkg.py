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
import glob
import shutil
import traceback
import platform
from optparse import OptionParser

import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
from Common.Misc import *
from Common.XmlParser import *
from Common.InfClassObjectLight import Inf
from Common.DecClassObjectLight import Dec

from PackageFile import *
from IpiDb import *
from DependencyRules import *
import md5

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
        EdkLogger.error("InstallPkg", ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                        ExtraData="WORKSPACE")

    WorkspaceDir = os.path.normpath(os.environ["WORKSPACE"])
    if not os.path.exists(WorkspaceDir):
        EdkLogger.error("InstallPkg", FILE_NOT_FOUND, "WORKSPACE doesn't exist", ExtraData="%s" % WorkspaceDir)
    elif ' ' in WorkspaceDir:
        EdkLogger.error("InstallPkg", FORMAT_NOT_SUPPORTED, "No space is allowed in WORKSPACE path", 
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
    UsageString = "%prog -i <distribution_package> [-t] [-f] [-q | -v] [-h]"

    Parser = OptionParser(description=__copyright__,version=__version__,prog="InstallPkg",usage=UsageString)

    Parser.add_option("-?", action="help", help="show this help message and exit")

    Parser.add_option("-i", "--distribution-package", action="store", type="string", dest="PackageFile",
            help="The distribution package to be installed")

    Parser.add_option("-t", "--install-tools", action="store_true", type=None, dest="Tools",
            help="Specify it to install tools or ignore the tools of the distribution package.")
    
    Parser.add_option("-f", "--misc-files", action="store_true", type=None, dest="MiscFiles",
            help="Specify it to install misc file or ignore the misc files of the distribution package.")

    Parser.add_option("-q", "--quiet", action="store_const", dest="LogLevel", const=EdkLogger.QUIET,
            help="Disable all messages except FATAL ERRORS.")

    Parser.add_option("-v", "--verbose", action="store_const", dest="LogLevel", const=EdkLogger.VERBOSE,
            help="Turn on verbose output")

    Parser.add_option("-d", "--debug", action="store", type="int", dest="LogLevel",
            help="Enable debug messages at specified level.")

    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    (Opt, Args)=Parser.parse_args()

    return Opt

def InstallNewPackage(WorkspaceDir, Path):
    FullPath = os.path.normpath(os.path.join(WorkspaceDir, Path))
    if os.path.exists(FullPath):
        print "Directory [%s] already exists, please select another location, press [Enter] with no input to quit:" %Path
        Input = sys.stdin.readline()
        Input = Input.replace('\r', '').replace('\n', '')
        if Input == '':
            EdkLogger.error("InstallPkg", UNKNOWN_ERROR, "User interrupt")
        Input = Input.replace('\r', '').replace('\n', '')
        return InstallNewPackage(WorkspaceDir, Input)
    else:
        return Path

def InstallNewFile(WorkspaceDir, File):
    FullPath = os.path.normpath(os.path.join(WorkspaceDir, File))
    if os.path.exists(FullPath):
        print "File [%s] already exists, please select another path, press [Enter] with no input to quit:" %File
        Input = sys.stdin.readline()
        Input = Input.replace('\r', '').replace('\n', '')
        if Input == '':
            EdkLogger.error("InstallPkg", UNKNOWN_ERROR, "User interrupt")
        Input = Input.replace('\r', '').replace('\n', '')
        return InstallNewFile(WorkspaceDir, Input)
    else:
        return File

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
    Options = None
    DistFileName = 'dist.pkg'
    ContentFileName = 'content.zip'
    DistFile, ContentZipFile, UnpackDir = None, None, None
    
    Options = MyOptionParser()
    try:
        if Options.LogLevel < EdkLogger.DEBUG_9:
            EdkLogger.SetLevel(Options.LogLevel + 1)
        else:
            EdkLogger.SetLevel(Options.LogLevel)

        CheckEnvVariable()
        WorkspaceDir = os.environ["WORKSPACE"]
        if not Options.PackageFile:
            EdkLogger.error("InstallPkg", OPTION_NOT_SUPPORTED, ExtraData="Must specify one distribution package")

        # unzip dist.pkg file
        EdkLogger.quiet("Unzipping and parsing distribution package XML file ... ")
        DistFile = PackageFile(Options.PackageFile)
        UnpackDir = os.path.normpath(os.path.join(WorkspaceDir, ".tmp"))
        DistPkgFile = DistFile.UnpackFile(DistFileName, os.path.normpath(os.path.join(UnpackDir, DistFileName)))
        if not DistPkgFile:
            EdkLogger.error("InstallPkg", FILE_NOT_FOUND, "File [%s] is broken in distribution package" %DistFileName)
        
        # Generate distpkg
        DistPkgObj = DistributionPackageXml()
        DistPkg = DistPkgObj.FromXml(DistPkgFile)

        # prepare check dependency
        Db = IpiDatabase(os.path.normpath(os.path.join(WorkspaceDir, "Conf/DistributionPackageDatabase.db")))
        Db.InitDatabase()
        Dep = DependencyRules(Db)
        
        # Check distribution package exist
        if Dep.CheckDpExists(DistPkg.Header.Guid, DistPkg.Header.Version):
            EdkLogger.error("InstallPkg", UNKNOWN_ERROR, "This distribution package has been installed", ExtraData=DistPkg.Header.Name)
        
        # unzip contents.zip file
        ContentFile = DistFile.UnpackFile(ContentFileName, os.path.normpath(os.path.join(UnpackDir, ContentFileName)))
        ContentZipFile = PackageFile(ContentFile)
        if not ContentFile:
            EdkLogger.error("InstallPkg", FILE_NOT_FOUND, "File [%s] is broken in distribution package" %ContentFileName)
        
        # verify MD5 signature
        Md5Sigature = md5.new(open(ContentFile).read())
        if DistPkg.Header.Signature != Md5Sigature.hexdigest():
            EdkLogger.error("InstallPkg", FILE_CHECKSUM_FAILURE, ExtraData=ContentFile)
        
        # Check package exist and install
        for Guid,Version,Path in DistPkg.PackageSurfaceArea:
            PackagePath = os.path.dirname(Path)
            NewPackagePath = PackagePath
            Package = DistPkg.PackageSurfaceArea[Guid,Version,Path]
            EdkLogger.info("Installing package ... %s" % Package.PackageHeader.Name)
            if Dep.CheckPackageExists(Guid, Version):
                EdkLogger.quiet("Package [%s] has been installed" %Path)
            NewPackagePath = InstallNewPackage(WorkspaceDir, PackagePath)
            Package.FileList = []
            for Item in Package.MiscFiles.Files:
                FromFile = os.path.join(PackagePath, Item.Filename)
                ToFile = os.path.normpath(os.path.join(WorkspaceDir, NewPackagePath, Item.Filename))
                ContentZipFile.UnpackFile(FromFile, ToFile)
                Package.FileList.append(ToFile)
            
            # Update package
            Package.PackageHeader.CombinePath = Package.PackageHeader.CombinePath.replace(PackagePath, NewPackagePath, 1)
            # Update modules of package
            Module = None
            for ModuleGuid, ModuleVersion, ModulePath in Package.Modules:
                Module = Package.Modules[ModuleGuid, ModuleVersion, ModulePath]
                NewModulePath = ModulePath.replace(PackagePath, NewPackagePath, 1)
                del Package.Modules[ModuleGuid, ModuleVersion, ModulePath]
                Package.Modules[ModuleGuid, ModuleVersion, NewModulePath] = Module
            del DistPkg.PackageSurfaceArea[Guid,Version,Path]
            DistPkg.PackageSurfaceArea[Guid,Version,Package.PackageHeader.CombinePath] = Package

#            SaveFileOnChange(os.path.join(Options.InstallDir, ModulePath, Module.Header.Name, ".inf"), Inf.ModuleToInf(Module), False)
#            EdkLogger.info("Installing package ... %s" % Package.Header.Name)
#            shutil.copytree(os.path.join(ContentFileDir, Path), Options.InstallDir)
#            SaveFileOnChange(os.path.join(Options.InstallDir, Path, Package.Header.Name, ".dec"), Dec.PackageToDec(Package), False)

        # Check module exist and install
        Module = None
        for Guid,Version,Path in DistPkg.ModuleSurfaceArea:
            ModulePath = os.path.dirname(Path)
            NewModulePath = ModulePath
            Module = DistPkg.ModuleSurfaceArea[Guid,Version,Path]
            EdkLogger.info("Installing module ... %s" % Module.ModuleHeader.Name)
            if Dep.CheckModuleExists(Guid, Version):
                EdkLogger.quiet("Module [%s] has been installed" %Path)
            NewModulePath = InstallNewPackage(WorkspaceDir, ModulePath)
            Module.FileList = []
            for Item in Module.MiscFiles.Files:
                ModulePath = ModulePath[os.path.normpath(ModulePath).rfind(os.path.normpath('/'))+1:]
                FromFile = os.path.join(ModulePath, Item.Filename)
                ToFile = os.path.normpath(os.path.join(WorkspaceDir, NewModulePath, Item.Filename))
                ContentZipFile.UnpackFile(FromFile, ToFile)
                Module.FileList.append(ToFile)
            
#            EdkLogger.info("Installing module ... %s" % Module.Header.Name)
#            shutil.copytree(os.path.join(ContentFileDir, Path), Options.InstallDir)
#            SaveFileOnChange(os.path.join(Options.InstallDir, Path, Module.Header.Name, ".inf"), Inf.ModuleToInf(Module), False)
            
            # Update module
            Module.ModuleHeader.CombinePath = Module.ModuleHeader.CombinePath.replace(os.path.dirname(Path), NewModulePath, 1)
            del DistPkg.ModuleSurfaceArea[Guid,Version,Path]
            DistPkg.ModuleSurfaceArea[Guid,Version,Module.ModuleHeader.CombinePath] = Module
#            
#        
#        for Guid,Version,Path in DistPkg.PackageSurfaceArea:
#            print Guid,Version,Path
#            for item in DistPkg.PackageSurfaceArea[Guid,Version,Path].FileList:
#                print item
#        for Guid,Version,Path in DistPkg.ModuleSurfaceArea:
#            print Guid,Version,Path
#            for item in DistPkg.ModuleSurfaceArea[Guid,Version,Path].FileList:
#                print item

        if Options.Tools:
            EdkLogger.info("Installing tools ... ")
            for File in DistPkg.Tools.Files:
                FromFile = File.Filename
                ToFile = InstallNewFile(WorkspaceDir, FromFile)
                ContentZipFile.UnpackFile(FromFile, ToFile)
        if Options.MiscFiles:
            EdkLogger.info("Installing misc files ... ")
            for File in DistPkg.MiscellaneousFiles.Files:
                FromFile = File.Filename
                ToFile = InstallNewFile(WorkspaceDir, FromFile)
                ContentZipFile.UnpackFile(FromFile, ToFile)

        # update database
        EdkLogger.quiet("Update Distribution Package Database ...")
        Db.AddDPObject(DistPkg)

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
                    "\nInstallPkg",
                    CODE_ERROR,
                    "Unknown fatal error when installing [%s]" % Options.PackageFile,
                    ExtraData="\n(Please send email to dev@buildtools.tianocore.org for help, attaching following call stack trace!)\n",
                    RaiseError=False
                    )
        EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = CODE_ERROR
    finally:
        EdkLogger.quiet("Removing temp files ... ")
        if DistFile:
            DistFile.Close()
        if ContentZipFile:
            ContentZipFile.Close()
        if UnpackDir:
            shutil.rmtree(UnpackDir)
        
        EdkLogger.quiet("DONE")
        Progressor.Abort()

if __name__ == '__main__':
    sys.exit(Main())
