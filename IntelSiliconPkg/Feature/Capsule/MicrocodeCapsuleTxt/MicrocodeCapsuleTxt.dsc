## @file
# MicrocodeCapsuleTxt
#
# Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
#
# Uncomment the following line and update with your platform pkg name
#
#  PLATFORM_NAME                  = <PlatformPkg>
  PLATFORM_GUID                  = 6875FD33-602E-4EF9-9DF2-8BA7D8B7A7AF
  PLATFORM_VERSION               = 0.1
#
# Uncomment the following line and update with your platform pkg name
#
#  FLASH_DEFINITION               = <PlatformPkg>/MicrocodeCapsuleTxt/MicrocodeCapsuleTxt.fdf
#
# Uncomment the following line and update with your platform pkg name
#
#  OUTPUT_DIRECTORY               = Build/<PlatformPkg>
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[Components]
#
# Uncomment the following line and update with path to Microcode INF file
#
#  <PlatformPkg>/MicrocodeCapsuleTxt/Microcode/Microcode.inf
