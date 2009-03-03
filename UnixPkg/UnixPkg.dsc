#/** @file
#
# EFI/Framework Emulation Platform with UEFI HII interface supported.
#
# The Emulation Platform can be used to debug individual modules, prior to creating
#       a real platform. This also provides an example for how an FPD is created.
# Copyright (c) 2006 - 2008, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
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
  PLATFORM_NAME                  = Unix
  PLATFORM_GUID                  = 7b3c1fb4-8986-11db-b5b2-0040d02b1835
  PLATFORM_VERSION               = 0.3
  DSC_ SPECIFICATION             = 0x00010005
  OUTPUT_DIRECTORY               = Build/Unix
  SUPPORTED_ARCHITECTURES        = IA32
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = UnixPkg/UnixPkg.fdf

################################################################################
#
# SKU Identification section - list of all SKU IDs supported by this Platform.
#
################################################################################
[SkuIds]
  0|DEFAULT

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################
[LibraryClasses.common]
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PciIncompatibleDeviceSupportLib|IntelFrameworkModulePkg/Library/PciIncompatibleDeviceSupportLib/PciIncompatibleDeviceSupportLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  UefiDecompressLib|IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  ExtendedHiiLib|MdeModulePkg/Library/ExtendedHiiLib/ExtendedHiiLib.inf
  S3Lib|MdeModulePkg/Library/PeiS3LibNull/PeiS3LibNull.inf
  RecoveryLib|MdeModulePkg/Library/PeiRecoveryLibNull/PeiRecoveryLibNull.inf
  IfrSupportLib|MdeModulePkg/Library/UefiIfrSupportLib/UefiIfrSupportLib.inf
  ExtendedIfrSupportLib|MdeModulePkg/Library/ExtendedIfrSupportLib/ExtendedIfrSupportLib.inf
  GenericBdsLib|IntelFrameworkModulePkg/Library/GenericBdsLib/GenericBdsLib.inf
  PlatformBdsLib|UnixPkg/Library/UnixBdsLib/PlatformBds.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf

[LibraryClasses.common.USER_DEFINED]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/PeiUnixOemHookStatusCodeLib/PeiUnixOemHookStatusCodeLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf

[LibraryClasses.common.DXE_CORE]
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PeCoffLib|UnixPkg/Library/DxeUnixPeCoffLib/DxeUnixPeCoffLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf

[LibraryClasses.common.PEIM]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  IoLib|MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/PeiUnixOemHookStatusCodeLib/PeiUnixOemHookStatusCodeLib.inf
  PeCoffGetEntryPointLib|UnixPkg/Library/EdkUnixPeiPeCoffGetEntryPointLib/EdkUnixPeiPeCoffGetEntryPointLib.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  PeCoffLib|UnixPkg/Library/PeiUnixPeCoffLib/PeiUnixPeCoffLib.inf
  ExtractGuidedSectionLib|MdePkg/Library/PeiExtractGuidedSectionLib/PeiExtractGuidedSectionLib.inf

[LibraryClasses.common.PEI_CORE]
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  IoLib|MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  OemHookStatusCodeLib|IntelFrameworkModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  PeCoffGetEntryPointLib|UnixPkg/Library/EdkUnixPeiPeCoffGetEntryPointLib/EdkUnixPeiPeCoffGetEntryPointLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  PeCoffLib|UnixPkg/Library/PeiCoreUnixPeCoffLib/PeiCoreUnixPeCoffLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  UnixLib|UnixPkg/Library/DxeUnixLib/DxeUnixLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf

[LibraryClasses.common.UEFI_DRIVER]
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  UnixLib|UnixPkg/Library/DxeUnixLib/DxeUnixLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf

[LibraryClasses.common.DXE_DRIVER]
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  UnixLib|UnixPkg/Library/DxeUnixLib/DxeUnixLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf
  DpcLib|MdeModulePkg/Library/DxeDpcLib/DxeDpcLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UnixLib|UnixPkg/Library/DxeUnixLib/DxeUnixLib.inf
  OemHookStatusCodeLib|UnixPkg/Library/DxeUnixOemHookStatusCodeLib/DxeUnixOemHookStatusCodeLib.inf
  DebugLib|IntelFrameworkModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  PrintLib|MdeModulePkg/Library/DxePrintLibPrint2Protocol/DxePrintLibPrint2Protocol.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform.
#
################################################################################
[PcdsFixedAtBuild.IA32]
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeMemorySize|1
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeRuntimeMemorySize|128

  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashNvStorageVariableBase|0x280000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashNvStorageFtwSpareBase|0x290000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashNvStorageFtwWorkingBase|0x28e000

  gEfiUnixPkgTokenSpaceGuid.PcdUnixBootMode|1
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFirmwareFdSize|0x2a0000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFirmwareBlockSize|0x10000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashNvStorageEventLogBase|0x28c000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashNvStorageEventLogSize|0x2000
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashFvRecoveryBase|0x0
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFlashFvRecoverySize|0x280000
  
  gEfiUnixPkgTokenSpaceGuid.PcdUnixMemorySizeForSecMain|L"64!64"|VOID*|10
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFirmwareVolume|L"../FV/FV_RECOVERY.fd"|VOID*|52

  gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|10000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|1000000
  gEfiMdePkgTokenSpaceGuid.PcdSpinLockTimeout|10000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x1f
  gEfiMdePkgTokenSpaceGuid.PcdDebugClearMemoryValue|0xAF
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
  gEfiMdePkgTokenSpaceGuid.PcdPerformanceLibraryPropertyMask|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxPeiPcdCallBackNumberPerPcdEntry|0x08
  gEfiMdeModulePkgTokenSpaceGuid.PcdVpdBaseAddress|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizeNonPopulateCapsule|0x0
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxSizePopulateCapsule|0x0
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPciIncompatibleDeviceSupportMask|0
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueUncorrectableMemoryError|0x0005100   # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE3
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleError|0x01040006        # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleReset|0x01040001        # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleInputError|0x01040007   # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_INPUT_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueRemoteConsoleOutputError|0x01040008  # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_OUTPUT_ERROR
  gEfiMdePkgTokenSpaceGuid.PcdUefiVariableDefaultTimeout|0x0008
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueEfiWatchDogTimerExpired|0x00011003
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueMemoryTestStarted|0x00051006
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueSetVirtualAddressMap|0x03101004
  gEfiMdePkgTokenSpaceGuid.PcdStatusCodeValueUncorrectableMemoryError|0x00051003
  gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|320
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|8
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|1
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|1
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|0

  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x10000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x00c000
  gEfiMdePkgTokenSpaceGuid.PcdMaximumGuidedExtractHandler|0x10
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxFvSupported|6
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreMaxPeimPerFv|32
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x400
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxHardwareErrorVariableSize|0x8000
  gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x10000

[PcdsFeatureFlag.common]
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseOEM|TRUE
  gEfiEdkModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseHardSerial|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseEfiSerial|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseRuntimeMemory|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeUseDataHub|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInSerial|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInDataHub|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInRuntimeMemory|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInOEM|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdNtEmulatorEnable|FALSE
  gEfiEdkModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText|TRUE
  gEfiEdkModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText|TRUE
  gEfiGenericPlatformTokenSpaceGuid.PcdPciIsaEnable|FALSE
  gEfiGenericPlatformTokenSpaceGuid.PcdPciVgaEnable|FALSE
  gEfiGenericPlatformTokenSpaceGuid.PcdPciBusHotplugDeviceSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport|TRUE
  #gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|FALSE
  
[PcdsFeatureFlag.IA32]
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseTraverseEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseCallbackOnSetEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseExEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseGetSizeEnabled|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiPcdDatabaseSetEnabled|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseSerial|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseMemory|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseOEM|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSwitchToLongMode|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportEfiDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportTianoDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplSupportCustomDecompress|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxeIplBuildShareCodeHobs|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDxePcdDatabaseTraverseEnabled|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseHardSerial|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseEfiSerial|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseRuntimeMemory|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseDataHub|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeUseOEM|TRUE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInSerial|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInDataHub|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInRuntimeMemory|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdStatusCodeReplayInOEM|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdNtEmulatorEnable|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathToText|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdDevicePathSupportDevicePathFromText|TRUE
  gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable|FALSE
  gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdSupportUpdateCapsuleReset|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPciIsaEnable|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPciVgaEnable|FALSE
  gEfiIntelFrameworkModulePkgTokenSpaceGuid.PcdPciBusHotplugDeviceSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdPeiCoreImageLoaderSearchTeSectionFirst|FALSE

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamicDefault.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0
  gEfiMdeModulePkgTokenSpaceGuid.PcdFlashNvStorageVariableBase|0
  gEfiGenericPlatformTokenSpaceGuid.PcdFlashNvStorageVariableBase|0x0|UINT32|4
  gEfiGenericPlatformTokenSpaceGuid.PcdFlashNvStorageVariableSize|0x0|UINT32|4
  gEfiGenericPlatformTokenSpaceGuid.PcdFlashNvStorageFtwSpareBase|0x0|UINT32|4
  gEfiGenericPlatformTokenSpaceGuid.PcdFlashNvStorageFtwSpareSize|0x0|UINT32|4
  gEfiGenericPlatformTokenSpaceGuid.PcdFlashNvStorageFtwWorkingBase|0x0|UINT32|4
  gEfiGenericPlatformTokenSpaceGuid.PcdFlashNvStorageFtwWorkingSize|0x0|UINT32|4
  gEfiUnixPkgTokenSpaceGuid.PcdUnixConsole|L"Bus Driver Console Window"|VOID*|50
  gEfiUnixPkgTokenSpaceGuid.PcdUnixUga|L"UGA Window"|VOID*|50
  gEfiUnixPkgTokenSpaceGuid.PcdUnixFileSystem|L".!../../../../EdkShellBinPkg/Bin/Ia32/Apps"|VOID*|106
  gEfiUnixPkgTokenSpaceGuid.PcdUnixVirtualDisk|L"disk1.img:FW"|VOID*|24
  gEfiUnixPkgTokenSpaceGuid.PcdUnixPhysicalDisk|L"E:RW;245760;512"|VOID*|30
  gEfiUnixPkgTokenSpaceGuid.PcdUnixCpuModel|L"Intel(R) Processor Model"|VOID*|48
  gEfiUnixPkgTokenSpaceGuid.PcdUnixCpuSpeed|L"3000"|VOID*|8
  gEfiUnixPkgTokenSpaceGuid.PcdUnixMemorySize|L"64!64"|VOID*|10
  gEfiUnixPkgTokenSpaceGuid.PcdUnixSerialPort|L"/dev/ttyS0!/dev/ttyS1"|VOID*|20

[PcdsDynamicHii.common.DEFAULT]
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0x0053 0x0065 0x0074 0x0075 0x0070|gEfiUnixSystemConfigGuid|0x0|80 # Variable: L"Setup"
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0x0053 0x0065 0x0074 0x0075 0x0070|gEfiUnixSystemConfigGuid|0x4|25 # Variable: L"Setup"

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
[Components.IA32]
  ##
  #  SEC Phase modules
  ##
  UnixPkg/Sec/SecMain.inf

  ##
  #  PEI Phase modules
  ##
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf  {
   <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }
  IntelFrameworkModulePkg/Universal/StatusCode/Pei/PeiStatusCode.inf
  UnixPkg/BootModePei/BootModePei.inf
  UnixPkg/UnixFlashMapPei/FlashMap.inf
  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf
  UnixPkg/UnixAutoScanPei/UnixAutoScan.inf
  UnixPkg/UnixFirmwareVolumePei/UnixFwh.inf
  UnixPkg/UnixThunkPpiToProtocolPei/UnixThunkPpiToProtocol.inf
  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  
  ##
  #  DXE Phase modules
  ##
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  UnixPkg/MetronomeDxe/Metronome.inf
  UnixPkg/RealTimeClockRuntimeDxe/RealTimeClock.inf
  UnixPkg/ResetRuntimeDxe/Reset.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  UnixPkg/FvbServicesRuntimeDxe/UnixFwh.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
  IntelFrameworkModulePkg/Universal/DataHubDxe/DataHubDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  UnixPkg/UnixThunkDxe/UnixThunk.inf
  UnixPkg/CpuRuntimeDxe/Cpu.inf
  MdeModulePkg/Universal/FirmwareVolume/FaultTolerantWriteDxe/FtwLite.inf
  IntelFrameworkModulePkg/Universal/DataHubStdErrDxe/DataHubStdErrDxe.inf
  UnixPkg/MiscSubClassPlatformDxe/MiscSubClassDriver.inf
  UnixPkg/TimerDxe/Timer.inf
  IntelFrameworkModulePkg/Universal/StatusCode/Dxe/DxeStatusCode.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  IntelFrameworkModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  IntelFrameworkModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf
  IntelFrameworkModulePkg/Bus/Pci/IdeBusDxe/IdeBusDxe.inf
  UnixPkg/UnixBusDriverDxe/UnixBusDriver.inf
 
  UnixPkg/UnixBlockIoDxe/UnixBlockIo.inf
  UnixPkg/UnixSerialIoDxe/UnixSerialIo.inf
  UnixPkg/UnixUgaDxe/UnixUga.inf
  UnixPkg/UnixConsoleDxe/UnixConsole.inf
  UnixPkg/UnixSimpleFileSystemDxe/UnixSimpleFileSystem.inf
  MdeModulePkg/Application/HelloWorld/HelloWorld.inf

  #
  # Network stack drivers
  # To test network drivers, need network Io driver(SnpNt32Io.dll), please refer to NETWORK-IO Subproject.
  #
  MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf
  MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
  MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
  MdeModulePkg/Universal/Network/Ip4ConfigDxe/Ip4ConfigDxe.inf
  MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
  MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf
  MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  MdeModulePkg/Universal/DriverSampleDxe/DriverSampleDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

