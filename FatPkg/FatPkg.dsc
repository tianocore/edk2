#/** @file
#
#  Build Binary Enhanced Fat Driver Modules
#
#  This Platform file is used to generate the Binary Fat Drivers
#  for EDK II Prime release.
#  Copyright (c) 2007, Intel Corporation
#
#  This program and the accompanying materials are licensed and made available
#  under the terms and conditions of the BSD License which accompanies this
#  distribution. The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

[Defines]
  PLATFORM_NAME                  = Fat
  PLATFORM_GUID                  = 25b55dbc-9d0b-4a32-80da-46e1273d622c
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  SUPPORTED_ARCHITECTURES        = IA32|X64|IPF|EBC
  OUTPUT_DIRECTORY               = Build/Fat
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses.common]
  #
  # Entry Point Libraries
  #
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  #
  # Common Libraries
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf

[Components.common]
  FatPkg\EnhancedFatDxe\Fat.inf
