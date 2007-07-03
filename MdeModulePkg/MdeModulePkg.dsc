#/** @file
# EFI/PI Reference Module Package for All Architectures
#
# This FPD file is used for Package Level build.
#
# Copyright (c) 2007, Intel Corporation
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
  PLATFORM_NAME                  = MdeModuleAll
  PLATFORM_GUID                  = 587CE499-6CBE-43cd-94E2-186218569478
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = $(WORKSPACE)/Build/MdeModule
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT



################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses.common]
  CacheMaintenanceLib|${WORKSPACE}/MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|${WORKSPACE}/MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|${WORKSPACE}/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciCf8Lib|${WORKSPACE}/MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|${WORKSPACE}/MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciLib|${WORKSPACE}/MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffGetEntryPoint|${WORKSPACE}/MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|${WORKSPACE}/MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PerformanceLib|${WORKSPACE}/MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PostCodeLib|${WORKSPACE}/MdePkg/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  PostCodeLib|${WORKSPACE}/MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  PrintLib|${WORKSPACE}/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|${WORKSPACE}/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiDecompressLib|${WORKSPACE}/MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf

[LibraryClasses.IA32]
  IoLib|${WORKSPACE}/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  BaseMemoryLib|${WORKSPACE}/MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[LibraryClasses.X64]
  IoLib|${WORKSPACE}/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  BaseMemoryLib|${WORKSPACE}/MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[LibraryClasses.IPF]
  IoLib|${WORKSPACE}/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.EBC]


[LibraryClasses.common.PEI_CORE]
  PeiCoreEntryPoint|${WORKSPACE}/MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  HobLib|${WORKSPACE}/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeimEntryPointLib|${WORKSPACE}/MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeiServiceLib|${WORKSPACE}/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|${WORKSPACE}/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf

[LibraryClasses.common.PEIM]
  HobLib|${WORKSPACE}/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeimEntryPoint|${WORKSPACE}/MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeiServicesLib|${WORKSPACE}/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|${WORKSPACE}/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  SmBusLib|${WORKSPACE}/MdePkg/Library/PeiSmbusLib/PeiSmbusLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DxeCoreEntryPoint|${WORKSPACE}/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  DxeServicesTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|${WORKSPACE}/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|${WORKSPACE}/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|${WORKSPACE}/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|${WORKSPACE}/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_SAL_DRIVER]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|${WORKSPACE}/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  HiiLib|${WORKSPACE}/MdePkg/Library/HiiLib/HiiLib.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|${WORKSPACE}/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|${WORKSPACE}/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  HiiLib|${WORKSPACE}/MdePkg/Library/HiiLib/HiiLib.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|${WORKSPACE}/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  HiiLib|${WORKSPACE}/MdePkg/Library/HiiLib/HiiLib.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|${WORKSPACE}/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  HobLib|${WORKSPACE}/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|${WORKSPACE}/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|${WORKSPACE}/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|${WORKSPACE}/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  HiiLib|${WORKSPACE}/MdePkg/Library/HiiLib/HiiLib.inf
  UefiApplicationEntryPoint|${WORKSPACE}/MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|${WORKSPACE}/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|${WORKSPACE}/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DebugLib|${WORKSPACE}/MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  FvbServiceLib|${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|${WORKSPACE}/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.IA32.BASE]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.SEC]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.PEI_CORE]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.PEIM]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_SAL_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.UEFI_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.UEFI_APPLICATION]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.BASE]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.SEC]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.PEI_CORE]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.PEIM]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.DXE_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.DXE_SAL_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.UEFI_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.UEFI_APPLICATION]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.BASE]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.SEC]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.PEI_CORE]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.PEIM]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.DXE_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.DXE_SAL_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.UEFI_DRIVER]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.UEFI_APPLICATION]
  TimerLib|${WORKSPACE}/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.X64.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.EBC.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|${WORKSPACE}/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag.common]
  PcdComponentNameDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnosticsDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdComponentName2Disable|gEfiMdePkgTokenSpaceGuid|TRUE
  PcdDriverDiagnostics2Disable|gEfiMdePkgTokenSpaceGuid|TRUE
  PcdSupportUpdateCapsuleRest|gEfiEdkModulePkgTokenSpaceGuid|FALSE
  PcdDxePcdDatabaseTraverseEnabled|gEfiEdkModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseTraverseEnabled|gEfiEdkModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseSetEnabled|gEfiEdkModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseGetSizeEnabled|gEfiEdkModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseCallbackOnSetEnabled|gEfiEdkModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseExEnabled|gEfiEdkModulePkgTokenSpaceGuid|TRUE
  PcdNtEmulatorEnable|gEfiEdkModulePkgTokenSpaceGuid|FALSE

#  PcdStatusCodeUseOEM|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE

[PcdsFixedAtBuild.common]
  PcdMaximumUnicodeStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumAsciiStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumLinkedListLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdSpinLockTimeout|gEfiMdePkgTokenSpaceGuid|10000000
  PcdDebugPropertyMask|gEfiMdePkgTokenSpaceGuid|0x0f
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdReportStatusCodePropertyMask|gEfiMdePkgTokenSpaceGuid|0x06
  PcdDebugClearMemoryValue|gEfiMdePkgTokenSpaceGuid|0xAF
  PcdPerformanceLibraryPropertyMask|gEfiMdePkgTokenSpaceGuid|0
  PcdPostCodePropertyMask|gEfiMdePkgTokenSpaceGuid|0
  PcdPciExpressBaseAddress|gEfiMdePkgTokenSpaceGuid|0xE0000000
  PcdFSBClock|gEfiMdePkgTokenSpaceGuid|200000000
  PcdUefiLibMaxPrintBufferSize|gEfiMdePkgTokenSpaceGuid|320
  PcdMaxSizeNonPopulateCapsule|gEfiEdkModulePkgTokenSpaceGuid|0x0
  PcdMaxSizePopulateCapsule|gEfiEdkModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwSpareBase|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwSpareSize|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwWorkingBase|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwWorkingSize|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdFlashNvStorageVariableBase|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdFlashNvStorageVariableSize|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdMaxPeiPerformanceLogEntries|gEfiEdkModulePkgTokenSpaceGuid|28
  PcdVpdBaseAddress|gEfiEdkModulePkgTokenSpaceGuid|0x0
  PcdMaxPeiPcdCallBackNumberPerPcdEntry|gEfiEdkModulePkgTokenSpaceGuid|0x08
  PcdStatusCodeValueEfiWatchDogTimerExpired|gEfiMdePkgTokenSpaceGuid|0x00011003   # EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_TIMER_EXPIRED
  PcdStatusCodeValueMemoryTestStarted|gEfiMdePkgTokenSpaceGuid|0x00051006         # EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP
  PcdStatusCodeValueSetVirtualAddressMap|gEfiMdePkgTokenSpaceGuid|0x03101004      # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST
  PcdStatusCodeValueUncorrectableMemoryError|gEfiMdePkgTokenSpaceGuid|0x0005100   # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE3

[PcdsPatchableInModule.common]
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdPciExpressBaseAddress|gEfiMdePkgTokenSpaceGuid|0xE0000000
  PcdFSBClock|gEfiMdePkgTokenSpaceGuid|200000000


################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################


[Components.Ia32]
  ${WORKSPACE}/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ${WORKSPACE}/MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  ${WORKSPACE}/MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  ${WORKSPACE}/MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  ${WORKSPACE}/MdeModulePkg/Library/EdkDxePrintLib/EdkDxePrintLib.inf
  ${WORKSPACE}/MdeModulePkg/Application/HelloWorld/HelloWorld.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Disk/DiskIo/Dxe/DiskIo.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Disk/Partition/Dxe/Partition.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Disk/UnicodeCollation/English/Dxe/English.inf
  ${WORKSPACE}/MdeModulePkg/Universal/SecurityStubDxe/SecurityStub.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Capsule/RuntimeDxe/CapsuleRuntime.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Ebc/Dxe/Ebc.inf
  ${WORKSPACE}/MdeModulePkg/Universal/GenericMemoryTest/Dxe/NullMemoryTest.inf
  ${WORKSPACE}/MdeModulePkg/Universal/FirmwareVolume/FaultTolerantWriteLite/Dxe/FtwLite.inf
  ${WORKSPACE}/MdeModulePkg/Universal/BaseMemoryTestPei/BaseMemoryTest.inf
  ${WORKSPACE}/MdeModulePkg/Universal/FirmwareVolume/GuidedSectionExtraction/Crc32SectionExtract/Dxe/Crc32SectionExtract.inf
  ${WORKSPACE}/MdeModulePkg/Universal/VariableRuntimeDxe/Variable.inf
  ${WORKSPACE}/MdeModulePkg/Universal/RuntimeDxe/Runtime.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Pci/AtapiPassThruDxe/AtapiPassThru.inf
  ${WORKSPACE}/MdeModulePkg/Universal/WatchDogTimerDxe/WatchDogTimer.inf
  ${WORKSPACE}/MdeModulePkg/Universal/VariablePei/Variable.inf
  ${WORKSPACE}/MdeModulePkg/Universal/VariableRuntimeDxe/Variable.inf
  ${WORKSPACE}/MdeModulePkg/Universal/DebugSupportDxe/DebugSupport.inf
  ${WORKSPACE}/MdeModulePkg/Universal/DebugPortDxe/DebugPort.inf
  ${WORKSPACE}/MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBus.inf
  ${WORKSPACE}/MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDisk.inf
  $(WORKSPACE)/MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  $(WORKSPACE)/MdeModulePkg/Universal/PCD/Pei/Pcd.inf

[Components.X64]
  ${WORKSPACE}/MdeModulePkg/Application/HelloWorld/HelloWorld.inf

[Components.IPF]
  ${WORKSPACE}/MdeModulePkg/Application/HelloWorld/HelloWorld.inf

[Components.EBC]
  ${WORKSPACE}/MdeModulePkg/Application/HelloWorld/HelloWorld.inf
