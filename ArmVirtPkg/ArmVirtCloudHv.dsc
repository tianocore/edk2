#
#  Copyright (c) 2021, ARM Limited. All rights reserved.
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
  PLATFORM_NAME                  = ArmVirtCloudHv
  PLATFORM_GUID                  = DFFED32B-DFFE-D32B-DFFE-D32BDFFED32B
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ArmVirtCloudHv-AARCH64
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmVirtPkg/ArmVirtCloudHv.fdf

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE PLAT_CLOUD_HV           = TRUE
  DEFINE TTY_TERMINAL            = FALSE
  DEFINE SECURE_BOOT_ENABLE      = FALSE

  #
  # Defines for _SUPPORTED values, allowing to identify supported features.
  #
  DEFINE SECURE_BOOT_ENABLE_SUPPORTED                = TRUE

# This comes at the beginning of includes to pick all relevant defines early on.
!include ArmVirtPkg/ArmVirtStackCookies.dsc.inc

# This comes at the end of includes to pick all relevant components without any
# unintentional overrides.
!include ArmVirtPkg/ArmVirt.dsc.inc

[LibraryClasses.common]
  ArmPlatformLib|ArmPlatformPkg/Library/ArmPlatformLibNull/ArmPlatformLibNull.inf

  PlatformBootManagerLib|ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  PlatformBmPrintScLib|OvmfPkg/Library/PlatformBmPrintScLib/PlatformBmPrintScLib.inf
  QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  PciHostBridgeUtilityLib|ArmVirtPkg/Library/ArmVirtPciHostBridgeUtilityLib/ArmVirtPciHostBridgeUtilityLib.inf

  TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLibNull/PeiDxeTpmPlatformHierarchyLib.inf
  ArmTransferListLib|ArmPkg/Library/ArmTransferListLib/ArmTransferListLib.inf
  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgLibNull.inf
  QemuFwCfgSimpleParserLib|OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common.PEIM]
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  ArmVirtMemInfoLib|ArmVirtPkg/Library/CloudHvVirtMemInfoLib/CloudHvVirtMemInfoPeiLib.inf

[LibraryClasses.common.DXE_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[BuildOptions]
!include NetworkPkg/NetworkBuildOptions.dsc.inc

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

  gEfiMdeModulePkgTokenSpaceGuid.PcdTurnOffUsbLegacySupport|TRUE

[PcdsFixedAtBuild.common]
  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x4007c000
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvStoreReserved|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800

  # Rsdp base address in Cloud Hypervisor
  gArmVirtTokenSpaceGuid.PcdCloudHvAcpiRsdpBaseAddress|0x40200000

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x4000000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
!if $(NETWORK_TLS_ENABLE) == TRUE
  #
  # The cumulative and individual VOLATILE variable size limits should be set
  # high enough for accommodating several and/or large CA certificates.
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x80000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVolatileVariableSize|0x40000
!endif

  #
  # ARM PrimeCell
  #

  ## PL011 - Serial Terminal
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|38400

  ## Default Terminal Type
  ## 0-PCANSI, 1-VT100, 2-VT00+, 3-UTF8, 4-TTYTERM
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4

  # System Memory Base -- fixed at 0x4000_0000
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000

  # initial location of the device tree blob passed by Cloud Hypervisor -- base of DRAM
  gUefiOvmfPkgTokenSpaceGuid.PcdDeviceTreeInitialBaseAddress|0x40000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

  #
  # The maximum physical I/O addressability of the processor, set with
  # BuildCpuHob().
  #
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|16

  #
  # Enable the non-executable DXE stack. (This gets set up by DxeIpl)
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetNxForStack|TRUE

  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|3
  gEfiShellPkgTokenSpaceGuid.PcdShellFileOperationSize|0x20000

  # Clearing BIT0 in this PCD prevents installing a 32-bit SMBIOS entry point,
  # if the entry point version is >= 3.0. AARCH64 OSes cannot assume the
  # presence of the 32-bit entry point anyway (because many AARCH64 systems
  # don't have 32-bit addressable physical RAM), and the additional allocations
  # below 4 GB needlessly fragment the memory map. So expose the 64-bit entry
  # point only, for entry point versions >= 3.0.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosEntryPointProvideMethod|0x2

[PcdsDynamicDefault.common]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|3

  # System Memory Size -- 1 MB initially, actual size will be fetched from DT
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x00100000

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x0

  gEfiSecurityPkgTokenSpaceGuid.PcdTpmBaseAddress|0x0

[PcdsDynamicHii]
  gUefiOvmfPkgTokenSpaceGuid.PcdForceNoAcpi|L"ForceNoAcpi"|gOvmfVariableGuid|0x0|FALSE|NV,BS

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]
  #
  # PEI Phase modules
  #
  ArmPlatformPkg/Sec/Sec.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  ArmPlatformPkg/PlatformPei/PlatformPeim.inf
  ArmPlatformPkg/MemoryInitPei/MemoryInitPeim.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf

  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf

  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
  }

  #
  # Architectural Protocols
  #
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      # don't use unaligned CopyMem () on the UEFI varstore NOR flash region
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf {
    <LibraryClasses>
      NULL|ArmVirtPkg/Library/ArmVirtPL031FdtClientLib/ArmVirtPL031FdtClientLib.inf
  }

  #
  # Status Code Routing
  #
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf

  #
  # Platform Driver
  #
  OvmfPkg/VirtioNetDxe/VirtioNet.inf

  #
  # PCI support
  #
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf

  #
  # ACPI Support
  #
  ArmVirtPkg/CloudHvPlatformHasAcpiDtDxe/CloudHvHasAcpiDtDxe.inf
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
  ArmVirtPkg/CloudHvAcpiPlatformDxe/CloudHvAcpiPlatformDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
