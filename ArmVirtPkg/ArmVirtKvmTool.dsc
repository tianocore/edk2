#  @file
#  Workspace file for KVMTool virtual platform.
#
#  Copyright (c) 2018 - 2023, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ArmVirtKvmTool
  PLATFORM_GUID                  = 4CB2C61E-FA32-4130-8E37-54ABC71A1A43
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x0001001B
!ifdef $(EDK2_OUT_DIR)
  OUTPUT_DIRECTORY               = $(EDK2_OUT_DIR)
!else
  OUTPUT_DIRECTORY               = Build/ArmVirtKvmTool-$(ARCH)
!endif
  SUPPORTED_ARCHITECTURES        = AARCH64|ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmVirtPkg/ArmVirtKvmTool.fdf

[Defines.AARCH64]
  DEFINE ACPIVIEW_ENABLE         = TRUE

!include ArmVirtPkg/ArmVirt.dsc.inc

!if $(ARCH) == AARCH64
!include DynamicTablesPkg/DynamicTables.dsc.inc
!endif

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf

  # Virtio Support
  VirtioLib|OvmfPkg/Library/VirtioLib/VirtioLib.inf
  VirtioMmioDeviceLib|OvmfPkg/Library/VirtioMmioDeviceLib/VirtioMmioDeviceLib.inf

  ArmPlatformLib|ArmPlatformPkg/Library/ArmPlatformLibNull/ArmPlatformLibNull.inf
  ArmVirtMemInfoLib|ArmVirtPkg/Library/KvmtoolVirtMemInfoLib/KvmtoolVirtMemInfoLib.inf

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  VirtNorFlashPlatformLib|ArmVirtPkg/Library/NorFlashKvmtoolLib/NorFlashKvmtoolLib.inf

  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

  # BDS Libraries
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  PlatformBootManagerLib|ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf

  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf

  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf

  PciPcdProducerLib|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciHostBridgeLib|OvmfPkg/Fdt/FdtPciHostBridgeLib/FdtPciHostBridgeLib.inf
  PciHostBridgeUtilityLib|ArmVirtPkg/Library/ArmVirtPciHostBridgeUtilityLib/ArmVirtPciHostBridgeUtilityLib.inf

  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf

  PlatformPeiLib|ArmVirtPkg/Library/KvmtoolPlatformPeiLib/KvmtoolPlatformPeiLib.inf

  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PlatformHookLib|ArmVirtPkg/Library/Fdt16550SerialPortHookLib/Fdt16550SerialPortHookLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
!if $(TARGET) != RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif

  HwInfoParserLib|DynamicTablesPkg/Library/FdtHwInfoParserLib/FdtHwInfoParserLib.inf
  DynamicPlatRepoLib|DynamicTablesPkg/Library/Common/DynamicPlatRepoLib/DynamicPlatRepoLib.inf

  ArmMonitorLib|ArmPkg/Library/ArmMonitorLib/ArmMonitorLib.inf
  ArmTrngLib|ArmPkg/Library/ArmTrngLib/ArmTrngLib.inf

[LibraryClasses.common.SEC, LibraryClasses.common.PEI_CORE, LibraryClasses.common.PEIM]
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PlatformHookLib|ArmVirtPkg/Library/Fdt16550SerialPortHookLib/EarlyFdt16550SerialPortHookLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
!if $(TARGET) != RELEASE
  DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
!if $(TARGET) != RELEASE
  DebugLib|MdePkg/Library/DxeRuntimeDebugLibSerialPort/DxeRuntimeDebugLibSerialPort.inf
!endif

[LibraryClasses.common.UEFI_DRIVER]
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
  #
  # We need to avoid jump tables in SEC and BASE modules, so that the PE/COFF
  # self-relocation code itself is guaranteed to be position independent.
  #
  GCC:*_*_*_CC_XIPFLAGS = -fno-jump-tables

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport|FALSE

  # Use MMIO for accessing RTC controller registers.
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcUseMmio|TRUE

[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8000000F

  gArmPlatformTokenSpaceGuid.PcdCoreCount|1

!if $(ARCH) == AARCH64
  gArmTokenSpaceGuid.PcdVFPEnabled|1
!endif

  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  #
  # TTY Terminal Type
  # 0-PCANSI, 1-VT100, 2-VT00+, 3-UTF8, 4-TTYTERM
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4

  # Use MMIO for accessing Serial port registers.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialPciDeviceInfo|{0xFF}

  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

  #
  # The maximum physical I/O addressability of the processor, set with
  # BuildCpuHob().
  #
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|16

[PcdsPatchableInModule.common]
  #
  # This will be overridden in the code
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x0
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x0

  #
  # The device tree base address is handed off by kvmtool.
  # We are booting from RAM using the Linux kernel boot protocol,
  # x0 will point to the DTB image in memory.
  #
  gUefiOvmfPkgTokenSpaceGuid.PcdDeviceTreeInitialBaseAddress|0x0

  gArmTokenSpaceGuid.PcdFdBaseAddress|0x0
  gArmTokenSpaceGuid.PcdFvBaseAddress|0x0

  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x0

[PcdsDynamicHii]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5

[PcdsDynamicDefault.common]
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerHypVirtIntrNum|0x0

  #
  # ARM General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x0
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x0

  #
  # PCI settings
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration|TRUE

  # set PcdPciExpressBaseAddress to MAX_UINT64, which signifies that this
  # PCD and PcdPciDisableBusEnumeration above have not been assigned yet
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xFFFFFFFFFFFFFFFF

  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x0

  #
  # Set video resolution for boot options and for text setup.
  # PlatformDxe can set the former at runtime.
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|800
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|600
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|640
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|480

  # Setup Flash storage variables
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x40000

  # Define PCD for emulating Runtime Variable storage when
  # CFI flash is absent.
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|FALSE

  ## RTC Register address in MMIO space.
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcIndexRegister64|0x0
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcTargetRegister64|0x0

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]
  #
  # PEI Phase modules
  #
  ArmVirtPkg/PrePi/ArmVirtPrePiUniCoreRelocatable.inf {
    <LibraryClasses>
      ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
      HobLib|EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
      PrePiHobListPointerLib|ArmPlatformPkg/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
      MemoryAllocationLib|EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
  }

  #
  # DXE
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  #
  # Architectural Protocols
  #
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }

  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf {
    <LibraryClasses>
      NULL|ArmVirtPkg/Library/NorFlashKvmtoolLib/NorFlashKvmtoolLib.inf
  }

  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  MdeModulePkg/Universal/Metronome/Metronome.inf
  PcAtChipsetPkg/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf {
    <LibraryClasses>
      NULL|ArmVirtPkg/Library/KvmtoolRtcFdtClientLib/KvmtoolRtcFdtClientLib.inf
  }

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

  ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf {
    <LibraryClasses>
      NULL|ArmVirtPkg/Library/ArmVirtTimerFdtClientLib/ArmVirtTimerFdtClientLib.inf
  }

  OvmfPkg/VirtNorFlashDxe/VirtNorFlashDxe.inf {
    <LibraryClasses>
      # don't use unaligned CopyMem () on the UEFI varstore NOR flash region
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }

  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf

  #
  # Platform Driver
  #
  ArmVirtPkg/KvmtoolPlatformDxe/KvmtoolPlatformDxe.inf {
    <LibraryClasses>
    NULL|ArmVirtPkg/Library/NorFlashKvmtoolLib/NorFlashKvmtoolLib.inf
  }
  OvmfPkg/Fdt/VirtioFdtDxe/VirtioFdtDxe.inf
  EmbeddedPkg/Drivers/FdtClientDxe/FdtClientDxe.inf
  OvmfPkg/Fdt/HighMemDxe/HighMemDxe.inf
  OvmfPkg/VirtioBlkDxe/VirtioBlk.inf
  OvmfPkg/VirtioScsiDxe/VirtioScsi.inf
  OvmfPkg/VirtioNetDxe/VirtioNet.inf
  OvmfPkg/VirtioRngDxe/VirtioRng.inf

  #
  # FAT filesystem + GPT/MBR partitioning + UDF filesystem
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf {
    <LibraryClasses>
      DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Logo/LogoDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

  #
  # SCSI Bus and Disk Driver
  #
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  #
  # PCI support
  #
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
      NULL|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  }
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
      NULL|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  }
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
      NULL|OvmfPkg/Library/BaseCachingPciExpressLib/BaseCachingPciExpressLib.inf
  }
  OvmfPkg/VirtioPciDeviceDxe/VirtioPciDeviceDxe.inf
  OvmfPkg/Virtio10Dxe/Virtio10.inf

  #
  # Rng Support
  #
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf

!if $(ARCH) == AARCH64
  #
  # ACPI Support
  #
  ArmVirtPkg/KvmtoolCfgMgrDxe/ConfigurationManagerDxe.inf
!endif
