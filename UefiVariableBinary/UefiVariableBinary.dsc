## @file
# Secure Boot Variable File
#
# Builds a firmware volume to contain Secure Boot keys
#
# Copyright (c) 2021, Star Labs Online Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
  PLATFORM_NAME                  = SecureBoot
  PLATFORM_GUID                  = 1035eeff-543e-4abb-ac7e-bcd68cb530f8
  PLATFORM_VERSION               = 0.1
  OUTPUT_DIRECTORY               = Build/UefiVariableBinary
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = UefiVariableBinary/UefiVariableBinary.fdf


