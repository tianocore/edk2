# @file
# Script to Build ArmVirtPkg UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging

from edk2toolext.environment import shell_environment
from edk2toolext.environment.uefi_build import UefiBuilder
from edk2toolext.invocables.edk2_platform_build import BuildSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toollib.utility_functions import RunCmd
from edk2toollib.utility_functions import GetHostInfo


    # ####################################################################################### #
    #                                Common Configuration                                     #
    # ####################################################################################### #
class CommonPlatform():
    ''' Common settings for this platform.  Define static data here and use
        for the different parts of stuart
    '''
    PackagesSupported = ("ArmVirtPkg",)
    ArchSupported = ("AARCH64", "ARM")
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('armvirt', 'edk2-build')
    WorkspaceRoot = os.path.realpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), ".."))


    # ####################################################################################### #
    #                         Configuration for Update & Setup                                #
    # ####################################################################################### #
class SettingsManager(UpdateSettingsManager, SetupSettingsManager):

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
        rs.append(RequiredSubmodule(
            "ArmPkg/Library/ArmSoftFloatLib/berkeley-softfloat-3", False))
        rs.append(RequiredSubmodule(
            "CryptoPkg/Library/OpensslLib/openssl", False))
        return rs

    def SetArchitectures(self, list_of_requested_architectures):
        ''' Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise Exception if a list_of_requested_architectures is not supported
        '''
        unsupported = set(list_of_requested_architectures) - set(self.GetArchitecturesSupported())
        if(len(unsupported) > 0):
            errorString = ( "Unsupported Architecture Requested: " + " ".join(unsupported))
            logging.critical( errorString )
            raise Exception( errorString )
        self.ActualArchitectures = list_of_requested_architectures

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return CommonPlatform.WorkspaceRoot

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''

        scopes = CommonPlatform.Scopes
        ActualToolChainTag = shell_environment.GetBuildVars().GetValue("TOOL_CHAIN_TAG", "")

        if GetHostInfo().os.upper() == "LINUX" and ActualToolChainTag.upper().startswith("GCC"):
            if "AARCH64" in self.ActualArchitectures:
                scopes += ("gcc_aarch64_linux",)
            if "ARM" in self.ActualArchitectures:
                scopes += ("gcc_arm_linux",)
        return scopes


    # ####################################################################################### #
    #                         Actual Configuration for Platform Build                         #
    # ####################################################################################### #
class PlatformBuilder( UefiBuilder, BuildSettingsManager):
    def __init__(self):
        UefiBuilder.__init__(self)

    def AddCommandLineOptions(self, parserObj):
        ''' Add command line options to the argparser '''
        parserObj.add_argument('-a', "--arch", dest="build_arch", type=str, default="AARCH64",
            help="Optional - Architecture to build.  Default = AARCH64")

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''

        shell_environment.GetBuildVars().SetValue("TARGET_ARCH", args.build_arch.upper(), "From CmdLine")
        shell_environment.GetBuildVars().SetValue("BLD_*_ARCH", args.build_arch.upper(), "From CmdLine")

        shell_environment.GetBuildVars().SetValue("ACTIVE_PLATFORM", "ArmVirtPkg/ArmVirtQemu.dsc", "From CmdLine")

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return CommonPlatform.WorkspaceRoot

    def GetPackagesPath(self):
        ''' Return a list of workspace relative paths that should be mapped as edk2 PackagesPath '''
        return ()

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        scopes = CommonPlatform.Scopes
        ActualToolChainTag = shell_environment.GetBuildVars().GetValue("TOOL_CHAIN_TAG", "")
        Arch = shell_environment.GetBuildVars().GetValue("TARGET_ARCH", "")

        if GetHostInfo().os.upper() == "LINUX" and ActualToolChainTag.upper().startswith("GCC"):
            if "AARCH64" == Arch:
                scopes += ("gcc_aarch64_linux",)
            elif "ARM" == Arch:
                scopes += ("gcc_arm_linux",)
        return scopes

    def GetName(self):
        ''' Get the name of the repo, platform, or product being build '''
        ''' Used for naming the log file, among others '''
        # check the startup nsh flag and if set then rename the log file.
        # this helps in CI so we don't overwrite the build log since running
        # uses the stuart_build command.
        if(shell_environment.GetBuildVars().GetValue("MAKE_STARTUP_NSH", "FALSE") == "TRUE"):
            return "ArmVirtPkg_With_Run"
        return "ArmVirtPkg"

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
        self.env.SetValue("PRODUCT_NAME", "ArmVirtQemu", "Platform Hardcoded")
        self.env.SetValue("MAKE_STARTUP_NSH", "FALSE", "Default to false")
        self.env.SetValue("QEMU_HEADLESS", "FALSE", "Default to false")
        return 0

    def PlatformPreBuild(self):
        return 0

    def PlatformPostBuild(self):
        return 0

    def FlashRomImage(self):
        VirtualDrive = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "VirtualDrive")
        os.makedirs(VirtualDrive, exist_ok=True)
        OutputPath_FV = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "FV")
        Built_FV = os.path.join(OutputPath_FV, "QEMU_EFI.fd")


        # should move into plugin since Qemu can be used by lots of
        # platforms.  Issue is --FlashOnly doesn't run prebuild and thus plugin is skipped
        # Discuss this with PyTool project
        if os.path.isdir("\\Program Files\\qemu"):
            shell_environment.GetEnvironment().append_path(os.path.abspath("\\Program Files\\qemu"))
        else:
            logging.critical("QEMU folder not found in program Files")

        # pad fd to 64mb
        with open(Built_FV, "ab") as fvfile:
            fvfile.seek(0, os.SEEK_END)
            additional = b'\0' * ((64 * 1024 * 1024)-fvfile.tell())
            fvfile.write(additional)

        if (self.env.GetValue("TARGET_ARCH").upper() == "AARCH64"):
            cmd = "qemu-system-aarch64"
            args  = "-M virt"
            args += " -cpu cortex-a57"                                          # emulate cpu
            args += " -pflash " +  Built_FV                                     # path to fw
            args += " -m 1024"                                                  # 1gb memory
            args += " -net none"                                                # turn off network
            args += " -serial stdio"                                            # Serial messages out
            args += f" -drive file=fat:rw:{VirtualDrive},format=raw,media=disk" # Mount disk with startup.nsh
        else:
            raise NotImplementedError()

        if (self.env.GetValue("QEMU_HEADLESS").upper() == "TRUE"):
            args += " -display none"  # no graphics

        if (self.env.GetValue("MAKE_STARTUP_NSH").upper() == "TRUE"):
            f = open(os.path.join(VirtualDrive, "startup.nsh"), "w")
            f.write("BOOT SUCCESS !!! \n")
            ## add commands here
            f.write("reset -s\n")
            f.close()

        ret = RunCmd(cmd, args)

        if ret == 0xc0000005:
            #for some reason getting a c0000005 on successful return
            return 0

        return ret



