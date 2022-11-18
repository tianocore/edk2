# @file
# Script to Build OVMF UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import shutil
import logging
from edk2toollib.utility_functions import RunCmd
from edk2toolext.environment import shell_environment

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from PlatformBuildLib import SettingsManager
from PlatformBuildLib import PlatformBuilder

    # ####################################################################################### #
    #                                Common Configuration                                     #
    # ####################################################################################### #
class CommonPlatform():
    ''' Common settings for this platform.  Define static data here and use
        for the different parts of stuart
    '''
    PackagesSupported = ("OvmfPkg",)
    ArchSupported = ("IA32", "X64")
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('ovmf', 'edk2-build')
    WorkspaceRoot = os.path.realpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".."))

    def __init__(self):
        '''  When the -u and -p input is not null, the self.PackagesSupported and self.UnitTestModuleList will be updated to following format:
        self.PackagesSupported  = ("UefiCpuPkg", "MdeModulePkg")
        self.UnitTestModuleList = {'UefiCpuPkg/Library/CpuExceptionHandlerLib/UnitTest/DxeCpuExceptionHandlerLibUnitTest.inf':'UefiCpuPkg.dsc',
                                   'MdeModulePkg/Application/HelloWorld/HelloWorld.inf'                                      :'MdeModulePkg.dsc'}
        '''
        self.PackagesSupported  = ()
        self.UnitTestModuleList = {}
        self.RunShellUnitTest   = False
        self.ShellUnitTestLog   = None

    @classmethod
    def GetDscName(cls, ArchCsv: str) -> str:
        ''' return the DSC given the architectures requested.

        ArchCsv: csv string containing all architectures to build
        '''
        dsc = "OvmfPkg"
        if "IA32" in ArchCsv.upper().split(","):
            dsc += "Ia32"
        if "X64" in ArchCsv.upper().split(","):
            dsc += "X64"
        dsc += ".dsc"
        return dsc

    def UpdatePackagesSupported(self, args):
        ''' Update PackagesSupported by -u ShellUnitTestList from cmd line. '''
        UnitTestModuleList = ','.join(args.ShellUnitTestList).split(",")
        if not UnitTestModuleList:
            return
        PackagesSupported = []
        for KeyValue in UnitTestModuleList:
            PkgName = KeyValue.split("Pkg")[0] + 'Pkg'
            if PkgName not in PackagesSupported:
                PackagesSupported.append(PkgName)
        self.PackagesSupported = tuple(PackagesSupported)
        print(args.ShellUnitTestList)
        print('PackagesSupported for UnitTest is {}'.format(self.PackagesSupported))

    def UpdateUnitTestToBuild(self, args):
        ''' Update UnitTestModuleList by -u ShellUnitTestList and -p PkgsToBuild from cmd line.
            ShellUnitTestList is in this format: {module1:dsc1, module2:dsc2, module3:dsc2...}.
            Only the modules which are in the PkgsToBuild list are added into self.UnitTestModuleList.
        '''
        UnitTestModuleList = ','.join(args.ShellUnitTestList).split(",")
        PkgsToBuild        = ','.join(args.PkgsToBuild).split(',')
        if not UnitTestModuleList or not PkgsToBuild:
            return

        for KeyValue in UnitTestModuleList:
            UnitTestPath = os.path.normpath(KeyValue.split(":")[0])
            DscPath      = os.path.normpath(KeyValue.split(":")[1])
            PkgName      = UnitTestPath.split("Pkg")[0] + 'Pkg'
            if PkgName in PkgsToBuild:
                self.UnitTestModuleList[UnitTestPath] = DscPath
        if self.UnitTestModuleList:
            self.RunShellUnitTest = True
            self.ShellUnitTestLog = os.path.join(PlatformBuilder.GetWorkspaceRoot(self), 'Build',
               "BUILDLOG_{0}_UnitTest.txt".format(PlatformBuilder.GetName(self)))

        print('UnitTestModuleList is {}'.format(self.UnitTestModuleList))

    def BuildUnitTest(self):
        self.env = shell_environment.GetBuildVars()
        self.VirtualDrive = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "VirtualDrive")
        # DSC by self.GetDscName() should have been built in BUILD process.
        BuiltDsc = [self.GetDscName(",".join(self.env.GetValue("TARGET_ARCH").split(' ')))]
        for UnitTestPath, DscPath in self.UnitTestModuleList.items():
            if DscPath not in BuiltDsc:
                ModuleName = UnitTestPath.split('.inf')[0].rsplit('\\')[-1]
                logging.info('Build {0} for {1}'.format(DscPath, ModuleName))
                BuiltDsc.append(DscPath)
                Arch = self.env.GetValue("TARGET_ARCH").split(" ")
                if 'X64' in Arch:
                    UTArch = 'X64'
                else:
                    UTArch = 'IA32'
                self.env.AllowOverride("ACTIVE_PLATFORM")
                self.env.SetValue("ACTIVE_PLATFORM", DscPath, "Test")
                self.env.AllowOverride("TARGET_ARCH")
                self.env.SetValue("TARGET_ARCH", UTArch, "Test") # Set UnitTest arch the same as Ovmf Shell module.
                ret = PlatformBuilder.Build(self)                # Build specific dsc for UnitTest modules
                if (ret != 0):
                    return ret
            EfiPath = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), UTArch,
                UnitTestPath.split('.inf')[0],"OUTPUT", ModuleName + '.efi')
            print('Copy {0}.efi from:{1}'.format(ModuleName, EfiPath))
            shutil.copy(EfiPath, self.VirtualDrive)
        return 0

    def WriteEfiToStartup(self):
        ''' Write all the .efi files' name in VirtualDrive into Startup.nsh '''
        f = open(os.path.join(self.VirtualDrive, "startup.nsh"), "w")
        for root,dirs,files in os.walk(self.VirtualDrive):
            for file in files:
                if os.path.splitext(file)[1] == '.efi':
                    f.write("{0} \n".format(file))
        f.close()

    def CheckUnitTestLog(self):
        LogPath     = self.ShellUnitTestLog
        file        = open(LogPath, "r")
        fileContent = file.readlines()
        logging.info('Check the UnitTest boot log:{0}'.format(LogPath))
        for Index in range(len(fileContent)):
            if 'FAILURE MESSAGE:' in fileContent[Index]:
                if fileContent[Index + 1].strip() != '':
                    FailureMessage = fileContent[Index + 1] + fileContent[Index + 2]
                    return FailureMessage
        return 0

import PlatformBuildLib
CommonPlatformSupportUT = CommonPlatform()
PlatformBuildLib.CommonPlatform = CommonPlatformSupportUT

class UnitTestSettingsManager(SettingsManager):
    def AddCommandLineOptions(self, parserObj):
        parserObj.add_argument('-u', '--UnitTest', dest='ShellUnitTestList', type=str,
                               help='Optional - Key:Value that contains Shell UnitTest list and corresponding DscPath you want to check.'
                               'Can list multiple by doing -u <UTPath1:DscPath1>,<UTPath2:DscPath2> or -p <UTPath3:DscPath3> -p <UTPath4:DscPath4>',
                               action="append", default=[])

    def RetrieveCommandLineOptions(self, args):
        CommonPlatformSupportUT.UpdatePackagesSupported(args)

    def GetPlatformDscAndConfig(self) -> tuple:
        ''' If a platform desires to provide its DSC then Policy 4 will evaluate if
        any of the changes will be built in the dsc.

        The tuple should be (<workspace relative path to dsc file>, <input dictionary of dsc key value pairs>)
        This rule can only be applied when we don't need to run UnitTest in this Ovmf PlatformCI.
        '''
        if (CommonPlatformSupportUT.PackagesSupported == CommonPlatform.PackagesSupported):
            super().GetPlatformDscAndConfig()
        return None

class UnitTestPlatformBuilder(PlatformBuilder):
    def AddCommandLineOptions(self, parserObj):
        ''' Add command line options to the argparser '''
        super().AddCommandLineOptions(parserObj)
        parserObj.add_argument('-p', '--pkg', '--pkg-dir', dest='PkgsToBuild', type=str,
                               help='Optional - Package list you want to build for UnitTest.efi. (workspace relative).'
                               'Can list multiple by doing -p <pkg1>,<pkg2> or -p <pkg3> -p <pkg4>',
                               action="append", default=[])
        parserObj.add_argument('-u', '--UnitTest', dest='ShellUnitTestList', type=str,
                               help='Optional - Key:Value that contains Shell UnitTest list and corresponding DscPath you want to check.'
                               'Can list multiple by doing -u <UTPath1:DscPath1>,<UTPath2:DscPath2> or -p <UTPath3:DscPath3> -p <UTPath4:DscPath4>',
                               action="append", default=[])

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''
        super().RetrieveCommandLineOptions(args)
        CommonPlatformSupportUT.UpdateUnitTestToBuild(args)

    def PlatformPostBuild(self):
        ''' Build specific Pkg in command line for UnitTest modules.'''
        if CommonPlatformSupportUT.RunShellUnitTest:
            ret = CommonPlatformSupportUT.BuildUnitTest()
            CommonPlatformSupportUT.WriteEfiToStartup()
            return ret
        return 0

    def FlashRomImage(self):
        ret = super().FlashRomImage()
        if CommonPlatformSupportUT.RunShellUnitTest and ret == 0:
            # Check the UnitTest boot log.
            UnitTestResult = CommonPlatformSupportUT.CheckUnitTestLog()
            if (UnitTestResult):
                logging.info("UnitTest failed with this FAILURE MESSAGE:\n{}".format(UnitTestResult))
                return UnitTestResult
        return ret
