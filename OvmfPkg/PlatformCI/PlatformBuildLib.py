# @file
# Script to Build OVMF UEFI firmware
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

    def AddCommandLineOptions(self, parserObj):
        parserObj.add_argument('-u', '--UnitTest', dest='ShellUnitTestList', type=str,
            help='Optional - Key:Value that contains Shell UnitTest list and corresponding DscPath you want to test.(workspace relative)'
            'Can list multiple by doing -u <UTPath1:DscPath1>,<UTPath2:DscPath2> or -u <UTPath3:DscPath3> -u <UTPath4:DscPath4>',
            action="append", default=None)

    def RetrieveCommandLineOptions(self, args):
        if args.ShellUnitTestList:
            CommonPlatform.UpdatePackagesSupported(args.ShellUnitTestList)

    def GetPlatformDscAndConfig(self) -> tuple:
        ''' If a platform desires to provide its DSC then Policy 4 will evaluate if
        any of the changes will be built in the dsc.

        The tuple should be (<workspace relative path to dsc file>, <input dictionary of dsc key value pairs>)
        This Policy 4 can only be applied when PackagesSupported only contains OvmfPkg Since it doesn't support
        mutiple packages evaluation.
        '''
        if (len(CommonPlatform.PackagesSupported) == 1) and (CommonPlatform.PackagesSupported[0] == 'OvmfPkg'):
            dsc = CommonPlatform.GetDscName(",".join(self.ActualArchitectures))
            return (f"OvmfPkg/{dsc}", {})
        return None

    # ####################################################################################### #
    #                         Actual Configuration for Platform Build                         #
    # ####################################################################################### #
class PlatformBuilder( UefiBuilder, BuildSettingsManager):
    def __init__(self):
        UefiBuilder.__init__(self)

    def AddCommandLineOptions(self, parserObj):
        ''' Add command line options to the argparser '''
        parserObj.add_argument('-a', "--arch", dest="build_arch", type=str, default="IA32,X64",
            help="Optional - CSV of architecture to build.  IA32 will use IA32 for Pei & Dxe. "
            "X64 will use X64 for both PEI and DXE.  IA32,X64 will use IA32 for PEI and "
            "X64 for DXE. default is IA32,X64")
        parserObj.add_argument('-p', '--pkg', '--pkg-dir', dest='PkgsToBuildForUT', type=str,
            help='Optional - Package list you want to build for UnitTest.efi. (workspace relative).'
            'Can list multiple by doing -p <pkg1>,<pkg2> or -p <pkg3> -p <pkg4>.If no valid input -p, build and run all -u UnitTest',
            action="append", default=None)
        parserObj.add_argument('-u', '--UnitTest', dest='ShellUnitTestList', type=str,
            help='Optional - Key:Value that contains Shell UnitTest list and corresponding DscPath you want to test.(workspace relative)'
            'Can list multiple by doing -u <UTPath1:DscPath1>,<UTPath2:DscPath2> or -u <UTPath3:DscPath3> -u <UTPath4:DscPath4>',
            action="append", default=None)

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''

        shell_environment.GetBuildVars().SetValue("TARGET_ARCH"," ".join(args.build_arch.upper().split(",")), "From CmdLine")
        dsc = CommonPlatform.GetDscName(args.build_arch)
        shell_environment.GetBuildVars().SetValue("ACTIVE_PLATFORM", f"OvmfPkg/{dsc}", "From CmdLine")
        self.RunShellUnitTest = False
        if args.ShellUnitTestList:
            CommonPlatform.UpdateUnitTestConfig(args)
            self.RunShellUnitTest = CommonPlatform.RunShellUnitTest

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
            return "OvmfPkg_With_Run"
        return "OvmfPkg"

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
        self.env.SetValue("PRODUCT_NAME", "OVMF", "Platform Hardcoded")
        self.env.SetValue("MAKE_STARTUP_NSH", "FALSE", "Default to false")
        self.env.SetValue("QEMU_HEADLESS", "FALSE", "Default to false")
        return 0

    def PlatformPreBuild(self):
        return 0

    def PlatformPostBuild(self):
        if self.RunShellUnitTest:
            ret = CommonPlatform.BuildUnitTest(self)
            if ret !=0:
                logging.critical("Build UnitTest failed")
                return ret
        return 0

    def FlashRomImage(self):
        VirtualDrive = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "VirtualDrive")
        os.makedirs(VirtualDrive, exist_ok=True)
        OutputPath_FV = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "FV")

        if (self.env.GetValue("QEMU_SKIP") and
            self.env.GetValue("QEMU_SKIP").upper() == "TRUE"):
            logging.info("skipping qemu boot test")
            return 0

        #
        # QEMU must be on the path
        #
        cmd = "qemu-system-x86_64"
        args  = "-debugcon stdio"                                           # write messages to stdio
        args += " -global isa-debugcon.iobase=0x402"                        # debug messages out thru virtual io port
        args += " -net none"                                                # turn off network
        args += f" -drive file=fat:rw:{VirtualDrive},format=raw,media=disk" # Mount disk with startup.nsh

        if (self.env.GetValue("QEMU_HEADLESS").upper() == "TRUE"):
            args += " -display none"  # no graphics

        if (self.env.GetBuildValue("SMM_REQUIRE") == "1"):
            args += " -machine q35,smm=on" #,accel=(tcg|kvm)"
            #args += " -m ..."
            #args += " -smp ..."
            args += " -global driver=cfi.pflash01,property=secure,value=on"
            args += " -drive if=pflash,format=raw,unit=0,file=" + os.path.join(OutputPath_FV, "OVMF_CODE.fd") + ",readonly=on"
            args += " -drive if=pflash,format=raw,unit=1,file=" + os.path.join(OutputPath_FV, "OVMF_VARS.fd")
        else:
            args += " -pflash " + os.path.join(OutputPath_FV, "OVMF.fd")    # path to firmware

        if (self.env.GetValue("MAKE_STARTUP_NSH").upper() == "TRUE"):
            f = open(os.path.join(VirtualDrive, "startup.nsh"), "w")
            if self.RunShellUnitTest:
                # When RunShellUnitTest is True, write all efi files name into startup.nsh.
                CommonPlatform.WriteEfiToStartup(VirtualDrive, f)
                # Output UnitTest log into ShellUnitTestLog.
                args += " -serial file:{}".format(CommonPlatform.ShellUnitTestLog)
            f.write("BOOT SUCCESS !!! \n")
            ## add commands here
            f.write("reset -s\n")
            f.close()

        ret = RunCmd(cmd, args)

        if ret == 0xc0000005:
            #for some reason getting a c0000005 on successful return
            ret = 0
        if self.RunShellUnitTest and ret == 0:
            # Check the UnitTest boot log.
            UnitTestResult = CommonPlatform.CheckUnitTestLog()
            if (UnitTestResult):
                logging.info("UnitTest failed with this FAILURE MESSAGE:\n{}".format(UnitTestResult))
                return UnitTestResult
        return ret
