#/** @file
# ARM processor package.
#
# Copyright (c) 2009 - 2010, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
#
#    This program and the accompanying materials
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
  SUPPORTED_ARCHITECTURES        = ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[BuildOptions]
  XCODE:*_*_ARM_PLATFORM_FLAGS  == -arch armv7
  XCODE:RELEASE_*_*_CC_FLAGS     = -DMDEPKG_NDEBUG 
  
  GCC:*_*_ARM_PLATFORM_FLAGS    == -march=armv7-a -mfpu=neon
  GCC:RELEASE_*_*_CC_FLAGS     = -DMDEPKG_NDEBUG 

  RVCT:*_*_ARM_PLATFORM_FLAGS  == --cpu Cortex-A8
  RVCT:RELEASE_*_*_CC_FLAGS  = -DMDEPKG_NDEBUG 

[LibraryClasses.common]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
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
  DefaultExceptionHandlerLib|ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf

  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  ArmGicLib|ArmPkg/Drivers/PL390Gic/PL390GicLib.inf
  ArmSmcLib|ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmDisassemblerLib|ArmPkg/Library/ArmDisassemblerLib/ArmDisassemblerLib.inf
  DmaLib|ArmPkg/Library/ArmDmaLib/ArmDmaLib.inf

  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf

  BdsLib|ArmPkg/Library/BdsLib/BdsLib.inf
  FdtLib|EmbeddedPkg/Library/FdtLib/FdtLib.inf
  
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.ARM]
  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf

[LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/AArch64/AArch64Lib.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf

[LibraryClasses.common.DXE_DRIVER]
  ArmPlatformGlobalVariableLib|ArmPlatformPkg/Library/ArmPlatformGlobalVariableLib/Dxe/DxeArmPlatformGlobalVariableLib.inf

[LibraryClasses.ARM]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

[LibraryClasses.AARCH64]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

[Components.common]
  ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  ArmPkg/Library/ArmDisassemblerLib/ArmDisassemblerLib.inf
  ArmPkg/Library/ArmDmaLib/ArmDmaLib.inf
  ArmPkg/Library/ArmLib/Null/NullArmLib.inf
  ArmPkg/Library/BaseMemoryLibStm/BaseMemoryLibStm.inf
  ArmPkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  ArmPkg/Library/BdsLib/BdsLib.inf
  ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  ArmPkg/Library/DebugAgentSymbolsBaseLib/DebugAgentSymbolsBaseLib.inf
  ArmPkg/Library/DebugPeCoffExtraActionLib/DebugPeCoffExtraActionLib.inf
  ArmPkg/Library/DebugUncachedMemoryAllocationLib/DebugUncachedMemoryAllocationLib.inf
  ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf
  ArmPkg/Library/RvdPeCoffExtraActionLib/RvdPeCoffExtraActionLib.inf
  ArmPkg/Library/SemiHostingDebugLib/SemiHostingDebugLib.inf
  ArmPkg/Library/SemiHostingSerialPortLib/SemiHostingSerialPortLib.inf
  ArmPkg/Library/SemihostLib/SemihostLib.inf
  ArmPkg/Library/UncachedMemoryAllocationLib/UncachedMemoryAllocationLib.inf

  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf
  ArmPkg/Drivers/PL390Gic/PL390GicDxe.inf
  ArmPkg/Drivers/PL390Gic/PL390GicLib.inf
  ArmPkg/Drivers/PL390Gic/PL390GicSecLib.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf

  ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmPkg/Library/ArmSmcLibNull/ArmSmcLibNull.inf

  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf

  ArmPkg/Application/LinuxLoader/LinuxFdtLoader.inf

[Components.ARM]
  ArmPkg/Library/BaseMemoryLibVstm/BaseMemoryLibVstm.inf

  ArmPkg/Drivers/ArmCpuLib/ArmCortexA8Lib/ArmCortexA8Lib.inf
  ArmPkg/Drivers/ArmCpuLib/ArmCortexA9Lib/ArmCortexA9Lib.inf
  ArmPkg/Drivers/ArmCpuLib/ArmCortexA15Lib/ArmCortexA15Lib.inf

#  ArmPkg/Library/ArmLib/Arm11/Arm11ArmLib.inf
#  ArmPkg/Library/ArmLib/Arm11/Arm11ArmLibPrePi.inf
#  ArmPkg/Library/ArmLib/Arm9/Arm9ArmLib.inf
#  ArmPkg/Library/ArmLib/Arm9/Arm9ArmLibPrePi.inf
  ArmPkg/Library/ArmLib/ArmV7/ArmV7LibSec.inf
  ArmPkg/Library/ArmLib/ArmV7/ArmV7LibPrePi.inf

  ArmPkg/Application/LinuxLoader/LinuxAtagLoader.inf

[Components.AARCH64]
  ArmPkg/Drivers/ArmCpuLib/ArmCortexAEMv8Lib/ArmCortexAEMv8Lib.inf
  ArmPkg/Drivers/ArmCpuLib/ArmCortexA5xLib/ArmCortexA5xLib.inf

  ArmPkg/Library/ArmLib/AArch64/AArch64LibSec.inf
  ArmPkg/Library/ArmLib/AArch64/AArch64LibPrePi.inf
