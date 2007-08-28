#/** @file
# EFI/PI Reference Module Package for All Architectures
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

[Defines]
  PLATFORM_NAME                  = MdeModule
  PLATFORM_GUID                  = 587CE499-6CBE-43cd-94E2-186218569478
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MdeModule
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT



[LibraryClasses.common]
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PeCoffGetEntryPoint|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PostCodeLib|MdePkg/Library/BasePostCodeLibDebug/BasePostCodeLibDebug.inf
  PostCodeLib|MdePkg/Library/BasePostCodeLibPort80/BasePostCodeLibPort80.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
  PeCoffLoaderLib|MdeModulePkg/Library/PeiDxePeCoffLoaderLib/PeCoffLoaderLib.inf
  CustomDecompressLib|MdePkg/Library/BaseCustomDecompressLibNull/BaseCustomDecompressLibNull.inf

[LibraryClasses.IA32]
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[LibraryClasses.X64]
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf

[LibraryClasses.IPF]
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.EBC.DXE_DRIVER]
  IoLib|IntelFrameworkPkg/Library/DxeIoLibCpuIo/DxeIoLibCpuIo.inf

[LibraryClasses.EBC.PEIM]
  IoLib|MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf

[LibraryClasses.common.PEI_CORE]
  PeiCoreEntryPoint|MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  OldPeiCoreEntryPoint|MdePkg/Library/OldPeiCoreEntryPoint/OldPeiCoreEntryPoint.inf

[LibraryClasses.common.PEIM]
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  SmBusLib|MdePkg/Library/PeiSmbusLibSmbus2Ppi/PeiSmbusLibSmbus2Ppi.inf

[LibraryClasses.common.DXE_CORE]
  HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PeCoffLoaderLib|MdeModulePkg/Library/PeiDxePeCoffLoaderLib/PeCoffLoaderLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf
  HiiLibFramework|IntelFrameworkPkg/Library/HiiLibFramework/HiiLib.inf
  UsbLib|MdePkg/Library/UefiUsbLib/UefiUsbLib.inf
  NetLib|MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  IpIoLib|MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  UdpIoLib|MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ReportStatusCodeLib|IntelFrameworkModulePkg/Library/DxeReportStatusCodeLibFramework/DxeReportStatusCodeLib.inf
  ScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_SAL_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SmbusLib|MdePkg/Library/DxeSmbusLib/DxeSmbusLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  MemoryAllocationLib|MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  DxeServiceTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  FvbServiceLib|MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  ScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[LibraryClasses.IA32.BASE]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.SEC]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.PEI_CORE]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.PEIM]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_SAL_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.UEFI_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.UEFI_APPLICATION]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.BASE]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.SEC]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.PEI_CORE]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.PEIM]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.DXE_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.DXE_SAL_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.UEFI_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.X64.UEFI_APPLICATION]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.BASE]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.SEC]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.PEI_CORE]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.PEIM]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.DXE_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.DXE_SAL_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.UEFI_DRIVER]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IPF.UEFI_APPLICATION]
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.X64.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.EBC.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf

[LibraryClasses.IPF.DXE_RUNTIME_DRIVER]
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  
[LibraryClasses.EBC.PEI_CORE]
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

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

  PcdUartDefaultBaudRate|gEfiMdePkgTokenSpaceGuid|115200
  PcdUartDefaultDataBits|gEfiMdePkgTokenSpaceGuid|8
  PcdUartDefaultParity|gEfiMdePkgTokenSpaceGuid|1
  PcdUartDefaultStopBits|gEfiMdePkgTokenSpaceGuid|1
  PcdDefaultTerminalType|gEfiMdePkgTokenSpaceGuid|0


[PcdsFixedAtBuild.IPF]
  PcdIoBlockBaseAddressForIpf|gEfiMdePkgTokenSpaceGuid|0x0ffffc000000

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.common]
  MdeModulePkg/Core/Pei/PeiMain.inf
  MdeModulePkg/Core/Dxe/DxeMain.inf
  MdeModulePkg/Library/DxeCorePerformanceLib/DxeCorePerformanceLib.inf
  MdeModulePkg/Library/DxePerformanceLib/DxePerformanceLib.inf
  MdeModulePkg/Library/PeiPerformanceLib/PeiPerformanceLib.inf
  MdeModulePkg/Library/EdkDxePrintLib/EdkDxePrintLib.inf

  MdeModulePkg/Library/DxeNetLib/DxeNetLib.inf
  MdeModulePkg/Library/DxeIpIoLib/DxeIpIoLib.inf
  MdeModulePkg/Library/DxeUdpIoLib/DxeUdpIoLib.inf

  MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
  MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
  MdeModulePkg/Universal/Network/Ip4ConfigDxe/Ip4ConfigDxe.inf
  MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
  MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  MdeModulePkg/Universal/Network/PxeBcDxe/PxeBcDxe.inf
  MdeModulePkg/Universal/Network/PxeDhcp4Dxe/PxeDhcp4Dxe.inf
  MdeModulePkg/Universal/Network/SnpDxe/SnpDxe.inf
  MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf
  MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf

  MdeModulePkg/Application/HelloWorld/HelloWorld.inf

  MdeModulePkg/Bus/Pci/AtapiPassThruDxe/AtapiPassThru.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  MdeModulePkg/Core/DxeIplPeim/DxeIpl.inf
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf

  MdeModulePkg/Universal/FirmwareVolume/FaultTolerantWriteDxe/FtwLite.inf
  MdeModulePkg/Universal/MemoryTest/NullMemoryTestDxe/NullMemoryTestDxe.inf
  MdeModulePkg/Universal/MemoryTest/BaseMemoryTestPei/BaseMemoryTestPei.inf
  MdeModulePkg/Universal/FirmwareVolume/Crc32SectionExtractDxe/Crc32SectionExtractDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf

  MdeModulePkg/Universal/WatchDogTimerDxe/WatchDogTimer.inf
  MdeModulePkg/Universal/DebugPortDxe/DebugPortDxe.inf
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf
  MdeModulePkg/Universal/PCD/Pei/Pcd.inf
  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/PcatSingleSegmentPciCfg2Pei/PcatSingleSegmentPciCfg2Pei.inf
  MdeModulePkg/Application/HelloWorld/HelloWorld.inf
  MdeModulePkg/Bus/Pci/EhciDxe/EhciDxe.inf
  MdeModulePkg/Bus/Pci/UhciDxe/UhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf

  MdeModulePkg/Universal/Variable/Pei/VariablePei.inf

[Components.IA32]
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Bus/Pci/UndiRuntimeDxe/UndiRuntimeDxe.inf

[Components.X64]
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf
  MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  MdeModulePkg/Universal/Variable/EmuRuntimeDxe/EmuVariableRuntimeDxe.inf
  MdeModulePkg/Bus/Pci/UndiRuntimeDxe/UndiRuntimeDxe.inf

[Components.IPF]
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/EbcDxe/EbcDxe.inf
  MdeModulePkg/Universal/DebugSupportDxe/DebugSupportDxe.inf

[Components.EBC]
  #BugBug: Need DXE I/O library instance for EBC.
  #MdeModulePkg/Universal/PcatRealTimeClockRuntimeDxe/PcatRealTimeClockRuntimeDxe.inf
  MdeModulePkg/Bus/Pci/UndiRuntimeDxe/UndiRuntimeDxe.inf

