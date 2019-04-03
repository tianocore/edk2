## @file
#  Dsc file for Dynamic Tables Framework.
#
#  Copyright (c) 2019, Linaro Limited. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = DynamicTables
  PLATFORM_GUID                  = f39096a0-7a0a-442a-9413-cf584ef80cbb
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001a
  OUTPUT_DIRECTORY               = Build/DynamicTables
  SUPPORTED_ARCHITECTURES        = ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include DynamicTables.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  PL011UartLib|ArmPlatformPkg/Library/PL011UartLib/PL011UartLib.inf

[Components.common]
  DynamicTablesPkg/Library/Common/TableHelperLib/TableHelperLib.inf
