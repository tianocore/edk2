#/** @file
# EFI/Framework Emulation Platform
#
# The Emulation Platform can be used to debug individual modules, prior to creating
#    a real platform. This also provides an example for how an FPD is created.
#
# Copyright (c) 2006 - 2007, Intel Corporation
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
  PLATFORM_NAME                  = NT32
  PLATFORM_GUID                  = EB216561-961F-47EE-9EF9-CA426EF547C2
  PLATFORM_VERSION               = 0.3
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = $(WORKSPACE)/Build/Nt32Platform
  SUPPORTED_ARCHITECTURES        = IA32
  BUILD_TARGETS                  = DEBUG
  SKUID_IDENTIFIER               = DEFAULT

  DEBUG_ICC_IA32_DLINK_FLAGS                  = /EXPORT:InitializeDriver=_ModuleEntryPoint /ALIGN:4096 /SUBSYSTEM:CONSOLE
  DEBUG_VS2003_IA32_DLINK_FLAGS               = /EXPORT:InitializeDriver=_ModuleEntryPoint /ALIGN:4096 /SUBSYSTEM:CONSOLE
  DEBUG_MYTOOLS_IA32_DLINK_FLAGS              = /EXPORT:InitializeDriver=_ModuleEntryPoint /ALIGN:4096 /SUBSYSTEM:CONSOLE
  DEBUG_WINDDK3790x1830_IA32_DLINK_FLAGS      = /EXPORT:InitializeDriver=_ModuleEntryPoint /ALIGN:4096 /SUBSYSTEM:CONSOLE
  DEBUG_VS2005PRO_IA32_DLINK_FLAGS            = /EXPORT:InitializeDriver=_ModuleEntryPoint /ALIGN:4096 /SUBSYSTEM:CONSOLE
  DEBUG_MIXED_IA32_DLINK_FLAGS                = /EXPORT:InitializeDriver=_ModuleEntryPoint /ALIGN:4096 /SUBSYSTEM:CONSOLE
  RELEASE_ICC_IA32_DLINK_FLAGS                = /ALIGN:4096
  RELEASE_VS2003_IA32_DLINK_FLAGS             = /ALIGN:4096
  RELEASE_MYTOOLS_IA32_DLINK_FLAGS            = /ALIGN:4096
  RELEASE_WINDDK3790x1830_IA32_DLINK_FLAGS    = /ALIGN:4096
  RELEASE_VS2005PRO_IA32_DLINK_FLAGS          = /ALIGN:4096
  RELEASE_MIXED_IA32_DLINK_FLAGS              = /ALIGN:4096


################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

[LibraryClasses.IA32.DXE_CORE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/Nt32Pkg/Library/Nt32PeCoffLoaderLib/Nt32PeCoffLoaderLib.inf

[LibraryClasses.IA32]
  IoLib|$(WORKSPACE)/MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf

[LibraryClasses.IA32.DXE_SAL_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  EdkFvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf

[LibraryClasses.IA32.DXE_SMM_DRIVER]
  WinNtLib|$(WORKSPACE)/Nt32Pkg/Library/DxeWinNtLib/DxeWinNtLib.inf
  EdkFvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf

[LibraryClasses.IA32.PEIM]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/Nt32Pkg/Library/Nt32PeCoffLoaderLib/Nt32PeCoffLoaderLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/PeiNt32OemHookStatusCodeLib/PeiNt32OemHookStatusCodeLib.inf
  PeCoffGetEntryPointLib|$(WORKSPACE)/Nt32Pkg/Library/EdkNt32PeiPeCoffGetEntryPointLib/EdkNt32PeiPeCoffGetEntryPointLib.inf

[LibraryClasses.IA32.PEI_CORE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLibMm7/PeiServicesTablePointerLibMm7.inf
  PeCoffGetEntryPointLib|$(WORKSPACE)/Nt32Pkg/Library/EdkNt32PeiPeCoffGetEntryPointLib/EdkNt32PeiPeCoffGetEntryPointLib.inf

[LibraryClasses.IA32.DXE_RUNTIME_DRIVER]
  WinNtLib|$(WORKSPACE)/Nt32Pkg/Library/DxeWinNtLib/DxeWinNtLib.inf
  EdkFvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf

[LibraryClasses.IA32.BASE]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.IA32.SEC]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.IA32.UEFI_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  WinNtLib|$(WORKSPACE)/Nt32Pkg/Library/DxeWinNtLib/DxeWinNtLib.inf
  EdkFvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf

[LibraryClasses.IA32.DXE_DRIVER]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  WinNtLib|$(WORKSPACE)/Nt32Pkg/Library/DxeWinNtLib/DxeWinNtLib.inf
  EdkFvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf
  EdkGenericBdsLib|$(WORKSPACE)/Nt32Pkg/Library/EdkGenericBdsLib/EdkGenericBdsLib.inf

[LibraryClasses.IA32.UEFI_APPLICATION]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  WinNtLib|$(WORKSPACE)/Nt32Pkg/Library/DxeWinNtLib/DxeWinNtLib.inf
  EdkFvbServiceLib|$(WORKSPACE)/MdeModulePkg/Library/EdkFvbServiceLib/EdkFvbServiceLib.inf
  OemHookStatusCodeLib|$(WORKSPACE)/Nt32Pkg/Library/DxeNt32OemHookStatusCodeLib/DxeNt32OemHookStatusCodeLib.inf

[LibraryClasses.common.DXE_CORE]
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
  DxeCoreEntryPoint|$(WORKSPACE)/MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf
  CustomDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreCustomDecompressLibFromHob/DxeCoreCustomDecompressLibFromHob.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf

[LibraryClasses.common]
  TimerLib|$(WORKSPACE)/MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/BaseUefiTianoDecompressLib/BaseUefiTianoDecompressLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PeCoffGetEntryPointLib|$(WORKSPACE)/MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  SerialPortLib|$(WORKSPACE)/MdeModulePkg/Library/EdkSerialPortLibNull/EdkSerialPortLibNull.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  BaseLib|$(WORKSPACE)/MdePkg/Library/BaseLib/BaseLib.inf
  CustomDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/BaseCustomDecompressLibNull/BaseCustomDecompressLibNull.inf
  PerformanceLib|$(WORKSPACE)/MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffLib|$(WORKSPACE)/MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PciIncompatibleDeviceSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkPciIncompatibleDeviceSupportLib/EdkPciIncompatibleDeviceSupportLib.inf
  CacheMaintenanceLib|$(WORKSPACE)/MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf

[LibraryClasses.common.DXE_SAL_DRIVER]
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  EdkIfrSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkIfrSupportLib/EdkIfrSupportLib.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  EdkGraphicsLib|$(WORKSPACE)/MdeModulePkg/Library/EdkGraphicsLib/EdkGraphicsLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLib/FrameworkHiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiRuntimeLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf
  EdkUsbLib|$(WORKSPACE)/MdeModulePkg/Library/EdkUsbLib/EdkUsbLib.inf
  EdkScsiLib|$(WORKSPACE)/MdeModulePkg/Library/EdkScsiLib/EdkScsiLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  EdkUsbLib|$(WORKSPACE)/MdeModulePkg/Library/EdkUsbLib/EdkUsbLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  EdkScsiLib|$(WORKSPACE)/MdeModulePkg/Library/EdkScsiLib/EdkScsiLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLib/FrameworkHiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  EdkIfrSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkIfrSupportLib/EdkIfrSupportLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  EdkGraphicsLib|$(WORKSPACE)/MdeModulePkg/Library/EdkGraphicsLib/EdkGraphicsLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

[LibraryClasses.common.PEIM]
  HobLib|$(WORKSPACE)/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  IoLib|$(WORKSPACE)/MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PeimEntryPoint|$(WORKSPACE)/MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|$(WORKSPACE)/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

[LibraryClasses.common.PEI_CORE]
  HobLib|$(WORKSPACE)/MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|$(WORKSPACE)/MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  IoLib|$(WORKSPACE)/MdePkg/Library/PeiIoLibCpuIo/PeiIoLibCpuIo.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PeiServicesLib|$(WORKSPACE)/MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  PeiCoreEntryPoint|$(WORKSPACE)/MdePkg/Library/PeiCoreEntryPoint/PeiCoreEntryPoint.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  EdkIfrSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkIfrSupportLib/EdkIfrSupportLib.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  EdkGraphicsLib|$(WORKSPACE)/MdeModulePkg/Library/EdkGraphicsLib/EdkGraphicsLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLib/FrameworkHiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiRuntimeLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf
  EdkUsbLib|$(WORKSPACE)/MdeModulePkg/Library/EdkUsbLib/EdkUsbLib.inf
  EdkScsiLib|$(WORKSPACE)/MdeModulePkg/Library/EdkScsiLib/EdkScsiLib.inf
  UefiDriverModelLib|$(WORKSPACE)/MdePkg/Library/UefiDriverModelLib/UefiDriverModelLib.inf

[LibraryClasses.common.UEFI_DRIVER]
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  EdkIfrSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkIfrSupportLib/EdkIfrSupportLib.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  EdkGraphicsLib|$(WORKSPACE)/MdeModulePkg/Library/EdkGraphicsLib/EdkGraphicsLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLib/FrameworkHiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf
  EdkUsbLib|$(WORKSPACE)/MdeModulePkg/Library/EdkUsbLib/EdkUsbLib.inf
  EdkScsiLib|$(WORKSPACE)/MdeModulePkg/Library/EdkScsiLib/EdkScsiLib.inf
  UefiDriverModelLib|$(WORKSPACE)/MdePkg/Library/UefiDriverModelLib/UefiDriverModelLib.inf

[LibraryClasses.common.DXE_DRIVER]
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  EdkIfrSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkIfrSupportLib/EdkIfrSupportLib.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  UefiDriverEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  EdkGraphicsLib|$(WORKSPACE)/MdeModulePkg/Library/EdkGraphicsLib/EdkGraphicsLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLib/FrameworkHiiLib.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf
  EdkUsbLib|$(WORKSPACE)/MdeModulePkg/Library/EdkUsbLib/EdkUsbLib.inf
  EdkScsiLib|$(WORKSPACE)/MdeModulePkg/Library/EdkScsiLib/EdkScsiLib.inf
  UefiDriverModelLib|$(WORKSPACE)/MdePkg/Library/UefiDriverModelLib/UefiDriverModelLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  EdkUsbLib|$(WORKSPACE)/MdeModulePkg/Library/EdkUsbLib/EdkUsbLib.inf
  DebugLib|$(WORKSPACE)/MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DxeServicesTableLib|$(WORKSPACE)/MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PrintLib|$(WORKSPACE)/MdePkg/Library/BasePrintLib/BasePrintLib.inf
  EdkScsiLib|$(WORKSPACE)/MdeModulePkg/Library/EdkScsiLib/EdkScsiLib.inf
  UefiDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreUefiDecompressLibFromHob/DxeCoreUefiDecompressLibFromHob.inf
  FrameworkHiiLib|$(WORKSPACE)/IntelFrameworkPkg/Library/HiiLib/FrameworkHiiLib.inf
  UefiApplicationEntryPoint|$(WORKSPACE)/MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  HobLib|$(WORKSPACE)/MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DevicePathLib|$(WORKSPACE)/MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  EdkIfrSupportLib|$(WORKSPACE)/MdeModulePkg/Library/EdkIfrSupportLib/EdkIfrSupportLib.inf
  BaseMemoryLib|$(WORKSPACE)/MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  EdkPeCoffLoaderLib|$(WORKSPACE)/MdeModulePkg/Library/EdkDxePeCoffLoaderFromHobLib/EdkDxePeCoffLoaderFromHobLib.inf
  PcdLib|$(WORKSPACE)/MdePkg/Library/DxePcdLib/DxePcdLib.inf
  MemoryAllocationLib|$(WORKSPACE)/MdePkg/Library/DxeMemoryAllocationLib/DxeMemoryAllocationLib.inf
  EdkGraphicsLib|$(WORKSPACE)/MdeModulePkg/Library/EdkGraphicsLib/EdkGraphicsLib.inf
  UefiLib|$(WORKSPACE)/MdePkg/Library/UefiLib/UefiLib.inf
  ReportStatusCodeLib|$(WORKSPACE)/MdePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  TianoDecompressLib|$(WORKSPACE)/MdeModulePkg/Library/DxeCoreTianoDecompressLibFromHob/DxeCoreTianoDecompressLibFromHob.inf
  UefiRuntimeServicesTableLib|$(WORKSPACE)/MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsPatchableInModule.IA32]
  PcdStatusCodeMemorySize|gEfiMdeModulePkgTokenSpaceGuid|1
  PcdStatusCodeRuntimeMemorySize|gEfiMdeModulePkgTokenSpaceGuid|128

[PcdsFixedAtBuild.IA32]
  PcdWinNtMemorySizeForSecMain|gEfiNt32PkgTokenSpaceGuid|L"64!64"|VOID*|10
  PcdWinNtFirmwareVolume|gEfiNt32PkgTokenSpaceGuid|L"..\\Fv\\Fv_Recovery.fd"|VOID*|52
  PcdWinNtBootMode|gEfiNt32PkgTokenSpaceGuid|1
  PcdMaximumUnicodeStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumAsciiStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumLinkedListLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdSpinLockTimeout|gEfiMdePkgTokenSpaceGuid|10000000
  PcdMaximumAsciiStringLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdMaximumLinkedListLength|gEfiMdePkgTokenSpaceGuid|1000000
  PcdSpinLockTimeout|gEfiMdePkgTokenSpaceGuid|10000000
  PcdReportStatusCodePropertyMask|gEfiMdePkgTokenSpaceGuid|0x06
  PcdDebugPropertyMask|gEfiMdePkgTokenSpaceGuid|0x1f
  PcdDebugClearMemoryValue|gEfiMdePkgTokenSpaceGuid|0xAF
  PcdDebugPrintErrorLevel|gEfiMdePkgTokenSpaceGuid|0x80000000
  PcdPerformanceLibraryPropertyMask|gEfiMdePkgTokenSpaceGuid|0
  PcdMaxPeiPcdCallBackNumberPerPcdEntry|gEfiMdeModulePkgTokenSpaceGuid|0x08
  PcdVpdBaseAddress|gEfiMdeModulePkgTokenSpaceGuid|0x0
  PcdMaxSizeNonPopulateCapsule|gEfiEdkModulePkgTokenSpaceGuid|0x0
  PcdMaxSizePopulateCapsule|gEfiEdkModulePkgTokenSpaceGuid|0x0


[PcdsFeatureFlag.IA32]
  PcdPeiPcdDatabaseTraverseEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseCallbackOnSetEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseExEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseGetSizeEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPeiPcdDatabaseSetEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdStatusCodeUseSerial|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseMemory|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseOEM|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplSwitchToLongMode|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDxeIplSupportEfiDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplSupportTianoDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplSupportCustomDecompress|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxeIplBuildShareCodeHobs|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDxePcdDatabaseTraverseEnabled|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdStatusCodeUseHardSerial|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseEfiSerial|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseRuntimeMemory|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseDataHub|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeUseOEM|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdStatusCodeReplayInSerial|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInDataHub|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInRuntimeMemory|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdStatusCodeReplayInOEM|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdSupportUpdateCapsuleRest|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdComponentNameDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdDriverDiagnosticsDisable|gEfiMdePkgTokenSpaceGuid|FALSE
  PcdNtEmulatorEnable|gEfiMdeModulePkgTokenSpaceGuid|FALSE
  PcdDevicePathSupportDevicePathToText|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdDevicePathSupportDevicePathFromText|gEfiMdeModulePkgTokenSpaceGuid|TRUE
  PcdPciBusHotplugDeviceSupport|gEfiGenericPlatformTokenSpaceGuid|TRUE
  PcdComponentName2Disable|gEfiMdePkgTokenSpaceGuid|TRUE
  PcdUefiLibMaxPrintBufferSize|gEfiMdePkgTokenSpaceGuid|320
  PcdDriverDiagnostics2Disable|gEfiMdePkgTokenSpaceGuid|TRUE
  PcdSupportUpdateCapsuleRest|gEfiEdkModulePkgTokenSpaceGuid|FALSE

################################################################################
#
# Pcd Dynamic Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsDynamic.common]
  PcdFlashNvStorageFtwSpareBase|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdFlashNvStorageVariableSize|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdWinNtCpuSpeed|gEfiNt32PkgTokenSpaceGuid|L"3000"|8
  PcdFlashNvStorageVariableBase|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdWinNtSerialPort|gEfiNt32PkgTokenSpaceGuid|L"COM1!COM2"|18
  PcdWinNtFileSystem|gEfiNt32PkgTokenSpaceGuid|L".!..\\..\\..\\..\\EdkShellBinPkg\\bin\\ia32\\Apps"|106
  PcdFlashNvStorageFtwWorkingSize|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdWinNtGop|gEfiNt32PkgTokenSpaceGuid|L"UGA Window 1!UGA Window 2"|50
  PcdWinNtConsole|gEfiNt32PkgTokenSpaceGuid|L"Bus Driver Console Window"|50
  PcdFlashNvStorageFtwWorkingBase|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdWinNtMemorySize|gEfiNt32PkgTokenSpaceGuid|L"64!64"|10
  PcdWinNtVirtualDisk|gEfiNt32PkgTokenSpaceGuid|L"FW;40960;512"|24
  PcdWinNtCpuModel|gEfiNt32PkgTokenSpaceGuid|L"Intel(R) Processor Model"|48
  PcdWinNtPhysicalDisk|gEfiNt32PkgTokenSpaceGuid|L"E:RW;245760;512"|30
  PcdFlashNvStorageFtwSpareSize|gEfiGenericPlatformTokenSpaceGuid|0x0
  PcdWinNtUga|gEfiNt32PkgTokenSpaceGuid|L"UGA Window 1!UGA Window 2"|50

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################

[Components.IA32]
  ${WORKSPACE}/Nt32Pkg/BootModePei/BootMode.inf
  $(WORKSPACE)/Nt32Pkg/WinNtThunkDxe/WinNtThunk.inf
# ${WORKSPACE}/Nt32Pkg/MiscSubClassPlatformDxe/MiscSubclassDriver.inf
  $(WORKSPACE)\Nt32Pkg\WinNtThunkDxe\WinNtThunk.inf
  $(WORKSPACE)\Nt32Pkg\WinNtThunkPPIToProtocolPei\WinNtThunkPPIToProtocol.inf
  $(WORKSPACE)\Nt32Pkg\WinNtAutoScanPei\WinNtAutoScan.inf
  $(WORKSPACE)\Nt32Pkg\WinNtBlockIoDxe\WinNtBlockIo.inf
  $(WORKSPACE)\Nt32Pkg\WinNtBusDriverDxe\WinNtBusDriver.inf
  $(WORKSPACE)\Nt32Pkg\WinNtConsoleDxe\WinNtConsole.inf
  $(WORKSPACE)\Nt32Pkg\WinNtSimpleFileSystemDxe\WinNtSimpleFileSystem.inf
  $(WORKSPACE)\Nt32Pkg\WinNtGopDxe\WinNtGop.inf
  $(WORKSPACE)\Nt32Pkg\WinNtSerialIoDxe\WinNtSerialIo.inf
#  $(WORKSPACE)\Nt32Pkg\TimerDxe\Timer.inf
#  $(WORKSPACE)\Nt32Pkg\ResetRuntimeDxe\Reset.inf
#  $(WORKSPACE)\Nt32Pkg\RealTimeClockRuntimeDxe\RealTimeClock.inf
#  $(WORKSPACE)\Nt32Pkg\MonotonicCounterRuntimeDxe\Metronome.inf
#  $(WORKSPACE)\Nt32Pkg\CpuRuntimeDxe\Cpu.inf
  $(WORKSPACE)/Nt32Pkg/FvbServicesRuntimeDxe/Nt32Fwh.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Security/SecurityStub/Dxe/SecurityStub.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Capsule/RuntimeDxe/CapsuleRuntime.inf
  ${WORKSPACE}/MdeModulePkg/Universal/Ebc/Dxe/Ebc.inf
  ${WORKSPACE}/MdeModulePkg/Universal/GenericMemoryTest/Dxe/NullMemoryTest.inf
  ${WORKSPACE}/MdeModulePkg/Universal/FirmwareVolume/FaultTolerantWriteLite/Dxe/FtwLite.inf
  #${WORKSPACE}/MdeModulePkg/Universal/FirmwareVolume/GuidedSectionExtraction/Crc32SectionExtract/Dxe/Crc32SectionExtract.inf
  ${WORKSPACE}/MdeModulePkg/Bus/Pci/AtapiPassThru/Dxe/AtapiPassThru.inf
