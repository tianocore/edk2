#
#  Copyright (c) 2012-2015, ARM Limited. All rights reserved.
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
  PLATFORM_NAME                  = ArmVExpressPkg-CTA15-A7
  PLATFORM_GUID                  = 0b511920-978d-4b34-acc0-3d9f8e6f9d81
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ArmVExpress-CTA15-A7
  SUPPORTED_ARCHITECTURES        = ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmPlatformPkg/ArmVExpressPkg/ArmVExpress-CTA15-A7.fdf
  DEFINE EDK2_SKIP_PEICORE=1

!include ArmPlatformPkg/ArmVExpressPkg/ArmVExpress.dsc.inc

[LibraryClasses.common]
  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf
  ArmPlatformLib|ArmPlatformPkg/ArmVExpressPkg/Library/ArmVExpressLibCTA15-A7/ArmVExpressLib.inf

  ArmPlatformSysConfigLib|ArmPlatformPkg/ArmVExpressPkg/Library/ArmVExpressSysConfigLib/ArmVExpressSysConfigLib.inf
  NorFlashPlatformLib|ArmPlatformPkg/ArmVExpressPkg/Library/NorFlashArmVExpressLib/NorFlashArmVExpressLib.inf

  #DebugAgentTimerLib|ArmPlatformPkg/ArmVExpressPkg/Library/DebugAgentTimerLib/DebugAgentTimerLib.inf

  # ARM General Interrupt Driver in Secure and Non-secure
  ArmGicLib|ArmPkg/Drivers/ArmGic/ArmGicLib.inf

  LcdPlatformLib|ArmPlatformPkg/ArmVExpressPkg/Library/HdLcdArmVExpressLib/HdLcdArmVExpressLib.inf

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  ArmSmcLib|ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf

[BuildOptions]
!ifdef ARM_BIGLITTLE_TC2
  *_*_ARM_ARCHCC_FLAGS  = -DARM_BIGLITTLE_TC2=1
  *_*_ARM_PP_FLAGS  = -DARM_BIGLITTLE_TC2=1
!endif

  RVCT:*_*_ARM_PLATFORM_FLAGS == --cpu Cortex-A15 -I$(WORKSPACE)/ArmPlatformPkg/ArmVExpressPkg/Include -I$(WORKSPACE)/ArmPlatformPkg/ArmVExpressPkg/Include/Platform/CTA15-A7

  GCC:*_*_ARM_PLATFORM_FLAGS == -mcpu=cortex-a15 -I$(WORKSPACE)/ArmPlatformPkg/ArmVExpressPkg/Include -I$(WORKSPACE)/ArmPlatformPkg/ArmVExpressPkg/Include/Platform/CTA15-A7

  XCODE:*_*_ARM_PLATFORM_FLAGS = -I$(WORKSPACE)/ArmPlatformPkg/ArmVExpressPkg/Include -I$(WORKSPACE)/ArmPlatformPkg/ArmVExpressPkg/Include/Platform/CTA15-A7

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryInitializeInSec|TRUE
  gArmPlatformTokenSpaceGuid.PcdSendSgiToBringUpSecondaryCores|TRUE

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

[PcdsFixedAtBuild.common]
  gArmPlatformTokenSpaceGuid.PcdFirmwareVendor|"ARM Versatile Express"
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPrompt|"ArmVExpress-CTA15-A7"

  gArmPlatformTokenSpaceGuid.PcdCoreCount|5

  #
  # NV Storage PCDs. Use base of 0x0C000000 for NOR1
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0FFC0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0FFD0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x00010000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0FFE0000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x00010000

  gArmTokenSpaceGuid.PcdVFPEnabled|1

  # Stacks for MPCores in Secure World
  # SRAM (CS1) is only available between 0x14000000 and 0x14001000 on the model
  # ZBT SRAM is available between 0x2E000000 and 0x2E010000 on the model
!ifdef ARM_BIGLITTLE_TC2
  gArmPlatformTokenSpaceGuid.PcdCPUCoresSecStackBase|0x17000000
!else
  gArmPlatformTokenSpaceGuid.PcdCPUCoresSecStackBase|0x2E000000
!endif
  gArmPlatformTokenSpaceGuid.PcdCPUCoreSecPrimaryStackSize|0x8000
  gArmPlatformTokenSpaceGuid.PcdCPUCoreSecSecondaryStackSize|0x1000
  # Share Monitor stacks with Secure World
  gArmPlatformTokenSpaceGuid.PcdCPUCoreSecMonStackSize|0

  # System Memory (1GB) - An additional 1GB will be added if UEFI is running on a 2GB Test Chip
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x80000000
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x40000000

!ifdef ARM_BIGLITTLE_TC2
  # TC2 Dual-Cluster profile
  gArmPlatformTokenSpaceGuid.PcdClusterCount|2

  # Core Ids and Gic values
  # A15_0 = 0x000, GicCoreId = 0
  # A15_1 = 0x001, GicCoreId = 1
  #  A7_0 = 0x100, GicCoreId = 2
  #  A7_1 = 0x101, GicCoreId = 3
  #  A7_2 = 0x102, GicCoreId = 4
  gArmTokenSpaceGuid.PcdArmPrimaryCore|0x100
!endif

  #
  # SEC Phase Global Variables :
  # - 0x00-0x04: Debugger Exception Handler Pointer address
  # - 0x04-0x08: Normal Exception Handler Pointer
  # - 0x0C-0x10: MpSafe Serial Console SpinLock
  # - 0x10-0x20: KfScb 8 Bakery Locks of 2Bytes each
  # - 0x20-0x30: CCI 8 Bakery Locks of 2Bytes each
  # - 0x30-0x48: ARM SMC Events (8 cores * 3 max_event * sizeof(UINT8))
  gArmPlatformTokenSpaceGuid.PcdSecGlobalVariableSize|0x48

  #
  # ARM PrimeCell
  #

  ## SP805 Watchdog - Motherboard Watchdog
  gArmPlatformTokenSpaceGuid.PcdSP805WatchdogBase|0x1C0F0000

  ## PL011 - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x1C090000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|38400

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x1C170000

!ifdef ARM_BIGLITTLE_TC2
  ## PL111 Lcd & HdLcd
  gArmPlatformTokenSpaceGuid.PcdPL111LcdBase|0x1C1F0000
  gArmPlatformTokenSpaceGuid.PcdArmHdLcdBase|0x2B000000
  gArmVExpressTokenSpaceGuid.PcdHdLcdVideoModeOscId|5
!endif

  #
  # PL180 MMC/SD card controller
  #
  gArmPlatformTokenSpaceGuid.PcdPL180SysMciRegAddress|0x1C010048
  gArmPlatformTokenSpaceGuid.PcdPL180MciBaseAddress|0x1C050000


  #
  # ARM General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x2C001000
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x2C002000

  # ISP1761 USB OTG Controller
  gEmbeddedTokenSpaceGuid.PcdIsp1761BaseAddress|0x1B000000

  # Ethernet (SMSC LAN9118)
  gEmbeddedTokenSpaceGuid.PcdLan9118DxeBaseAddress|0x1A000000

  #
  # Define the device path to the FDT for the platform
  #
  gEmbeddedTokenSpaceGuid.PcdFdtDevicePaths|L"VenHw(1F15DA3C-37FF-4070-B471-BB4AF12A724A)/MemoryMapped(0x0,0x0E800000,0x0E803000)"

  #
  # ARM OS Loader
  #
  # Versatile Express machine type (ARM VERSATILE EXPRESS = 2272) required for ARM Linux:
  gArmTokenSpaceGuid.PcdArmMachineType|2272
  gArmPlatformTokenSpaceGuid.PcdDefaultBootDescription|L"Linux from NorFlash"
  gArmPlatformTokenSpaceGuid.PcdDefaultBootDevicePath|L"VenHw(1F15DA3C-37FF-4070-B471-BB4AF12A724A)/MemoryMapped(0x0,0xE000000,0xE800000)"
  gArmPlatformTokenSpaceGuid.PcdDefaultBootArgument|"console=ttyAMA0,38400 earlyprintk debug verbose"
  gArmPlatformTokenSpaceGuid.PcdDefaultBootType|2

  # Use the serial console (ConIn & ConOut) and the Graphic driver (ConOut)
  # PL111 - CLCD
  #gArmPlatformTokenSpaceGuid.PcdDefaultConOutPaths|L"VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(38400,8,N,1)/VenPcAnsi();VenHw(407B4008-BF5B-11DF-9547-CF16E0D72085)"
  # HDLCD
  gArmPlatformTokenSpaceGuid.PcdDefaultConOutPaths|L"VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(38400,8,N,1)/VenPcAnsi();VenHw(CE660500-824D-11E0-AC72-0002A5D5C51B)"
  gArmPlatformTokenSpaceGuid.PcdDefaultConInPaths|L"VenHw(D3987D4B-971A-435F-8CAF-4967EB627241)/Uart(38400,8,N,1)/VenPcAnsi()"

  #
  # ARM Architectural Timer Frequency
  #
!ifdef ARM_BIGLITTLE_TC2
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|24000000
!else
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|10000000
!endif

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]
  #
  # PEI Phase modules
  #
  ArmPlatformPkg/PrePi/PeiMPCore.inf {
    <LibraryClasses>
      ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf
      ArmPlatformLib|ArmPlatformPkg/ArmVExpressPkg/Library/ArmVExpressLibCTA15-A7/ArmVExpressLib.inf
      ArmPlatformGlobalVariableLib|ArmPlatformPkg/Library/ArmPlatformGlobalVariableLib/PrePi/PrePiArmPlatformGlobalVariableLib.inf
  }

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
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  EmbeddedPkg/SerialDxe/SerialDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

  ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  ArmPlatformPkg/Drivers/NorFlashDxe/NorFlashDxe.inf
  #ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/PL111LcdGraphicsOutputDxe.inf
  ArmPlatformPkg/Drivers/LcdGraphicsOutputDxe/HdLcdGraphicsOutputDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf
  ArmPlatformPkg/Drivers/SP805WatchdogDxe/SP805WatchdogDxe.inf

  #
  # Platform
  #
  ArmPlatformPkg/ArmVExpressPkg/ArmVExpressDxe/ArmHwDxe.inf

  #
  # Filesystems
  #
!ifndef ARM_BIGLITTLE_TC2
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf
!endif

  #
  # Multimedia Card Interface
  #
  EmbeddedPkg/Universal/MmcDxe/MmcDxe.inf
  ArmPlatformPkg/Drivers/PL180MciDxe/PL180MciDxe.inf

  # SMSC LAN 9118
  EmbeddedPkg/Drivers/Lan9118Dxe/Lan9118Dxe.inf

  #
  # FAT filesystem + GPT/MBR partitioning
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  MdeModulePkg/Universal/Acpi/AcpiTableDxe/AcpiTableDxe.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  ArmPlatformPkg/Bds/Bds.inf
