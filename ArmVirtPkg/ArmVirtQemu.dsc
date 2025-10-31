#
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2020, Intel Corporation. All rights reserved.
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
  PLATFORM_NAME                  = ArmVirtQemu
  PLATFORM_GUID                  = 37d7e986-f7e9-45c2-8067-e371421a626c
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ArmVirtQemu-AArch64
  SUPPORTED_ARCHITECTURES        = AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmVirtPkg/ArmVirtQemu.fdf

  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE TTY_TERMINAL            = FALSE
  DEFINE SECURE_BOOT_ENABLE      = FALSE
  DEFINE QEMU_PV_VARS            = FALSE
  DEFINE TPM2_ENABLE             = FALSE
  DEFINE TPM2_CONFIG_ENABLE      = FALSE
  DEFINE CAVIUM_ERRATUM_27456    = FALSE

  #
  # Network definition
  #
  DEFINE NETWORK_IP6_ENABLE              = FALSE
  DEFINE NETWORK_HTTP_BOOT_ENABLE        = FALSE
  DEFINE NETWORK_SNP_ENABLE              = FALSE
  DEFINE NETWORK_TLS_ENABLE              = FALSE
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS  = TRUE
  DEFINE NETWORK_ISCSI_ENABLE            = FALSE
  DEFINE NETWORK_PXE_BOOT_ENABLE         = TRUE

# This comes at the beginning of includes to pick all relevant defines early on.
!include ArmVirtPkg/ArmVirtStackCookies.dsc.inc

!include NetworkPkg/NetworkDefines.dsc.inc

!include MdePkg/MdeLibs.dsc.inc

# This comes at the end of includes to pick all relevant components without any
# unintentional overrides.
!include ArmVirtPkg/ArmVirt.dsc.inc

[LibraryClasses.common]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf
  ArmTransferListLib|ArmPkg/Library/ArmTransferListLib/ArmTransferListLib.inf

  # Virtio Support
  VirtioLib|OvmfPkg/Library/VirtioLib/VirtioLib.inf
  VirtioMmioDeviceLib|OvmfPkg/Library/VirtioMmioDeviceLib/VirtioMmioDeviceLib.inf
  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioDxeLib.inf
  QemuFwCfgS3Lib|OvmfPkg/Library/QemuFwCfgS3Lib/BaseQemuFwCfgS3LibNull.inf
  QemuFwCfgSimpleParserLib|OvmfPkg/Library/QemuFwCfgSimpleParserLib/QemuFwCfgSimpleParserLib.inf
  QemuLoadImageLib|OvmfPkg/Library/GenericQemuLoadImageLib/GenericQemuLoadImageLib.inf

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  VirtNorFlashDeviceLib|OvmfPkg/Library/VirtNorFlashDeviceLib/VirtNorFlashDeviceLib.inf
  VirtNorFlashPlatformLib|OvmfPkg/Library/FdtNorFlashQemuLib/FdtNorFlashQemuLib.inf

  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  PlatformBootManagerLib|OvmfPkg/Library/PlatformBootManagerLibLight/PlatformBootManagerLib.inf
  PlatformBmPrintScLib|OvmfPkg/Library/PlatformBmPrintScLib/PlatformBmPrintScLib.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
  QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf
  PlatformBootManagerCommonLib|OvmfPkg/Library/PlatformBootManagerCommonLib/PlatformBootManagerCommonLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  PciPcdProducerLib|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciHostBridgeLib|OvmfPkg/Fdt/FdtPciHostBridgeLib/FdtPciHostBridgeLib.inf
  PciHostBridgeUtilityLib|OvmfPkg/Library/PciHostBridgeUtilityLib/PciHostBridgeUtilityLib.inf
  PeiHardwareInfoLib|OvmfPkg/Library/HardwareInfoLib/PeiHardwareInfoLib.inf

!if $(TPM2_ENABLE) == TRUE
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tcg2PhysicalPresenceLib|OvmfPkg/Library/Tcg2PhysicalPresenceLibQemu/DxeTcg2PhysicalPresenceLib.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLib/PeiDxeTpmPlatformHierarchyLib.inf
!else
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLibNull/PeiDxeTpmPlatformHierarchyLib.inf
!endif

  ArmMonitorLib|ArmVirtPkg/Library/ArmVirtMonitorLib/ArmVirtMonitorLib.inf
!if $(DEBUG_TO_MEM)
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogDxeLib.inf
!else
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogLibNull.inf
!endif

  ArmPlatformLib|ArmVirtPkg/Library/ArmPlatformLibQemu/ArmPlatformLibQemu.inf

[LibraryClasses.common.PEIM]
  ArmVirtMemInfoLib|ArmVirtPkg/Library/QemuVirtMemInfoLib/QemuVirtMemInfoPeiLib.inf
  ArmMonitorLib|ArmVirtPkg/Library/ArmVirtMonitorPeiLib/ArmVirtMonitorPeiLib.inf
  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  QemuFwCfgLib|OvmfPkg/Library/QemuFwCfgLib/QemuFwCfgMmioPeiLib.inf
!if $(DEBUG_TO_MEM)
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogPeiLib.inf
!else
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogLibNull.inf
!endif

  ArmMmuLib|UefiCpuPkg/Library/ArmMmuLib/ArmMmuPeiLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf

[LibraryClasses.common.DXE_DRIVER]
  AcpiPlatformLib|OvmfPkg/Library/AcpiPlatformLib/DxeAcpiPlatformLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

!if $(TPM2_ENABLE) == TRUE
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
!endif

[LibraryClasses.common.UEFI_DRIVER]
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[BuildOptions]
!if $(CAVIUM_ERRATUM_27456) == TRUE
  GCC:*_*_AARCH64_PP_FLAGS = -DCAVIUM_ERRATUM_27456
!endif

!include NetworkPkg/NetworkBuildOptions.dsc.inc

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuBootOrderPciTranslation|TRUE
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuBootOrderMmioTranslation|TRUE

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

  gEfiMdeModulePkgTokenSpaceGuid.PcdTurnOffUsbLegacySupport|TRUE

  gArmVirtTokenSpaceGuid.PcdTpm2SupportEnabled|$(TPM2_ENABLE)

!if $(QEMU_PV_VARS) == TRUE
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuVarsRequire|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdEnableVariableRuntimeCache|FALSE
!endif

[PcdsFixedAtBuild.common]
  gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFdBaseAddress|0x00000000
  gUefiOvmfPkgTokenSpaceGuid.PcdOvmfFirmwareFdSize|$(FD_SIZE)

  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x4007c000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800
!if $(NETWORK_TLS_ENABLE) == TRUE
  #
  # The cumulative and individual VOLATILE variable size limits should be set
  # high enough for accommodating several and/or large CA certificates.
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x80000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVolatileVariableSize|0x40000
!endif

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x04000000

  #
  # ARM PrimeCell
  #

  ## PL011 - Serial Terminal
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|38400

  ## Default Terminal Type
  ## 0-PCANSI, 1-VT100, 2-VT00+, 3-UTF8, 4-TTYTERM
!if $(TTY_TERMINAL) == TRUE
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4
  # Set terminal type to TtyTerm, the value encoded is EFI_TTY_TERM_GUID
  gUefiOvmfPkgTokenSpaceGuid.PcdTerminalTypeGuidBuffer|{0x80, 0x6d, 0x91, 0x7d, 0xb1, 0x5b, 0x8c, 0x45, 0xa4, 0x8f, 0xe2, 0x5f, 0xdd, 0x51, 0xef, 0x94}
!else
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|1
!endif

  #
  # Network Pcds
  #
!include NetworkPkg/NetworkFixedPcds.dsc.inc

  # System Memory Base -- fixed at 0x4000_0000
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x40000000

  # initial location of the device tree blob passed by QEMU -- base of DRAM
  gUefiOvmfPkgTokenSpaceGuid.PcdDeviceTreeInitialBaseAddress|0x40000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE

  # Point to the MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0xdc, 0x5b, 0xc2, 0xee, 0xf2, 0x67, 0x95, 0x4d, 0xb1, 0xd5, 0xf8, 0x1b, 0x20, 0x39, 0xd1, 0x1d }

  #
  # The maximum physical I/O addressability of the processor, set with
  # BuildCpuHob().
  #
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|16

  #
  # Enable the non-executable DXE stack. (This gets set up by DxeIpl)
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetNxForStack|TRUE

!if $(SECURE_BOOT_ENABLE) == TRUE
  # override the default values from SecurityPkg to ensure images from all sources are verified in secure boot
  gEfiSecurityPkgTokenSpaceGuid.PcdOptionRomImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdFixedMediaImageVerificationPolicy|0x04
  gEfiSecurityPkgTokenSpaceGuid.PcdRemovableMediaImageVerificationPolicy|0x04
!endif

  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|3
  gEfiShellPkgTokenSpaceGuid.PcdShellFileOperationSize|0x20000

  # Shadowing PEI modules is absolutely pointless when the NOR flash is emulated
  gEfiMdeModulePkgTokenSpaceGuid.PcdShadowPeimOnBoot|FALSE

  # System Memory Size -- 128 MB initially, actual size will be fetched from DT
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x8000000

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize   | 0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize   | 0x40000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize | 0x40000

[PcdsFixedAtBuild.AARCH64]
  # Clearing BIT0 in this PCD prevents installing a 32-bit SMBIOS entry point,
  # if the entry point version is >= 3.0. AARCH64 OSes cannot assume the
  # presence of the 32-bit entry point anyway (because many AARCH64 systems
  # don't have 32-bit addressable physical RAM), and the additional allocations
  # below 4 GB needlessly fragment the memory map. So expose the 64-bit entry
  # point only, for entry point versions >= 3.0.
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosEntryPointProvideMethod|0x2

[PcdsDynamicDefault.common]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|3

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase     | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase64   | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase64   | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase     | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase   | 0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase64 | 0

  ## If TRUE, OvmfPkg/AcpiPlatformDxe will not wait for PCI
  #  enumeration to complete before installing ACPI tables.
  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration|TRUE

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

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x0

  # set PcdPciExpressBaseAddress to MAX_UINT64, which signifies that this
  # PCD and PcdPciDisableBusEnumeration above have not been assigned yet
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xFFFFFFFFFFFFFFFF

  gEfiMdePkgTokenSpaceGuid.PcdPciIoTranslation|0x0

  #
  # SMBIOS entry point version
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0300
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev|0x0
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuSmbiosValidated|FALSE

!include OvmfPkg/Include/Dsc/OvmfDisplayPcds.dsc.inc

!include NetworkPkg/NetworkDynamicPcds.dsc.inc

  #
  # TPM2 support
  #
!if $(TPM2_ENABLE) == TRUE
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmBaseAddress|0x0
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2HashMask|0
!else
[PcdsPatchableInModule]
  # make this PCD patchable instead of dynamic when TPM support is not enabled
  # this permits setting the PCD in unreachable code without pulling in dynamic PCD support
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmBaseAddress|0x0
!endif

[PcdsDynamicHii]
  gUefiOvmfPkgTokenSpaceGuid.PcdForceNoAcpi|L"ForceNoAcpi"|gOvmfVariableGuid|0x0|FALSE|NV,BS

!if $(TPM2_CONFIG_ENABLE) == TRUE
  gEfiSecurityPkgTokenSpaceGuid.PcdTcgPhysicalPresenceInterfaceVer|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x0|"1.3"|NV,BS
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2AcpiTableRev|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x8|3|NV,BS
!endif

  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|L"Timeout"|gEfiGlobalVariableGuid|0x0|5

[LibraryClasses.common.PEI_CORE, LibraryClasses.common.PEIM]
!if $(TPM2_ENABLE) == TRUE
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
!else
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!endif

[LibraryClasses.common.SEC]
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogLibNull.inf

[LibraryClasses.common.PEI_CORE]
!if $(DEBUG_TO_MEM)
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogPeiCoreLib.inf
!else
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogLibNull.inf
!endif

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
!if $(DEBUG_TO_MEM)
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogRtLib.inf
!else
  MemDebugLogLib|OvmfPkg/Library/MemDebugLogLib/MemDebugLogLibNull.inf
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
  ArmPlatformPkg/Sec/Sec.inf
  MdeModulePkg/Core/Pei/PeiMain.inf
  ArmPlatformPkg/PlatformPei/PlatformPeim.inf
  ArmVirtPkg/MemoryInitPei/MemoryInitPeim.inf
  ArmPkg/Drivers/CpuPei/CpuPei.inf

!if $(DEBUG_TO_MEM)
  OvmfPkg/MemDebugLogPei/MemDebugLogPei.inf
!endif

!if $(TPM2_ENABLE) == TRUE
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  MdeModulePkg/Universal/ResetSystemPei/ResetSystemPei.inf
  OvmfPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf
  SecurityPkg/Tcg/Tcg2Pei/Tcg2Pei.inf {
    <LibraryClasses>
      HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
      NULL|SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf
  }
!endif

  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
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
!if $(QEMU_PV_VARS) == TRUE
  OvmfPkg/VirtMmCommunicationDxe/VirtMmCommunication.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableSmmRuntimeDxe.inf
!else
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
      # don't use unaligned CopyMem () on the UEFI varstore NOR flash region
      BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  }
!endif
!if $(SECURE_BOOT_ENABLE) == TRUE
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
!if $(TPM2_ENABLE) == TRUE
      NULL|SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
!endif
  }
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
  OvmfPkg/EnrollDefaultKeys/EnrollDefaultKeys.inf
!else
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
!endif
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf {
    <LibraryClasses>
      NULL|EmbeddedPkg/Library/NvVarStoreFormattedLib/NvVarStoreFormattedLib.inf
  }
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf {
    <LibraryClasses>
      NULL|ArmVirtPkg/Library/ArmVirtPL031FdtClientLib/ArmVirtPL031FdtClientLib.inf
  }
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

  ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf {
    <LibraryClasses>
      NULL|ArmVirtPkg/Library/ArmVirtGicArchLib/ArmVirtGicArchLib.inf
  }
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
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf

  #
  # Status Code Routing
  #
  MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf

  #
  # Platform Driver
  #
  OvmfPkg/Fdt/VirtioFdtDxe/VirtioFdtDxe.inf
  EmbeddedPkg/Drivers/FdtClientDxe/FdtClientDxe.inf
  OvmfPkg/Fdt/HighMemDxe/HighMemDxe.inf
  OvmfPkg/VirtioBlkDxe/VirtioBlk.inf
  OvmfPkg/VirtioScsiDxe/VirtioScsi.inf
  OvmfPkg/VirtioNetDxe/VirtioNet.inf
  OvmfPkg/VirtioRngDxe/VirtioRng.inf
  OvmfPkg/VirtioSerialDxe/VirtioSerial.inf {
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0
  }
  OvmfPkg/VirtioKeyboardDxe/VirtioKeyboard.inf

  #
  # FAT filesystem + GPT/MBR partitioning + UDF filesystem + virtio-fs
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf
  OvmfPkg/VirtioFsDxe/VirtioFsDxe.inf

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
  MdeModulePkg/Application/BootManagerMenuApp/BootManagerMenuApp.inf
  OvmfPkg/QemuKernelLoaderFsDxe/QemuKernelLoaderFsDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Library/BlobVerifierLibNull/BlobVerifierLibNull.inf
  }

  #
  # Networking stack
  #
!include NetworkPkg/NetworkComponents.dsc.inc
!include OvmfPkg/Include/Dsc/NetworkComponents.dsc.inc

  #
  # SCSI Bus and Disk Driver
  #
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  #
  # NVME Driver
  #
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf

  #
  # SMBIOS Support
  #
  MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Library/SmbiosVersionLib/DetectSmbiosVersionLib.inf
  }
  OvmfPkg/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  #
  # PCI support
  #
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
  OvmfPkg/PciHotPlugInitDxe/PciHotPlugInit.inf
  OvmfPkg/VirtioPciDeviceDxe/VirtioPciDeviceDxe.inf
  OvmfPkg/Virtio10Dxe/Virtio10.inf

  #
  # Video support
  #
  OvmfPkg/QemuRamfbDxe/QemuRamfbDxe.inf
  OvmfPkg/VirtioGpuDxe/VirtioGpu.inf
  OvmfPkg/PlatformDxe/Platform.inf

!include OvmfPkg/Include/Dsc/UsbComponents.dsc.inc

  #
  # Hash2 Protocol Support
  #
  SecurityPkg/Hash2DxeCrypto/Hash2DxeCrypto.inf

  #
  # TPM2 support
  #
!if $(TPM2_ENABLE) == TRUE
  SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
    <LibraryClasses>
      HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
      NULL|SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf
  }
!if $(TPM2_CONFIG_ENABLE) == TRUE
  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf
!endif
!endif

  #
  # ACPI Support
  #
  OvmfPkg/PlatformHasAcpiDtDxe/PlatformHasAcpiDtDxe.inf
[Components.AARCH64]
  MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
  OvmfPkg/AcpiPlatformDxe/AcpiPlatformDxe.inf {
    <LibraryClasses>
      NULL|OvmfPkg/Fdt/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  }
