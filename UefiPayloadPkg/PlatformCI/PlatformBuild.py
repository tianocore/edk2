# @file
# Script to Build UefiPayloadPkg UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# Copyright (c) 2025 Intel Corporation
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
import io
import sys

from edk2toolext.environment import shell_environment
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext.invocables.edk2_platform_build import BuildSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import GetHostInfo
sys.path.append("./UefiPayloadPkg")
from UniversalPayloadBuild import UniversalPayloadFullBuild, InitArgumentParser

# ####################################################################################### #
#                                Common Configuration                                     #
# ####################################################################################### #


class CommonPlatform():
    ''' Common settings for this platform.  Define static data here and use
        for the different parts of stuart
    '''
    PackagesSupported = ("UefiPayloadPkg",)
    ArchSupported = ("X64", "IA32")
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('uefipayloadpkg', 'edk2-build')
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
        return (os.path.join("UefiPayloadPkg", "UefiPayloadPkg.dsc"), {})

    # ####################################################################################### #
    #                         Actual Configuration for Platform Build                         #
    # ####################################################################################### #


class PlatformBuilder(UefiBuilder, BuildSettingsManager):
    def __init__(self):
        UefiBuilder.__init__(self)
        self.args = None

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
            "ACTIVE_PLATFORM", "UefiPayloadPkg/UefiPayloadPkg.dsc", "From CmdLine")
        self.args = args


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
            return "UefiPayloadPkg_With_Run"
        return "UefiPayloadPkg"

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
        self.env.SetValue("PRODUCT_NAME", "UefiPayloadPkg", "Platform Hardcoded")
        self.env.SetValue("TOOL_CHAIN_TAG", "VS2022", "Default Toolchain")

        # Add support for using the correct Platform Headers, tools, and Libs based on architecture
        # requested to be built when building VS2022 or VS2019
        if self.env.GetValue("TOOL_CHAIN_TAG").startswith ('VS'):
            key = self.env.GetValue("TOOL_CHAIN_TAG") + "_HOST"
            if self.env.GetValue("TARGET_ARCH") == "IA32":
                shell_environment.ShellEnvironment().set_shell_var(key, "x86")
            elif self.env.GetValue("TARGET_ARCH") == "X64":
                shell_environment.ShellEnvironment().set_shell_var(key, "x64")
        if os.name == 'nt':
            shell_environment.ShellEnvironment().set_shell_var("CLANG_HOST_BIN", "n")
        self.env.SetValue ("BLD_*_BUILD_ARCH", self.args.build_arch, "build_arch")
        self.env.SetValue ("BLD_*_UNIVERSAL_PAYLOAD", "TRUE", "build UPL")

        return 0

    def PlatformPreBuild(self):
        return 0

    def PlatformPostBuild(self):
        #
        # Additional UPL build step to generate ELF entry and merge with common build FD.
        #
        default_args = InitArgumentParser(True)
        default_args.Arch = self.args.build_arch
        default_args.ToolChain = self.env.GetValue("TOOL_CHAIN_TAG")
        default_args.Fit = self.env.GetValue("FIT_BUILD", "FALSE") != "FALSE"
        default_args.CiBuild = True
        return UniversalPayloadFullBuild(default_args)
