#
#  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ArmRealViewEb-RTSM-A9x2
  PLATFORM_GUID                  = f6c2f4a0-2027-11e0-a2a1-0002a5d5c51b
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ArmRealViewEb-RTSM-A9x2
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmPlatformPkg/ArmRealViewEbPkg/ArmRealViewEb-RTSM-MPCore.fdf

!include ArmPlatformPkg/ArmRealViewEbPkg/ArmRealViewEb.dsc.inc

[LibraryClasses.common]
  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf
  ArmCpuLib|ArmPkg/Drivers/ArmCpuLib/ArmCortexA9Lib/ArmCortexA9Lib.inf
  ArmPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ArmRealViewEbLibRTSM/ArmRealViewEbLib.inf

[LibraryClasses.common.SEC]
  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7LibSec.inf
  ArmPlatformSecLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ArmRealViewEbSecLibRTSM/ArmRealViewEbSecLib.inf
  ArmPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ArmRealViewEbLibRTSM/ArmRealViewEbLibSec.inf

[BuildOptions]
  RVCT:*_*_ARM_PLATFORM_FLAGS == --cpu Cortex-A9 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform

  GCC:*_*_ARM_PLATFORM_FLAGS == -mcpu=cortex-a9 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform

  XCODE:*_*_ARM_PLATFORM_FLAGS == -arch armv7 -I$(WORKSPACE)/ArmPlatformPkg/ArmRealViewEbPkg/Include/Platform


################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
!ifdef EDK2_SKIP_PEICORE
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryInitializeInSec|TRUE
  gArmPlatformTokenSpaceGuid.PcdSendSgiToBringUpSecondaryCores|TRUE
!endif

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

[PcdsFixedAtBuild.common]
  gArmPlatformTokenSpaceGuid.PcdFirmwareVendor|"ARM RealView Emulation Board"
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPrompt|"ArmRealViewEb-A9x2"

  gArmPlatformTokenSpaceGuid.PcdCoreCount|2

  #
  # NV Storage PCDs. Use base of 0x43F00000 for NOR0
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x43F00000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00020000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x43F20000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00020000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x43F40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00020000

  # Stacks for MPCores in Secure World
  gArmPlatformTokenSpaceGuid.PcdCPUCoresSecStackBase|0x4B000000
  # Stacks for MPCores in Monitor Mode
  gArmPlatformTokenSpaceGuid.PcdCPUCoresSecMonStackBase|0x4A000000
  # Stacks for MPCores in Normal World
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x48000000

  # System Memory (256MB)
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x70000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x10000000

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  #
  # ARM Pcds
  #
  gArmTokenSpaceGuid.PcdArmUncachedMemoryMask|0x0000000040000000

  #
  # ARM PrimeCells
  #

  ## SP804 Timer
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPerformanceCounterFrequencyInHz|1000000
  gEmbeddedTokenSpaceGuid.PcdTimerPeriod|100000        # expressed in 100ns units, 100,000 x 100 ns = 10,000,000 ns = 10 ms
  gArmPlatformTokenSpaceGuid.PcdSP804TimerPeriodicInterruptNum|33
  gArmPlatformTokenSpaceGuid.PcdSP804TimerPeriodicBase|0x10011000
  gArmPlatformTokenSpaceGuid.PcdSP804TimerMetronomeBase|0x10011020
  gArmPlatformTokenSpaceGuid.PcdSP804TimerPerformanceBase|0x10012020

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x10017000

  ## PL111 Lcd
  gArmPlatformTokenSpaceGuid.PcdPL111LcdBase|0x10020000

  #
  # ARM PL011 - Serial Terminal
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x10009000

  #
  # ARM General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x1F001000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x1F000100

  #
  # ARM L2x0 PCDs
  #
  gArmTokenSpaceGuid.PcdL2x0ControllerBase|0x1F002000

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]

  #
  # SEC
  #
  ArmPlatformPkg/Sec/Sec.inf {
    <LibraryClasses>
      # Use the implementation which set the Secure bits
      ArmGicLib|ArmPkg/Drivers/ArmGic/ArmGicSecLib.inf
  }

  #
  # PEI Phase modules
  #
!ifdef EDK2_SKIP_PEICORE
  ArmPlatformPkg/PrePi/PeiMPCore.inf {
    <LibraryClasses>
      ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf
      ArmPlatformLib|ArmPlatformPkg/ArmRealViewEbPkg/Library/ArmRealViewEbLibRTSM/ArmRealViewEbLib.inf
      ArmPlatformGlobalVariableLib|ArmPlatformPkg/Library/ArmPlatformGlobalVariableLib/PrePi/PrePiArmPlatformGlobalVariableLib.inf
  }
!else
  ArmPlatformPkg/PrePeiCore/PrePeiCoreMPCore.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  ArmPlatformPkg/PlatformPei/PlatformPeim.inf
  ArmPlatformPkg/MemoryInitPei/MemoryInitPeim.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf
  IntelFrameworkModulePkg/Universal/StatusCode/Pei/StatusCodePei.inf
  Nt32Pkg/BootModePei/BootModePei.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }
!endif

  #
  # DXE
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }

  #
  # Architectural Protocols
  #
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf

  EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/PL111LcdGraphicsOutputDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  EmbeddedPkg/SerialDxe/SerialDxe.inf

  ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  ArmPlatformPkg/Drivers/SP804TimerDxe/SP804TimerDxe.inf

  ArmPlatformPkg/Drivers/NorFlashDxe/NorFlashDxe.inf

  #
  # Semi-hosting filesystem
  #
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf

  #
  # FAT filesystem + GPT/MBR partitioning
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  ArmPlatformPkg/Bds/Bds.inf
