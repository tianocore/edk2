#/** @file
# Omap35xx SoC package.
#
# Copyright (c) 2009 - 2010, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
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
  PLATFORM_NAME                  = Omap35xxPkg
  PLATFORM_GUID                  = D196A631-B7B7-4953-A3EE-0F773CBABF20
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/Omap35xxPkg
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  DEFINE TARGET_HACK             = DEBUG


[LibraryClasses.common]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf

  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf

  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf

  CacheMaintenanceLib|ArmPkg/Library/ArmCacheMaintenanceLib/ArmCacheMaintenanceLib.inf
  DefaultExceptioHandlerLib|ArmPkg/Library/DefaultExceptionHandlerLib/DefaultExceptionHandlerLib.inf
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf

  RealTimeClockLib|EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf

  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  OmapLib|Omap35xxPkg/Library/OmapLib/OmapLib.inf
  OmapDmaLib|Omap35xxPkg/Library/OmapDmaLib/OmapDmaLib.inf

  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf

  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  DmaLib|EmbeddedPkg/Library/NonCoherentDmaLib/NonCoherentDmaLib.inf

  TimerLib|Omap35xxPkg/Library/Omap35xxTimerLib/Omap35xxTimerLib.inf

#
# Assume everything is fixed at build
#
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf


[LibraryClasses.common.DXE_DRIVER]
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  NonDiscoverableDeviceRegistrationLib|MdeModulePkg/Library/NonDiscoverableDeviceRegistrationLib/NonDiscoverableDeviceRegistrationLib.inf

[LibraryClasses.ARM]
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[BuildOptions]
  XCODE:*_*_ARM_ARCHCC_FLAGS     == -arch armv7 -march=armv7
  XCODE:*_*_ARM_ARCHASM_FLAGS    == -arch armv7
  XCODE:*_*_ARM_ARCHDLINK_FLAGS  == -arch armv7

  GCC:*_*_ARM_ARCHCC_FLAGS     == -march=armv7-a -mthumb
  GCC:*_*_ARM_ARCHASM_FLAGS    == -march=armv7-a

  RVCT:*_*_ARM_ARCHCC_FLAGS     == --cpu 7-A
  RVCT:*_*_ARM_ARCHASM_FLAGS    == --cpu 7-A

  *_*_*_CC_FLAGS = -DDISABLE_NEW_DEPRECATED_INTERFACES

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################


[PcdsFixedAtBuild.common]

# DEBUG_ASSERT_ENABLED       0x01
# DEBUG_PRINT_ENABLED        0x02
# DEBUG_CODE_ENABLED         0x04
# CLEAR_MEMORY_ENABLED       0x08
# ASSERT_BREAKPOINT_ENABLED  0x10
# ASSERT_DEADLOOP_ENABLED    0x20
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2f

#  DEBUG_INIT      0x00000001  // Initialization
#  DEBUG_WARN      0x00000002  // Warnings
#  DEBUG_LOAD      0x00000004  // Load events
#  DEBUG_FS        0x00000008  // EFI File system
#  DEBUG_POOL      0x00000010  // Alloc & Free (pool)
#  DEBUG_PAGE      0x00000020  // Alloc & Free (page)
#  DEBUG_INFO      0x00000040  // Informational debug messages
#  DEBUG_DISPATCH  0x00000080  // PEI/DXE/SMM Dispatchers
#  DEBUG_VARIABLE  0x00000100  // Variable
#  DEBUG_BM        0x00000400  // Boot Manager
#  DEBUG_BLKIO     0x00001000  // BlkIo Driver
#  DEBUG_NET       0x00004000  // SNP Driver
#  DEBUG_UNDI      0x00010000  // UNDI Driver
#  DEBUG_LOADFILE  0x00020000  // LoadFile
#  DEBUG_EVENT     0x00080000  // Event messages
#  DEBUG_GCD       0x00100000  // Global Coherency Database changes
#  DEBUG_CACHE     0x00200000  // Memory range cachability changes
#  DEBUG_VERBOSE   0x00400000  // Detailed debug messages that may
#                              // significantly impact boot performance
#  DEBUG_ERROR     0x80000000  // Error
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000004

  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07

  gEmbeddedTokenSpaceGuid.PcdFlashFvMainBase|0
  gEmbeddedTokenSpaceGuid.PcdFlashFvMainSize|0
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0x87FE0000 # stack at top of memory
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0x20000  # 128K stack
  gArmTokenSpaceGuid.PcdCpuVectorBaseAddress|0x80000000
  gArmTokenSpaceGuid.PcdCpuResetAddress|0x80008000

  gOmap35xxTokenSpaceGuid.PcdOmap35xxGpmcOffset|0x6E000000
  gOmap35xxTokenSpaceGuid.PcdOmap35xxMMCHS1Base|0x4809C000

  # Console
  gOmap35xxTokenSpaceGuid.PcdOmap35xxConsoleUart|3

  # Timers
  gOmap35xxTokenSpaceGuid.PcdOmap35xxArchTimer|3
  gOmap35xxTokenSpaceGuid.PcdOmap35xxFreeTimer|4
  gEmbeddedTokenSpaceGuid.PcdTimerPeriod|100000
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPerformanceCounterPeriodInNanoseconds|77
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPerformanceCounterFrequencyInHz|13000000

  # OMAP Interrupt Controller
  gEmbeddedTokenSpaceGuid.PcdInterruptBaseAddress|0x48200000

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]
  Omap35xxPkg/Library/Omap35xxTimerLib/Omap35xxTimerLib.inf
  Omap35xxPkg/Library/OmapLib/OmapLib.inf
  Omap35xxPkg/Library/OmapDmaLib/OmapDmaLib.inf

  Omap35xxPkg/Flash/Flash.inf
  Omap35xxPkg/MMCHSDxe/MMCHS.inf
  Omap35xxPkg/SmbusDxe/Smbus.inf
  Omap35xxPkg/Gpio/Gpio.inf
  Omap35xxPkg/InterruptDxe/InterruptDxe.inf
  Omap35xxPkg/TimerDxe/TimerDxe.inf
  Omap35xxPkg/TPS65950Dxe/TPS65950.inf

  Omap35xxPkg/LcdGraphicsOutputDxe/LcdGraphicsOutputDxe.inf
  Omap35xxPkg/Library/DebugAgentTimerLib/DebugAgentTimerLib.inf
  Omap35xxPkg/Library/GdbSerialLib/GdbSerialLib.inf
  Omap35xxPkg/Library/RealTimeClockLib/RealTimeClockLib.inf
  Omap35xxPkg/Library/SerialPortLib/SerialPortLib.inf
  Omap35xxPkg/MmcHostDxe/MmcHostDxe.inf
  Omap35xxPkg/PciEmulation/PciEmulation.inf


