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
  PLATFORM_NAME                  = MdeModule
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
  CacheMaintenanceLib|$(WORKSPACE)/MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|$(WORKSPACE)/MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciCf8Lib|$(WORKSPACE)/MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|$(WORKSPACE)/MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciLib|$(WORKSPACE)/MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffGetEntryPoint|$(WORKSPACE)/MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|$(WORKSPACE)/MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PerformanceLib|$(WORKSPACE)/MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PostCodeLib|$(WORKSPACE)/MdePkg/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  PostCodeLib|$(WORKSPACE)/MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiDecompressLib|$(WORKSPACE)/MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  PeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/PeiDxePeCoffLoaderLib/PeCoffLoaderLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  CustomDecompressLib|$(WORKSPACE)/IntelFrameworkModulePkg/Library/BaseUefiTianoCustomDecompressLib/BaseUefiTianoCustomDecompressLib.inf
  HiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLibFramework/HiiLib.inf


[LibraryClasses.IA32]
  IoLib|$(WORKSPACE)/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[LibraryClasses.X64]
  IoLib|$(WORKSPACE)/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[LibraryClasses.IPF]
  IoLib|$(WORKSPACE)/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.EBC]


[LibraryClasses.common.PEI_CORE]
  PeiCoreEntryPoint|$(WORKSPACE)/MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeiServicesLib|$(WORKSPACE)/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeiServicesLib|$(WORKSPACE)/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeCoffGetEntryPointLib|$(WORKSPACE)/MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  OldPeiCoreEntryPoint|$(WORKSPACE)/MdePkg/Library/OldPeiCoreEntryPoint/OldPeiCoreEntryPoint.inf

[LibraryClasses.common.PEIM]
  HobLib|$(WORKSPACE)/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeimEntryPoint|$(WORKSPACE)/MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|$(WORKSPACE)/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  SmBusLib|$(WORKSPACE)/MdePkg/Library/PeiSmbusLib/PeiSmbusLib.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DxeCoreEntryPoint|$(WORKSPACE)/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  PeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/PeiDxePeCoffLoaderLib/PeCoffLoaderLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|$(WORKSPACE)/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/FrameworkHiiLib/HiiLib.inf
  UsbLib|$(WORKSPACE)/MdePkg/Library/UefiUsbLib/UefiUsbLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|$(WORKSPACE)/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_SAL_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|$(WORKSPACE)/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  HiiLib|$(WORKSPACE)/MdePkg/Library/HiiLib/HiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|$(WORKSPACE)/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  HiiLib|$(WORKSPACE)/MdePkg/Library/HiiLib/HiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|$(WORKSPACE)/MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  HiiLib|$(WORKSPACE)/MdePkg/Library/HiiLib/HiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  HiiLib|$(WORKSPACE)/MdePkg/Library/HiiLib/HiiLib.inf
  UefiApplicationEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  FvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/IntelFrameworkPkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|$(WORKSPACE)/MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.IA32.BASE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.SEC]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.PEI_CORE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.PEIM]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_SAL_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.UEFI_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.UEFI_APPLICATION]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.BASE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.SEC]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.PEI_CORE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.PEIM]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.DXE_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.DXE_SAL_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.UEFI_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.UEFI_APPLICATION]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.BASE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.SEC]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.PEI_CORE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.PEIM]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.DXE_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.DXE_SAL_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.UEFI_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.UEFI_APPLICATION]
  TimerLib|$(WORKSPACE)/MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.X64.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.EBC.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFeatureFlag.common]
  PcdSupportUpdateCapsuleRest|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdComponentNameDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnosticsDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdComponentName2Disable|gEfiMdePkgTokenSpaceGuid|TRUE
  PcdDriverDiagnostics2Disable|gEfiMdePkgTokenSpaceGuid|TRUE
  PcdSupportUpdateCapsuleRest|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDxePcdDatabaseTraverseEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseTraverseEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseSetEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseGetSizeEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseCallbackOnSetEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseExEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdNtEmulatorEnable|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDevicePathSupportDevicePathFromText|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDevicePathSupportDevicePathToText|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDxeIplSupportCustomDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplBuildShareCodeHobs|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDxeIplSupportEfiDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplSupportTianoDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplSupportCustomDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE

#  PcdStatusCodeUseOEM|gEfiIntelFrameworkModulePkgTokenSpaceGuid|FALSE

[PcdsFeatureFlag.IA32]
  PcdDxeIplSwitchToLongMode|gEfiMdeModulePkgTokenSpaceGuid|TRUE

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
  PcdMaxSizeNonPopulateCapsule|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdMaxSizePopulateCapsule|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwSpareBase|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwSpareSize|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwWorkingBase|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageFtwWorkingSize|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageVariableBase|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdFlashNvStorageVariableSize|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdMaxPeiPerformanceLogEntries|gEfiMdeModulePkgTokenSpaceGuid|28
  PcdVpdBaseAddress|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdMaxPeiPcdCallBackNumberPerPcdEntry|gEfiMdeModulePkgTokenSpaceGuid|0x08
  PcdStatusCodeValueEfiWatchDogTimerExpired|gEfiMdePkgTokenSpaceGuid|0x00011003   # EFI_COMPUTING_UNIT_HOST_PROCESSOR | EFI_CU_HP_EC_TIMER_EXPIRED
  PcdStatusCodeValueMemoryTestStarted|gEfiMdePkgTokenSpaceGuid|0x00051006         # EFI_SOFTWARE_EFI_BOOT_SERVICE | EFI_SW_RS_PC_SET_VIRTUAL_ADDRESS_MAP
  PcdStatusCodeValueSetVirtualAddressMap|gEfiMdePkgTokenSpaceGuid|0x03101004      # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_PC_TEST
  PcdStatusCodeValueUncorrectableMemoryError|gEfiMdePkgTokenSpaceGuid|0x0005100   # EFI_COMPUTING_UNIT_MEMORY | EFI_CU_MEMORY_EC_UNCORRECTABLE3
  PcdStatusCodeValueRemoteConsoleError|gEfiMdePkgTokenSpaceGuid|0x01040006        # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_CONTROLLER_ERROR
  PcdStatusCodeValueRemoteConsoleReset|gEfiMdePkgTokenSpaceGuid|0x01040001        # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_PC_RESET
  PcdStatusCodeValueRemoteConsoleInputError|gEfiMdePkgTokenSpaceGuid|0x01040007   # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_INPUT_ERROR
  PcdStatusCodeValueRemoteConsoleOutputError|gEfiMdePkgTokenSpaceGuid|0x01040008  # EFI_PERIPHERAL_REMOTE_CONSOLE | EFI_P_EC_OUTPUT_ERROR

  PcdStatusCodeValueMouseInterfaceError|gEfiMdePkgTokenSpaceGuid|0x01020005       # EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR
  PcdStatusCodeValueMouseEnable|gEfiMdePkgTokenSpaceGuid|0x01020004               # EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE
  PcdStatusCodeValueMouseDisable|gEfiMdePkgTokenSpaceGuid|0x01020002              # EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE
  PcdStatusCodeValueKeyboardEnable|gEfiMdePkgTokenSpaceGuid|0x01010004            # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE
  PcdStatusCodeValueKeyboardPresenceDetect|gEfiMdePkgTokenSpaceGuid|0x01010003    # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT
  PcdStatusCodeValueKeyboardDisable|gEfiMdePkgTokenSpaceGuid|0x01010002           # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE
  PcdStatusCodeValueKeyboardReset|gEfiMdePkgTokenSpaceGuid|0x01010001             # EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET
  PcdStatusCodeValueKeyboardClearBuffer|gEfiMdePkgTokenSpaceGuid|0x01011000       # EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER
  PcdStatusCodeValueKeyboardSelfTest|gEfiMdePkgTokenSpaceGuid|0x01011001          # EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST
  PcdStatusCodeValueKeyboardInterfaceError|gEfiMdePkgTokenSpaceGuid|0x01010005    # EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INTERFACE_ERROR
  PcdStatusCodeValueKeyboardInputError|gEfiMdePkgTokenSpaceGuid|0x01010007        # EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR
  PcdStatusCodeValueMouseInputError|gEfiMdePkgTokenSpaceGuid|0x01020007           # EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR
  PcdStatusCodeValueMouseReset|gEfiMdePkgTokenSpaceGuid|0x01020001                # EFI_PERIPHERAL_MOUSE | EFI_P_PC_RESET

[PcdsFixedAtBuild.IPF]
  PcdIoBlockBaseAddressForIpf|gEfiMdePkgTokenSpaceGuid|0x0ffffc000000

[PcdsPatchableInModule.common]
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdPciExpressBaseAddress|gEfiMdePkgTokenSpaceGuid|0xE0000000
  PcdFSBClock|gEfiMdePkgTokenSpaceGuid|200000000

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  $(WORKSPACE)/MdeModulePkg/Core/Pei/PeiMain.inf
  $(WORKSPACE)/MdeModulePkg/Core/Dxe/DxeMain.inf
  $(WORKSPACE)/MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  $(WORKSPACE)/MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  $(WORKSPACE)/MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  $(WORKSPACE)/MdeModulePkg/Library/EdkDxePrintLib/EdkDxePrintLib.inf

  $(WORKSPACE)/MdeModulePkg/Application/HelloWorld/HelloWorld.inf

  $(WORKSPACE)/MdeModulePkg/Bus/Pci/AtapiPassThruDxe/AtapiPassThru.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  $(WORKSPACE)/MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf

  $(WORKSPACE)/MdeModulePkg/Universal/FirmwareVolume/FaultTolerantWriteDxe/FtwLite.inf
  $(WORKSPACE)/MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/MemoryTest/BaseMemoryTestPei/BaseMemoryTestPei.inf
  $(WORKSPACE)/MdeModulePkg/Universal/FirmwareVolume/Crc32SectionExtractDxe/Crc32SectionExtractDxe.inf
  $(WORKSPACE)/MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf

  $(WORKSPACE)/MdeModulePkg/Universal/WatchDogTimerDxe/WatchDogTimer.inf
  $(WORKSPACE)/MdeModulePkg/Universal/DebugPortDxe/DebugPortDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  $(WORKSPACE)/MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  $(WORKSPACE)/MdeModulePkg/Application/HelloWorld/HelloWorld.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  $(WORKSPACE)/MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf

  $(WORKSPACE)/MdeModulePkg/Universal/Variable/Pei/VariablePei.inf

[Components.Ia32]
  $(WORKSPACE)/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf

[Components.X64]
  $(WORKSPACE)/MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf

[Components.IPF]
  $(WORKSPACE)/MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  $(WORKSPACE)/MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf

[Components.EBC]
  $(WORKSPACE)/MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf

