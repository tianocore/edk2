#/** @file
# ARM processor package.
#
# Copyright (c) 2009 - 2010, Apple Inc. All rights reserved.
#
#  All rights reserved. This program and the accompanying materials
#    are licensed and made available under the terms and conditions of the BSD License
#    which accompanies this distribution. The full text of the license may be found at
#    http://opensource.org/licenses/bsd-license.php
#
#    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#**/

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ArmPkg
  PLATFORM_GUID                  = 5CFBD99E-3C43-4E7F-8054-9CDEAFF7710F
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/Arm
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT


[LibraryClasses.common]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

  SemihostLib|ArmPkg/Library/SemihostLib/SemihostLib.inf
  UncachedMemoryAllocationLib|ArmPkg/Library/UncachedMemoryAllocationLib/UncachedMemoryAllocationLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  DefaultExceptioHandlerLib|ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf

  ArmLib|ArmPkg/Library/ArmLib/ArmCortexA/ArmCortexArmLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  ArmDisassemblerLib|ArmPkg/Library/ArmDisassemblerLib/ArmDisassemblerLib.inf

[LibraryClasses.ARM]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf


[Components.common]
  ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  ArmPkg/Library/ArmLib/Arm11/Arm11ArmLib.inf
  ArmPkg/Library/ArmLib/Arm11/Arm11ArmLibPrePi.inf
  ArmPkg/Library/ArmLib/Arm9/Arm9ArmLib.inf
  ArmPkg/Library/ArmLib/Arm9/Arm9ArmLibPrePi.inf
  ArmPkg/Library/ArmLib/ArmCortexA/ArmCortexArmLib.inf
  ArmPkg/Library/ArmLib/ArmCortexA/ArmCortexArmLibPrePi.inf
  ArmPkg/Library/ArmLib/Null/NullArmLib.inf
  ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  ArmPkg/Library/SemiHostingDebugLib/SemiHostingDebugLib.inf
  ArmPkg/Library/SemiHostingSerialPortLib/SemiHostingSerialPortLib.inf
  ArmPkg/Library/SemihostLib/SemihostLib.inf
  ArmPkg/Library/UncachedMemoryAllocationLib/UncachedMemoryAllocationLib.inf
  ArmPkg/Library/DebugUncachedMemoryAllocationLib/DebugUncachedMemoryAllocationLib.inf
  ArmPkg/Library/RvdPeCoffExtraActionLib/RvdPeCoffExtraActionLib.inf
  ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf

  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  ArmPkg/Drivers/DebugSupportDxe/DebugSupportDxe.inf
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf
