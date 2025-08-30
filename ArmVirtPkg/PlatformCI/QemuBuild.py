# @file
# Script to Build OVMF UEFI firmware
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import sys

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
    PackagesSupported = ("ArmVirtPkg",)
    ArchSupported = ("AARCH64",)
    TargetsSupported = ("DEBUG", "RELEASE", "NOOPT")
    Scopes = ('armvirt', 'edk2-build')
    WorkspaceRoot = os.path.realpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", ".."))

    DscName = os.path.join("ArmVirtPkg", "ArmVirtQemu.dsc")

    # this platform produces a bootable NOR flash image
    FvQemuArg = " -pflash "

import PlatformBuildLib
PlatformBuildLib.CommonPlatform = CommonPlatform
