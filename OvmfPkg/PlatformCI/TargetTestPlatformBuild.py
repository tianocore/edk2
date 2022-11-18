# @file
# Script to Build OVMF UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
from asyncio.windows_events import NULL
import shutil
import os
import sys
import logging
import argparse
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

        PackagesSupported = ("OvmfPkg", "UefiCpuPkg", "MdeModulePkg")
    '''
    PackagesSupported = ("OvmfPkg",)
    ArchSupported = ("IA32", "X64")
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('ovmf', 'edk2-build')
    WorkspaceRoot = os.path.realpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".."))

    @classmethod
    def GetDscName(cls, ArchCsv: str) -> str:
        ''' return the DSC given the architectures requested.

        ArchCsv: csv string containing all architectures to build
        
        dsc = "OvmfPkg"
        if "IA32" in ArchCsv.upper().split(","):
            dsc += "Ia32"
        if "X64" in ArchCsv.upper().split(","):
            dsc += "X64"
        dsc += ".dsc"
        return dsc
        '''
        return "TargetUnitTest/OvmfPkgTargetTest.dsc"

class UnitTestSupport():
    ''' Common settings for this platform.  Define static data here and use
        for the different parts of stuart
    
    UnitTestModule    = {'UefiCpuPkg/Library/CpuExceptionHandlerLib/UnitTest/DxeCpuExceptionHandlerLibUnitTest.inf':'UefiCpuPkg.dsc',
                         'MdeModulePkg/Application/HelloWorld/HelloWorld.inf'                                      :'MdeModulePkg.dsc'
                        }
    PackagesToBuild   = ["UefiCpuPkg", "MdeModulePkg"]
    '''
    UnitTestModule  = {}
    PackagesToBuild = []

    def UpdateUnitTestConfig(self, args):
        self.env = shell_environment.GetBuildVars()
        '''Update UnitTestModule, PackagesSupported and PackagesToBuild in UnitTestSupport class'''
        UnitTestModuleList = ';'.join(args.ShellUnitTestList).split(";")
        if not UnitTestModuleList:
            return
        PackagesSupported = list(CommonPlatform.PackagesSupported)
        for Module in UnitTestModuleList:
            PkgName = Module.split("Pkg")[0] + 'Pkg'
            if PkgName not in UnitTestSupport.UnitTestModule.keys():
                UnitTestSupport.UnitTestModule[Module] = PkgName + '.dsc'
            if PkgName not in PackagesSupported:
                PackagesSupported.append(PkgName)
        CommonPlatform.PackagesSupported = tuple(PackagesSupported)
        print('PackagesSupported is {}'.format(CommonPlatform.PackagesSupported))
        print('UnitTestModule    is {}'.format(UnitTestSupport.UnitTestModule))
        try:
            PackagesToBuild = ','.join(args.PackagesForUT).split(',')
            if PackagesToBuild:
                UnitTestSupport.PackagesToBuild = PackagesToBuild
                print('PackagesToBuild is {}'.format(UnitTestSupport.PackagesToBuild))
        except:
            pass

    def BuildUnitTest(self, packageList):
        VirtualDrive = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "VirtualDrive")
        BuiltDsc = []
        for package in packageList:
            print(package)
            for Module, Dsc in UnitTestSupport.UnitTestModule.items():
                if package in Dsc:
                    if Dsc not in BuiltDsc:
                        ModuleName = os.path.normpath(Module).split('.inf')[0].rsplit('\\')[-1]
                        BuiltDsc.append(Dsc)
                        DscPath = os.path.join(package, Dsc)
                        # Build specific dsc for UnitTest modules
                        print('Going to build this {0} for {1}'.format(DscPath, ModuleName))
                        Arch = self.env.GetValue("TARGET_ARCH").split(" ")
                        # Set the Unit Test arch the same as the Shell in Ovmf.
                        if 'X64' in Arch:
                            UTArch = 'X64'
                        else:
                            UTArch = 'IA32'
                        self.env.AllowOverride("ACTIVE_PLATFORM")
                        self.env.SetValue("ACTIVE_PLATFORM", DscPath, "Test")
                        self.env.AllowOverride("TARGET_ARCH")
                        self.env.SetValue("TARGET_ARCH", UTArch, "Test")
                        PlatformBuilder.Build(self)
                    # Copy the UnitTest efi files to VirtualDrive folder
                    EfiPath = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), UTArch, Module.split('.inf')[0], self.env.GetValue("TARGET"), ModuleName + '.efi')
                    print('Copy {0}.efi from:{1}'.format(ModuleName, EfiPath))
                    shutil.copy(EfiPath, VirtualDrive)

    def WriteEfiToStartup(self, Folder):
        ''' Write all the .efi files' name in VirtualDrive into Startup.nsh'''
        if (self.env.GetValue("MAKE_STARTUP_NSH").upper() == "TRUE"):
            f = open(os.path.join(Folder, "startup.nsh"), "w")
            for root,dirs,files in os.walk(Folder):
                for file in files:
                    print(file)
                    print(os.path.splitext(file)[1])
                    if os.path.splitext(file)[1] == '.efi':
                        f.write("{0} \n".format(file))
            f.write("reset -s\n")
            f.close()

    def CheckBootLog(self):
        #
        # Find all FAILURE MESSAGE in boot log
        #
        #BuildLog = "BUILDLOG_{0}.txt".format(PlatformBuilder.GetName(self))
        #LogPath  = os.path.join(self.ws, 'Build', BuildLog)
        LogPath = 'E:/code/FutureCoreRpArraw/RomImages/fail.log'
        print('Checking the boot log: {0}'.format(LogPath))
        file        = open(LogPath, "r")
        fileContent = file.readlines()
        for Index in range(len(fileContent)):
            if 'FAILURE MESSAGE:' in fileContent[Index]:
                if fileContent[Index + 1].strip() != '':
                    FailureMessage = fileContent[Index + 1] + fileContent[Index + 2]
                    print(FailureMessage)
                    return FailureMessage
        return 0

import PlatformBuildLib
UnitTestUnitTestConfig = UnitTestSupport()
PlatformBuildLib.CommonPlatform = CommonPlatform

class UnitTestSettingsManager(SettingsManager):
    def GetPlatformDscAndConfig(self) -> tuple:
        return None
    def AddCommandLineOptions(self, parserObj):
        print('AddCommandLineOptions in PrSettingsManager')
        parserObj.add_argument('ShellUnitTestList', type=str,
                               help='Optional - A package or folder you want to update (workspace relative).'
                               'Can list multiple by doing -p <pkg1>,<pkg2> or -p <pkg3> -p <pkg4>',
                               action="append", default=[])

    def RetrieveCommandLineOptions(self, args):
        print('RetrieveCommandLineOptions in PrSettingsManager')
        UnitTestUnitTestConfig.UpdateUnitTestConfig(args)

class UnitTestPlatformBuilder(PlatformBuilder):
    def AddCommandLineOptions(self, parserObj):
        ''' Add command line options to the argparser '''
        print('AddCommandLineOptions in PlatformBuilder')
        PlatformBuilder.AddCommandLineOptions(self, parserObj)
        parserObj.add_argument('-p', '--pkg', '--pkg-dir', dest='PackagesForUT', type=str,
                               help='Optional - A package or folder you want to update (workspace relative).'
                               'Can list multiple by doing -p <pkg1>,<pkg2> or -p <pkg3> -p <pkg4>',
                               action="append", default=[])
        parserObj.add_argument('ShellUnitTestList', type=str,
                               help='Optional - A package or folder you want to update (workspace relative).'
                               'Can list multiple by doing -p <pkg1>,<pkg2> or -p <pkg3> -p <pkg4>',
                               action="append", default=['None'])

    def RetrieveCommandLineOptions(self, args):
        '''  Retrieve command line options from the argparser '''
        print('RetrieveCommandLineOptions in PlatformBuilder')
        PlatformBuilder.RetrieveCommandLineOptions(self, args)
        UnitTestUnitTestConfig.UpdateUnitTestConfig(args)

    def PlatformPostBuild(self):
        ''' Build specific Pkg in command line for UnitTest modules.'''
        UnitTestUnitTestConfig.BuildUnitTest(UnitTestSupport.PackagesToBuild)
        VirtualDrive = os.path.join(self.env.GetValue("BUILD_OUTPUT_BASE"), "VirtualDrive")
        UnitTestUnitTestConfig.WriteEfiToStartup(VirtualDrive)
        return 0

    def FlashRomImage(self):
        PlatformBuilder.FlashRomImage(self)
        UnitTestResult = UnitTestUnitTestConfig.CheckBootLog()
        if (UnitTestResult):
            logging.info("UnitTest failed with this FAILURE MESSAGE:\n{}".format(UnitTestResult))
            return UnitTestResult
        return 0
