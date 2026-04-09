# @file
# Script to Build OVMF UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys
import subprocess

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
    ArchSupported = ("X64",)
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('ovmf', 'edk2-build')
    WorkspaceRoot = os.path.realpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".."))

    @classmethod
    def GetDscName(cls, ArchCsv: str) -> str:
        ''' return the DSC given the architectures requested.

        ArchCsv: csv string containing all architectures to build
        '''
        return "AmdSev/AmdSevX64.dsc"

import PlatformBuildLib
PlatformBuildLib.CommonPlatform = CommonPlatform

# hack alert -- create dummy grub.efi
subprocess.run(['touch', 'OvmfPkg/AmdSev/Grub/grub.efi'])
subprocess.run(['ls', '-l', '--sort=time', 'OvmfPkg/AmdSev/Grub'])
