# @file
# Script to Build EmulatorPkg UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
import io

from edk2toolext.environment import shell_environment
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext.invocables.edk2_platform_build import BuildSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import GetHostInfo

# ####################################################################################### #
#                                Common Configuration                                     #
# ####################################################################################### #


class CommonPlatform():
    ''' Common settings for this platform.  Define static data here and use
        for the different parts of stuart
    '''
    PackagesSupported = ("EmulatorPkg",)
    ArchSupported = ("X64", "IA32")
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('emulatorpkg', 'edk2-build')
    WorkspaceRoot = os.path.realpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".."))

    # ####################################################################################### #
    #                         Configuration for Update & Setup                                #
    # ####################################################################################### #


class SettingsManager(UpdateSettingsManager, SetupSettingsManager, PrEvalSettingsManager):

    def GetPackagesSupported(self):
        ''' return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths '''
        return CommonPlatform.PackagesSupported

    def GetArchitecturesSupported(self):
        ''' return iterable of edk2 architectures supported by this build '''
        return CommonPlatform.ArchSupported

    def GetTargetsSupported(self):
        ''' return iterable of edk2 target tags supported by this build '''
        return CommonPlatform.TargetsSupported

    def GetRequiredSubmodules(self):
        ''' return iterable containing RequiredSubmodule objects.
        If no RequiredSubmodules return an empty iterable
        '''
        rs = []
        # intentionally declare this one with recursive false to avoid overhead
        rs.append(RequiredSubmodule(
            "CryptoPkg/Library/OpensslLib/openssl", False))

        # To avoid maintenance of this file for every new submodule
        # lets just parse the .gitmodules and add each if not already in list.
        # The GetRequiredSubmodules is designed to allow a build to optimize
        # the desired submodules but it isn't necessary for this repository.
        result = io.StringIO()
        ret = RunCmd("git", "config --file .gitmodules --get-regexp path", workingdir=self.GetWorkspaceRoot(), outstream=result)
        # Cmd output is expected to look like:
        # submodule.CryptoPkg/Library/OpensslLib/openssl.path CryptoPkg/Library/OpensslLib/openssl
        # submodule.SoftFloat.path ArmPkg/Library/ArmSoftFloatLib/berkeley-softfloat-3
        if ret == 0:
            for line in result.getvalue().splitlines():
                _, _, path = line.partition(" ")
                if path is not None:
                    if path not in [x.path for x in rs]:
                        rs.append(RequiredSubmodule(path, True)) # add it with recursive since we don't know
        return rs

    def SetArchitectures(self, list_of_requested_architectures):
        ''' Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise Exception if a list_of_requested_architectures is not supported
        '''
        unsupported = set(list_of_requested_architectures) - \
            set(self.GetArchitecturesSupported())
        if(len(unsupported) > 0):
            errorString = (
                "Unsupported Architecture Requested: " + " ".join(unsupported))
            logging.critical(errorString)
            raise Exception(errorString)
        self.ActualArchitectures = list_of_requested_architectures

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return CommonPlatform.WorkspaceRoot

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        return CommonPlatform.Scopes

    def FilterPackagesToTest(self, changedFilesList: list, potentialPackagesList: list) -> list:
        ''' Filter other cases that this package should be built
        based on changed files. This should cover things that can't
        be detected as dependencies. '''
        build_these_packages = []
        possible_packages = potentialPackagesList.copy()
        for f in changedFilesList:
            # BaseTools files that might change the build
            if "BaseTools" in f:
                if os.path.splitext(f) not in [".txt", ".md"]:
                    build_these_packages = possible_packages
                    break
            # if the azure pipeline platform template file changed
            if "platform-build-run-steps.yml" in f:
                build_these_packages = possible_packages
                break
        return build_these_packages

    def GetPlatformDscAndConfig(self) -> tuple:
        ''' If a platform desires to provide its DSC then Policy 4 will evaluate if
        any of the changes will be built in the dsc.

        The tuple should be (<workspace relative path to dsc file>, <input dictionary of dsc key value pairs>)
        '''
        return (os.path.join("EmulatorPkg", "EmulatorPkg.dsc"), {})

    # ####################################################################################### #
    #                         Actual Configuration for Platform Build                         #
    # ####################################################################################### #


class PlatformBuilder(UefiBuilder, BuildSettingsManager):
    def __init__(self):
        UefiBuilder.__init__(self)

    def AddCommandLineOptions(self, parserObj):
        ''' Add command line options to the argparser '''
        parserObj.add_argument('-a', "--arch", dest="build_arch", type=str, default="X64",
                               help="Optional - architecture to build.  IA32 will use IA32 for Pei & Dxe. "
                               "X64 will use X64 for both PEI and DXE.")

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''

        shell_environment.GetBuildVars().SetValue(
            "TARGET_ARCH", args.build_arch.upper(), "From CmdLine")
        shell_environment.GetBuildVars().SetValue(
            "ACTIVE_PLATFORM", "EmulatorPkg/EmulatorPkg.dsc", "From CmdLine")

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return CommonPlatform.WorkspaceRoot

    def GetPackagesPath(self):
        ''' Return a list of workspace relative paths that should be mapped as edk2 PackagesPath '''
        return ()

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        return CommonPlatform.Scopes

    def GetName(self):
        ''' Get the name of the repo, platform, or product being build '''
        ''' Used for naming the log file, among others '''

        # check the startup nsh flag and if set then rename the log file.
        # this helps in CI so we don't overwrite the build log since running
        # uses the stuart_build command.
        if(shell_environment.GetBuildVars().GetValue("MAKE_STARTUP_NSH", "FALSE") == "TRUE"):
            return "EmulatorPkg_With_Run"
        return "EmulatorPkg"

    def GetLoggingLevel(self, loggerType):
        ''' Get the logging level for a given type
        base == lowest logging level supported
        con  == Screen logging
        txt  == plain text file logging
        md   == markdown file logging
        '''
        return logging.DEBUG

    def SetPlatformEnv(self):
        logging.debug("PlatformBuilder SetPlatformEnv")
        self.env.SetValue("PRODUCT_NAME", "EmulatorPkg", "Platform Hardcoded")
        self.env.SetValue("TOOL_CHAIN_TAG", "VS2019", "Default Toolchain")

        # Add support for using the correct Platform Headers, tools, and Libs based on emulator architecture
        # requested to be built when building VS2019 or VS2017
        if self.env.GetValue("TOOL_CHAIN_TAG") == "VS2019" or self.env.GetValue("TOOL_CHAIN_TAG") == "VS2017":
            key = self.env.GetValue("TOOL_CHAIN_TAG") + "_HOST"
            if self.env.GetValue("TARGET_ARCH") == "IA32":
                shell_environment.ShellEnvironment().set_shell_var(key, "x86")
            elif self.env.GetValue("TARGET_ARCH") == "X64":
                shell_environment.ShellEnvironment().set_shell_var(key, "x64")

        # Add support for using the correct Platform Headers, tools, and Libs based on emulator architecture
        # requested to be built when building on linux.
        if GetHostInfo().os.upper() == "LINUX":
            self.ConfigureLinuxDLinkPath()

        if GetHostInfo().os.upper() == "WINDOWS":
            self.env.SetValue("BLD_*_WIN_HOST_BUILD", "TRUE",
                              "Trigger Windows host build")

        self.env.SetValue("MAKE_STARTUP_NSH", "FALSE", "Default to false")

        # I don't see what this does but it is in build.sh
        key = "BLD_*_BUILD_" + self.env.GetValue("TARGET_ARCH")
        self.env.SetValue(key, "TRUE", "match script in build.sh")
        return 0

    def PlatformPreBuild(self):
        return 0

    def PlatformPostBuild(self):
        return 0

    def FlashRomImage(self):
        ''' Use the FlashRom Function to run the emulator.  This gives an easy stuart command line to
        activate the emulator. '''

        OutputPath = os.path.join(self.env.GetValue(
            "BUILD_OUTPUT_BASE"), self.env.GetValue("TARGET_ARCH"))

        if (self.env.GetValue("MAKE_STARTUP_NSH") == "TRUE"):
            f = open(os.path.join(OutputPath, "startup.nsh"), "w")
            f.write("BOOT SUCCESS !!! \n")
            # add commands here
            f.write("reset\n")
            f.close()

        if GetHostInfo().os.upper() == "WINDOWS":
            cmd = "WinHost.exe"
        elif GetHostInfo().os.upper() == "LINUX":
            cmd = "./Host"
        else:
            logging.critical("Unsupported Host")
            return -1
        return RunCmd(cmd, "", workingdir=OutputPath)

    def ConfigureLinuxDLinkPath(self):
        '''
        logic copied from build.sh to setup the correct libraries
        '''
        if self.env.GetValue("TARGET_ARCH") == "IA32":
            LIB_NAMES = ["ld-linux.so.2", "libdl.so.2 crt1.o", "crti.o crtn.o"]
            LIB_SEARCH_PATHS = ["/usr/lib/i386-linux-gnu",
                                "/usr/lib32", "/lib32", "/usr/lib", "/lib"]
        elif self.env.GetValue("TARGET_ARCH") == "X64":
            LIB_NAMES = ["ld-linux-x86-64.so.2",
                         "libdl.so.2", "crt1.o", "crti.o", "crtn.o"]
            LIB_SEARCH_PATHS = ["/usr/lib/x86_64-linux-gnu",
                                "/usr/lib64", "/lib64", "/usr/lib", "/lib"]

        HOST_DLINK_PATHS = ""
        for lname in LIB_NAMES:
            logging.debug(f"Looking for {lname}")
            for dname in LIB_SEARCH_PATHS:
                logging.debug(f"In {dname}")
                if os.path.isfile(os.path.join(dname, lname)):
                    logging.debug(f"Found {lname} in {dname}")
                    HOST_DLINK_PATHS += os.path.join(
                        os.path.join(dname, lname)) + os.pathsep
                    break
        HOST_DLINK_PATHS = HOST_DLINK_PATHS.rstrip(os.pathsep)
        logging.critical(f"Setting HOST_DLINK_PATHS to {HOST_DLINK_PATHS}")
        shell_environment.ShellEnvironment().set_shell_var(
            "HOST_DLINK_PATHS", HOST_DLINK_PATHS)
