## @file    ShellPkgHostTest.dsc
#
#  Copyright (c) Microsoft Corporation.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#  Description
#
##

[Defines]
PLATFORM_NAME           = ShellPkgHostTest
PLATFORM_GUID           = DE1A879F-BB19-44D8-A24F-21E79DB2A502
PLATFORM_VERSION        = 0.1
DSC_SPECIFICATION       = 0x00010005
OUTPUT_DIRECTORY        = Build/ShellPkg/HostTest
SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64
BUILD_TARGETS           = NOOPT
SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[LibraryClasses.common.HOST_APPLICATION]

[Components]
  #
  # Build HOST_APPLICATION Libraries With GoogleTest
  #
  ShellPkg/Test/Mock/Library/GoogleTest/MockShellLib/MockShellLib.inf
  ShellPkg/Test/Mock/Library/GoogleTest/MockShellCommandLib/MockShellCommandLib.inf
