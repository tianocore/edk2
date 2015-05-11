#/** @file
# Embedded Package
#
#
# Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2012-2015, ARM Ltd. All rights reserved.<BR>
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
  PLATFORM_NAME                  = Embedded
  PLATFORM_GUID                  = 8DBB580B-CF89-4D57-95C6-DFE96C44686E
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/Embedded
  SUPPORTED_ARCHITECTURES        = IA32|X64|IPF|ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = EmbeddedPkg/EmbeddedPkg.fdf


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
[LibraryClasses.common]
#  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf


  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  EfiFileLib|EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf

  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf

  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf

  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  SerialPortExtLib|EmbeddedPkg/Library/TemplateSerialPortExtLib/TemplateSerialPortExtLib.inf
  RealTimeClockLib|EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf
  EfiResetSystemLib|EmbeddedPkg/Library/TemplateResetSystemLib/TemplateResetSystemLib.inf
  GdbSerialLib|EmbeddedPkg/Library/GdbSerialLib/GdbSerialLib.inf


 #
 # Need to change this for IPF
 #
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf

  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  EblCmdLib|EmbeddedPkg/Library/EblCmdLibNull/EblCmdLibNull.inf

  EblNetworkLib|EmbeddedPkg/Library/EblNetworkLib/EblNetworkLib.inf

  AcpiLib|EmbeddedPkg/Library/AcpiLib/AcpiLib.inf
  FdtLib|EmbeddedPkg/Library/FdtLib/FdtLib.inf

  # Shell libraries
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf

  # Networking Requirements
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf

[LibraryClasses.common.DXE_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf


[LibraryClasses.common.UEFI_APPLICATION]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.SEC]
  ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  ArmGicLib|ArmPkg/Drivers/ArmGic/ArmGicLib.inf
  ArmSmcLib|ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf
  BdsLib|ArmPkg/Library/BdsLib/BdsLib.inf
  SemihostLib|ArmPkg/Library/SemihostLib/SemihostLib.inf
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for GCC stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.ARM]
  ArmLib|ArmPkg/Library/ArmLib/ArmV7/ArmV7Lib.inf

[LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/AArch64/AArch64Lib.inf


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

  #
  # Control what commands are supported from the UI
  # Turn these on and off to add features or save size
  #
  gEmbeddedTokenSpaceGuid.PcdEmbeddedMacBoot|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedDirCmd|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedHobCmd|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedHwDebugCmd|TRUE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedIoEnable|FALSE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedScriptCmd|FALSE
  gEmbeddedTokenSpaceGuid.PcdEmbeddedPciDebugCmd|TRUE

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
  gEmbeddedTokenSpaceGuid.PcdEmbeddedAutomaticBootCommand|L""
  gEmbeddedTokenSpaceGuid.PcdEmbeddedDefaultTextColor|0x07
  gEmbeddedTokenSpaceGuid.PcdEmbeddedMemVariableStoreSize|0x10000

  gEmbeddedTokenSpaceGuid.PcdPrePiHobBase|0
  gEmbeddedTokenSpaceGuid.PcdPrePiStackBase|0
  gEmbeddedTokenSpaceGuid.PcdPrePiStackSize|0

#
# Optinal feature to help prevent EFI memory map fragments
# Turned on and off via: PcdPrePiProduceMemoryTypeInformationHob
# Values are in EFI Pages (4K). DXE Core will make sure that
# at least this much of each type of memory can be allocated
# from a single memory range. This way you only end up with
# maximum of two fragements for each type in the memory map
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

[PcdsFixedAtBuild.IPF]
  gEfiMdePkgTokenSpaceGuid.PcdIoBlockBaseAddressForIpf|0x0ffffc000000

#
# This makes it so you can source level debug with NT32. VC++ debugger limitiation!
#
#[BuildOptions]
#  DEBUG_*_IA32_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /ALIGN:4096 /SUBSYSTEM:CONSOLE
#  RELEASE_*_IA32_DLINK_FLAGS = /ALIGN:4096
#  *_*_IA32_CC_FLAGS = /D EFI_SPECIFICATION_VERSION=0x0002000A /D TIANO_RELEASE_VERSION=0x00080006

[BuildOptions]
  RVCT:*_*_ARM_PLATFORM_FLAGS == --cpu=7-A.security


################################################################################
#
# Components Section - list of all Modules needed by this Platform
#
################################################################################
[Components.common]
  EmbeddedPkg/Library/EblAddExternalCommandLib/EblAddExternalCommandLib.inf
  EmbeddedPkg/Library/EblCmdLibNull/EblCmdLibNull.inf
  EmbeddedPkg/Library/EfiFileLib/EfiFileLib.inf
  EmbeddedPkg/Library/GdbSerialDebugPortLib/GdbSerialDebugPortLib.inf
  EmbeddedPkg/Library/GdbSerialLib/GdbSerialLib.inf
  EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
  EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
  MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  EmbeddedPkg/Library/SerialPortExtLibNull/SerialPortExtLibNull.inf
  EmbeddedPkg/Library/TemplateResetSystemLib/TemplateResetSystemLib.inf
  EmbeddedPkg/Library/TemplateRealTimeClockLib/TemplateRealTimeClockLib.inf
  EmbeddedPkg/Library/LzmaHobCustomDecompressLib/LzmaHobCustomDecompressLib.inf
  EmbeddedPkg/Library/NullDmaLib/NullDmaLib.inf

  EmbeddedPkg/Ebl/Ebl.inf
####  EmbeddedPkg/EblExternCmd/EblExternCmd.inf
  EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  EmbeddedPkg/ResetRuntimeDxe/ResetRuntimeDxe.inf
  EmbeddedPkg/SerialDxe/SerialDxe.inf
  EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf {
    <LibraryClasses>
      TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  }

  EmbeddedPkg/Universal/MmcDxe/MmcDxe.inf

  # FDT installation
  EmbeddedPkg/Drivers/FdtPlatformDxe/FdtPlatformDxe.inf {
    <LibraryClasses>
      # It depends on BdsLib that depends on TimerLib
      TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  }

  EmbeddedPkg/Application/AndroidFastboot/AndroidFastbootApp.inf {
    <LibraryClasses>
      # It depends on BdsLib that depends on TimerLib
      TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  }
  EmbeddedPkg/Drivers/AndroidFastbootTransportUsbDxe/FastbootTransportUsbDxe.inf
  EmbeddedPkg/Drivers/AndroidFastbootTransportTcpDxe/FastbootTransportTcpDxe.inf

  # Drivers
  EmbeddedPkg/Drivers/Isp1761UsbDxe/Isp1761UsbDxe.inf
  EmbeddedPkg/Drivers/Lan9118Dxe/Lan9118Dxe.inf
  EmbeddedPkg/Drivers/SataSiI3132Dxe/SataSiI3132Dxe.inf

[Components.IA32, Components.X64, Components.IPF, Components.ARM]
  EmbeddedPkg/GdbStub/GdbStub.inf
