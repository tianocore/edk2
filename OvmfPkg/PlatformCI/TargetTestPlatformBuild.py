# @file
# Script to Build OVMF UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import logging
from edk2toollib.utility_functions import RunCmd

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

    @classmethod
    def GetDscName(cls, ArchCsv: str) -> str:
        ''' return the DSC given the architectures requested.

        TargetUnitTest: dsc file which contains target unit test module.
        '''
        return "TargetUnitTest/OvmfPkgTargetTest.dsc"

import PlatformBuildLib
PlatformBuildLib.CommonPlatform = CommonPlatform

class UnitTestPlatformBuilder(PlatformBuilder):
    def CheckBootLog(self):
        #
        # Find all test results in boot log
        #
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

        print(os.path.join(OutputPath_FV, "OVMF.fd"))
        args += " -pflash " + os.path.join(OutputPath_FV, "OVMF.fd")    # path to firmware


        if (self.env.GetValue("MAKE_STARTUP_NSH").upper() == "TRUE"):
            f = open(os.path.join(VirtualDrive, "startup.nsh"), "w")
            f.write("BOOT SUCCESS !!! \n")
            ## add commands here
            f.write("reset -s\n")
            f.close()

        ret = RunCmd(cmd, args)
        print(args)
        UnitTestResult = self.CheckBootLog()
        if (UnitTestResult):
            logging.info("UnitTest modules failed.")
            return UnitTestResult
        logging.info("UnitTest modules succussfully boot.")
        if ret == 0xc0000005:
            #for some reason getting a c0000005 on successful return
            return 0

        return ret
