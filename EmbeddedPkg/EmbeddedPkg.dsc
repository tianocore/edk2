#/** @file
# Embedded Package
#
#
# Copyright (c) 2007 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2012-2015, ARM Ltd. All rights reserved.<BR>
# Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
#**/

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = Embedded
  PLATFORM_GUID                  = 8DBB580B-CF89-4D57-95C6-DFE96C44686E
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/Embedded
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64|RISCV64|LOONGARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
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

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]
  UfdtDispatcherLib|EmbeddedPkg/UfdtDispatcher/Library/UfdtDispatcherLib/UfdtDispatcherLib.inf
  UfdtDepexLib|EmbeddedPkg/UfdtDispatcher/Library/UfdtDepexLib/UfdtDepexLib.inf
#  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf


  AndroidBootImgLib|EmbeddedPkg/Library/AndroidBootImgLib/AndroidBootImgLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf

  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf

  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  RealTimeClockLib|EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf
  GdbSerialLib|EmbeddedPkg/Library/GdbSerialLib/GdbSerialLib.inf


 #
 # Need to change this for IPF
 #
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

  AcpiLib|EmbeddedPkg/Library/AcpiLib/AcpiLib.inf
  FdtLib|MdePkg/Library/BaseFdtLib/BaseFdtLib.inf

  # Shell libraries
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  # Networking Requirements
  !include NetworkPkg/NetworkLibs.dsc.inc
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DtPlatformDtbLoaderLib|EmbeddedPkg/Library/DxeDtPlatformDtbLoaderLibDefault/DxeDtPlatformDtbLoaderLibDefault.inf

  TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf


[LibraryClasses.common.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf

[LibraryClasses.common.SEC]
  ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf

[LibraryClasses.AARCH64]
  MapMmioLib|OvmfPkg/Library/MapMmioLib/MapMmioLib.inf
  ArmSmcLib|MdePkg/Library/ArmSmcLib/ArmSmcLib.inf
  SemihostLib|ArmPkg/Library/SemihostLib/SemihostLib.inf

  ArmLib|MdePkg/Library/ArmLib/ArmBaseLib.inf

################################################################################
#
# Pcd Section - list of all PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|FALSE

  gEmbeddedTokenSpaceGuid.PcdPrePiProduceMemoryTypeInformationHob|FALSE


[PcdsFixedAtBuild.common]
  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|10000000
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06
  gEfiMdePkgTokenSpaceGuid.PcdDebugClearMemoryValue|0xAF
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdPostCodePropertyMask|0
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdePkgTokenSpaceGuid.PcdFSBClock|200000000
  gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|320
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdPciExpressBaseAddress|0xE0000000
  gEfiMdePkgTokenSpaceGuid.PcdFSBClock|200000000

  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0

#
# Optional feature to help prevent EFI memory map fragments
# Turned on and off via: PcdPrePiProduceMemoryTypeInformationHob
# Values are in EFI Pages (4K). DXE Core will make sure that
# at least this much of each type of memory can be allocated
# from a single memory range. This way you only end up with
# maximum of two fragments for each type in the memory map
# (the memory used, and the free memory that was prereserved
# but not used).
#
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiACPIReclaimMemory|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiACPIMemoryNVS|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiReservedMemoryType|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiRuntimeServicesData|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiRuntimeServicesCode|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiBootServicesCode|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiBootServicesData|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiLoaderCode|0
  gEmbeddedTokenSpaceGuid.PcdMemoryTypeEfiLoaderData|0

#
# Timer config for this platform
#
  gEmbeddedTokenSpaceGuid.PcdTimerBaseAddress|0x3c700000
  gEmbeddedTokenSpaceGuid.PcdTimerVector|7
  gEmbeddedTokenSpaceGuid.PcdTimerPeriod|100000

[PcdsDynamicDefault.AARCH64]
  gArmTokenSpaceGuid.PcdGicDistributorBase|0
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcIndexRegister64|0
  gPcAtChipsetPkgTokenSpaceGuid.PcdRtcTargetRegister64|0
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0
  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|0
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|0
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|0
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|0
  gArmTokenSpaceGuid.PcdArmArchTimerHypVirtIntrNum|0
  gArmTokenSpaceGuid.PcdGenericWatchdogControlBase|0
  gArmTokenSpaceGuid.PcdGenericWatchdogRefreshBase|0
  gArmTokenSpaceGuid.PcdGenericWatchdogEl2IntrNum|0

[BuildOptions]
  RELEASE_*_*_CC_FLAGS  = -DMDEPKG_NDEBUG
  *_*_*_CC_FLAGS  = -DDISABLE_NEW_DEPRECATED_INTERFACES

################################################################################
#
# Components Section - list of all Modules needed by this Platform
#
################################################################################
[Components.common]
  EmbeddedPkg/UfdtDispatcher/Library/UfdtDepexLib/UfdtDepexLib.inf
  EmbeddedPkg/UfdtDispatcher/Library/UfdtDispatcherLib/UfdtDispatcherLib.inf
  EmbeddedPkg/Library/GdbSerialDebugPortLib/GdbSerialDebugPortLib.inf
  EmbeddedPkg/Library/GdbSerialLib/GdbSerialLib.inf
  EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
  EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf
  EmbeddedPkg/Library/CoherentDmaLib/CoherentDmaLib.inf
  EmbeddedPkg/Library/NonCoherentDmaLib/NonCoherentDmaLib.inf
  EmbeddedPkg/Library/DxeDtPlatformDtbLoaderLibDefault/DxeDtPlatformDtbLoaderLibDefault.inf
  EmbeddedPkg/Library/VirtualRealTimeClockLib/VirtualRealTimeClockLib.inf

  EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf {
    <LibraryClasses>
      TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  }

  EmbeddedPkg/Universal/MmcDxe/MmcDxe.inf

  EmbeddedPkg/Library/AcpiLib/AcpiLib.inf
  EmbeddedPkg/Library/DebugAgentTimerLibNull/DebugAgentTimerLibNull.inf
  EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
  EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf

  EmbeddedPkg/Drivers/ConsolePrefDxe/ConsolePrefDxe.inf
  EmbeddedPkg/Drivers/DtPlatformDxe/DtPlatformDxe.inf
  EmbeddedPkg/Drivers/FdtClientDxe/FdtClientDxe.inf

  EmbeddedPkg/Drivers/MemoryAttributeManagerDxe/MemoryAttributeManagerDxe.inf

  EmbeddedPkg/Drivers/NonCoherentIoMmuDxe/NonCoherentIoMmuDxe.inf {
    <LibraryClasses>
      DmaLib|EmbeddedPkg/Library/NonCoherentDmaLib/NonCoherentDmaLib.inf
  }

[Components.AARCH64]
  # FDT discovery framework and resource parsers.
  EmbeddedPkg/UfdtDispatcher/UfdtDispatcherDxe/UfdtDispatcher.inf {
    <LibraryClasses>
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtFixedClockBindingDriver/UfdtFixedClockBindingDriver.inf
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtGicV2BindingDriver/UfdtGicV2BindingDriver.inf
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtGicV3BindingDriver/UfdtGicV3BindingDriver.inf
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtTimerBindingDriver/UfdtTimerBindingDriver.inf
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtPl031BindingDriver/UfdtPl031BindingDriver.inf
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtMc146818RtcBindingDriver/UfdtMc146818RtcBindingDriver.inf
      NULL|EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtWatchdogBindingDriver/UfdtWatchdogBindingDriver.inf
  }
  ArmPkg/Drivers/ArmGicDxe/ArmGicV2Dxe.inf {
    <LibraryClasses>
      NULL|EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtGicV2DepexLib/UfdtGicV2DepexLib.inf
  }
  ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf {
    <LibraryClasses>
      NULL|EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtGicV3DepexLib/UfdtGicV3DepexLib.inf
      TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  }
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtFixedClockBindingDriver/UfdtFixedClockBindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtGicV2BindingDriver/UfdtGicV2BindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtGicV3BindingDriver/UfdtGicV3BindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtMc146818RtcBindingDriver/UfdtMc146818RtcBindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtPl031BindingDriver/UfdtPl031BindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtTimerBindingDriver/UfdtTimerBindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/BindingDrivers/UfdtWatchdogBindingDriver/UfdtWatchdogBindingDriver.inf
  EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtGicV2DepexLib/UfdtGicV2DepexLib.inf
  EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtGicV3DepexLib/UfdtGicV3DepexLib.inf
  EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtRtcDepexLib/UfdtRtcDepexLib.inf
  EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtMc146818RtcDepexLib/UfdtMc146818RtcDepexLib.inf
  EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtTimerDepexLib/UfdtTimerDepexLib.inf
  EmbeddedPkg/UfdtDispatcher/Library/Depexes/UfdtWatchdogDepexLib/UfdtWatchdogDepexLib.inf

  EmbeddedPkg/Application/AndroidBoot/AndroidBootApp.inf
  EmbeddedPkg/Application/AndroidFastboot/AndroidFastbootApp.inf
  EmbeddedPkg/Drivers/AndroidFastbootTransportUsbDxe/FastbootTransportUsbDxe.inf
  EmbeddedPkg/Drivers/AndroidFastbootTransportTcpDxe/FastbootTransportTcpDxe.inf

[Components.IA32, Components.X64]
  EmbeddedPkg/GdbStub/GdbStub.inf

[Components.IA32, Components.X64, Components.RISCV64, Components.LOONGARCH64]
  EmbeddedPkg/UfdtDispatcher/UfdtDispatcherDxe/UfdtDispatcher.inf
