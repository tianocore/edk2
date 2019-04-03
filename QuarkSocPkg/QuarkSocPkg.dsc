## @file
# INTEL Quark SoC Module Package Reference Implementations
#
# This DSC file is used for Package Level build.
#
# This Module provides FRAMEWORK reference implementation for INTEL Quark SoC.
#   Copyright (c) 2013-2016 Intel Corporation.
#
#   SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = QuarkSocPkg
  PLATFORM_GUID                  = 5F9864F4-EAFB-4ded-A41A-CA501EE50502
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/QuarkSocPkg
  SUPPORTED_ARCHITECTURES        = IA32
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this
#                              Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT              # The entry: 0|DEFAULT is reserved and always required.

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses]
  #
  # Entry point
  #
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
!if $(CFG_SOURCE_DEBUG) == 1
  PeCoffExtraActionLib|SourceLevelDebugPkg/Library/PeCoffExtraActionLibDebug/PeCoffExtraActionLibDebug.inf
!else
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
!endif
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiCpuLib|UefiCpuPkg/Library/BaseUefiCpuLib/BaseUefiCpuLib.inf
  #
  # Framework
  #
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  S3IoLib|MdePkg/Library/BaseS3IoLib/BaseS3IoLib.inf
  S3PciLib|MdePkg/Library/BaseS3PciLib/BaseS3PciLib.inf
  #
  # Generic Modules
  #
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf

  #
  # CPU
  #
  MtrrLib|QuarkSocPkg/QuarkNorthCluster/Library/MtrrLib/MtrrLib.inf
  #
  # Quark North Cluster
  #
  SmmLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCSmmLib/QNCSmmLib.inf
  SmbusLib|QuarkSocPkg/QuarkNorthCluster/Library/SmbusLib/SmbusLib.inf
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/DxeAcpiTimerLib.inf
  ResetSystemLib|QuarkSocPkg/QuarkNorthCluster/Library/ResetSystemLib/ResetSystemLib.inf
  IntelQNCLib|QuarkSocPkg/QuarkNorthCluster/Library/IntelQNCLib/IntelQNCLib.inf
  QNCAccessLib|QuarkSocPkg/QuarkNorthCluster/Library/QNCAccessLib/QNCAccessLib.inf
  #
  # Quark South Cluster
  #
  IohLib|QuarkSocPkg/QuarkSouthCluster/Library/IohLib/IohLib.inf
  SerialPortLib|MdeModulePkg/Library/BaseSerialPortLib16550/BaseSerialPortLib16550.inf
  PlatformHookLib|MdeModulePkg/Library/BasePlatformHookLibNull/BasePlatformHookLibNull.inf
  #
  # Misc
  #
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugCommunicationLib|SourceLevelDebugPkg/Library/DebugCommunicationLibSerialPort/DebugCommunicationLibSerialPort.inf
!else
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
!endif

[LibraryClasses.IA32.PEIM,LibraryClasses.IA32.PEI_CORE]
  #
  # SEC and PEI phase common
  #
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  PerformanceLib|MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SecPeiDebugAgentLib.inf
!endif
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

[LibraryClasses.IA32.SEC]
  #
  # SEC specific phase
  #
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  TimerLib|PcAtChipsetPkg/Library/AcpiTimerLib/BaseAcpiTimerLib.inf

[LibraryClasses.IA32]
  #
  # DXE phase common
  #
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
!if $(CFG_SOURCE_DEBUG) == 1
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/DxeDebugAgentLib.inf
!endif

[LibraryClasses.IA32.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxSmmLib.inf
!if $(CFG_SOURCE_DEBUG) == TRUE
  DebugAgentLib|SourceLevelDebugPkg/Library/DebugAgent/SmmDebugAgentLib.inf
!endif

[LibraryClasses.IA32.SMM_CORE]
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf

[LibraryClasses.IA32.UEFI_DRIVER,LibraryClasses.IA32.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFixedAtBuild]
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialLineControl|0x03
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialFifoControl|0x07
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialDetectCable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialClockRate|44236800
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialPciDeviceInfo|{0x14, 0x05, 0x84, 0x00, 0xFF}
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterStride|4

  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBusNumber           |0
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciDeviceNumber        |31
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciFunctionNumber      |0
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciEnableRegisterOffset|0x4b
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoBarEnableMask          |0x80
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPciBarRegisterOffset   |0x48
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiIoPortBaseAddress        |0x1000
  gPcAtChipsetPkgTokenSpaceGuid.PcdAcpiPm1TmrOffset             |0x0008

[PcdsFeatureFlag]

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicDefault.common.DEFAULT]

###################################################################################################
#
# Components Section - list of the modules and components that will be processed by compilation
#                      tools and the EDK II tools to generate PE32/PE32+/Coff image files.
#
# Note: The EDK II DSC file is not used to specify how compiled binary images get placed
#       into firmware volume images. This section is just a list of modules to compile from
#       source into UEFI-compliant binaries.
#       It is the FDF file that contains information on combining binary files into firmware
#       volume images, whose concept is beyond UEFI and is described in PI specification.
#       Binary modules do not need to be listed in this section, as they should be
#       specified in the FDF file. For example: Shell binary (Shell_Full.efi), FAT binary (Fat.efi),
#       Logo (Logo.bmp), and etc.
#       There may also be modules listed in this section that are not required in the FDF file,
#       When a module listed here is excluded from FDF file, then UEFI-compliant binary will be
#       generated for it, but the binary will not be put into any firmware volume.
#
###################################################################################################

[Components]
  QuarkSocPkg/QuarkNorthCluster/MemoryInit/Pei/MemoryInitPei.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Pei/SmmAccessPei/SmmAccessPei.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Pei/SmmControlPei/SmmControlPei.inf
  QuarkSocPkg/QuarkNorthCluster/QNCInit/Dxe/QNCInitDxe.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/Dxe/SmmAccessDxe/SmmAccess.inf
  QuarkSocPkg/QuarkNorthCluster/Spi/PchSpiRuntime.inf {
    <LibraryClasses>
      PciExpressLib|MdePkg/Library/DxeRuntimePciExpressLib/DxeRuntimePciExpressLib.inf
  }
  QuarkSocPkg/QuarkNorthCluster/Spi/PchSpiSmm.inf
  QuarkSocPkg/QuarkNorthCluster/S3Support/Dxe/QncS3Support.inf {
    <LibraryClasses>
      DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  QuarkSocPkg/QuarkNorthCluster/Smm/Dxe/SmmControlDxe/SmmControlDxe.inf
  QuarkSocPkg/QuarkNorthCluster/Smm/DxeSmm/QncSmmDispatcher/QNCSmmDispatcher.inf

  QuarkSocPkg/QuarkSouthCluster/Usb/Common/Pei/UsbPei.inf
  QuarkSocPkg/QuarkSouthCluster/Usb/Ohci/Pei/OhciPei.inf
  QuarkSocPkg/QuarkSouthCluster/IohInit/Dxe/IohInitDxe.inf
  QuarkSocPkg/QuarkSouthCluster/Usb/Ohci/Dxe/OhciDxe.inf
  QuarkSocPkg/QuarkSouthCluster/Sdio/Dxe/SDControllerDxe/SDControllerDxe.inf
  QuarkSocPkg/QuarkSouthCluster/Sdio/Dxe/SDMediaDeviceDxe/SDMediaDeviceDxe.inf
