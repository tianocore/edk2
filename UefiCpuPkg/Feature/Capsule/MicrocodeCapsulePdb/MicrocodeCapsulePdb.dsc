## @file
#
# Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
#
#    This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
#  FLASH_DEFINITION               = <PlatformPkg>/MicrocodeCapsulePdb/MicrocodeCapsulePdb.fdf
#
# Uncomment the following line and update with your platform pkg name
#
#  OUTPUT_DIRECTORY               = Build/<PlatformPkg>
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
