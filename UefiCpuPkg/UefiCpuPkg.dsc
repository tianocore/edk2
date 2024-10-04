## @file
#  UefiCpuPkg Package
#
#  Copyright (c) 2007 - 2024, Intel Corporation. All rights reserved.<BR>
#  Copyright (C) 2023 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = UefiCpu
  PLATFORM_GUID                  = a1b7be22-78b3-4260-9569-8649e8c17d49
  PLATFORM_VERSION               = 0.90
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/UefiCpu
  SUPPORTED_ARCHITECTURES        = IA32|X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

#
# External libraries to build package
#

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  SerialPortLib|MdePkg/Library/BaseSerialPortLibNull/BaseSerialPortLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
  LocalApicLib|UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
  CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  SmmCpuPlatformHookLib|UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
  SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.inf
  SmmCpuSyncLib|UefiCpuPkg/Library/SmmCpuSyncLib/SmmCpuSyncLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf
  CcExitLib|UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf
  AmdSvsmLib|UefiCpuPkg/Library/AmdSvsmLibNull/AmdSvsmLibNull.inf
  MicrocodeLib|UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf
  SmmCpuRendezvousLib|UefiCpuPkg/Library/SmmCpuRendezvousLib/SmmCpuRendezvousLib.inf
  CpuPageTableLib|UefiCpuPkg/Library/CpuPageTableLib/CpuPageTableLib.inf
  UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
  UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
  UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibDebugLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxDxeLib.inf
  HobLib|MdeModulePkg/Library/BaseHobLibNull/BaseHobLibNull.inf
  MemoryAllocationLib|MdeModulePkg/Library/BaseMemoryAllocationLibNull/BaseMemoryAllocationLibNull.inf

[LibraryClasses.common.SEC]
  PlatformSecLib|UefiCpuPkg/Library/PlatformSecLibNull/PlatformSecLibNull.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf

[LibraryClasses.common.PEIM]
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  LockBoxLib|MdeModulePkg/Library/SmmLockBoxLib/SmmLockBoxPeiLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf
  RegisterCpuFeaturesLib|UefiCpuPkg/Library/RegisterCpuFeaturesLib/PeiRegisterCpuFeaturesLib.inf
  CpuCacheInfoLib|UefiCpuPkg/Library/CpuCacheInfoLib/PeiCpuCacheInfoLib.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.X64.PEIM]
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/PeiCpuExceptionHandlerLib.inf

[LibraryClasses.common.DXE_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  MpInitLib|UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  RegisterCpuFeaturesLib|UefiCpuPkg/Library/RegisterCpuFeaturesLib/DxeRegisterCpuFeaturesLib.inf
  CpuCacheInfoLib|UefiCpuPkg/Library/CpuCacheInfoLib/DxeCpuCacheInfoLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf

[LibraryClasses.common.MM_STANDALONE]
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf
  SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/StandaloneMmCpuFeaturesLib.inf

[LibraryClasses.common.MM_STANDALONE, LibraryClasses.common.DXE_SMM_DRIVER]
  CpuExceptionHandlerLib|UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  MmSaveStateLib|UefiCpuPkg/Library/MmSaveStateLib/IntelMmSaveStateLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.LoongArch64]
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf

#
# Drivers/Libraries within this package
#

[Components]
  UefiCpuPkg/CpuIo2Dxe/CpuIo2Dxe.inf
  UefiCpuPkg/CpuIoPei/CpuIoPei.inf
  UefiCpuPkg/Library/SecPeiDxeTimerLibUefiCpu/SecPeiDxeTimerLibUefiCpu.inf
  UefiCpuPkg/Application/Cpuid/Cpuid.inf
  UefiCpuPkg/Library/CpuTimerLib/BaseCpuTimerLib.inf
  UefiCpuPkg/Library/CpuCacheInfoLib/PeiCpuCacheInfoLib.inf
  UefiCpuPkg/Library/CpuCacheInfoLib/DxeCpuCacheInfoLib.inf
  UefiCpuPkg/MicrocodeMeasurementDxe/MicrocodeMeasurementDxe.inf
  UefiCpuPkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLib.inf

[Components.IA32, Components.X64]
  UefiCpuPkg/CpuDxe/CpuDxe.inf
  UefiCpuPkg/CpuFeatures/CpuFeaturesPei.inf {
    <LibraryClasses>
      NULL|UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  }
  UefiCpuPkg/CpuFeatures/CpuFeaturesDxe.inf {
    <LibraryClasses>
      NULL|UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  }
  UefiCpuPkg/CpuIo2Smm/CpuIo2Smm.inf
  UefiCpuPkg/CpuIo2Smm/CpuIo2StandaloneMm.inf
  UefiCpuPkg/CpuMpPei/CpuMpPei.inf
  UefiCpuPkg/CpuS3DataDxe/CpuS3DataDxe.inf
  UefiCpuPkg/Library/BaseXApicLib/BaseXApicLib.inf
  UefiCpuPkg/Library/BaseXApicX2ApicLib/BaseXApicX2ApicLib.inf
  UefiCpuPkg/Library/CpuCommonFeaturesLib/CpuCommonFeaturesLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/DxeCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/SecPeiCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/PeiCpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/MpInitLib/PeiMpInitLib.inf
  UefiCpuPkg/Library/MpInitLib/DxeMpInitLib.inf
  UefiCpuPkg/Library/MpInitLibUp/MpInitLibUp.inf
  UefiCpuPkg/Library/MicrocodeLib/MicrocodeLib.inf
  UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  UefiCpuPkg/Library/PlatformSecLibNull/PlatformSecLibNull.inf
  UefiCpuPkg/Library/RegisterCpuFeaturesLib/PeiRegisterCpuFeaturesLib.inf
  UefiCpuPkg/Library/RegisterCpuFeaturesLib/DxeRegisterCpuFeaturesLib.inf
  UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLib.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLibStm.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/StandaloneMmCpuFeaturesLib.inf
  UefiCpuPkg/Library/SmmCpuSyncLib/SmmCpuSyncLib.inf
  UefiCpuPkg/Library/CcExitLibNull/CcExitLibNull.inf
  UefiCpuPkg/Library/AmdSvsmLibNull/AmdSvsmLibNull.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationPei.inf
  UefiCpuPkg/PiSmmCommunication/PiSmmCommunicationSmm.inf
  UefiCpuPkg/SecCore/SecCore.inf {
    <LibraryClasses>
      NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
  }
  UefiCpuPkg/SecCore/SecCoreNative.inf {
    <LibraryClasses>
      NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
  }
  UefiCpuPkg/SecMigrationPei/SecMigrationPei.inf
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <Defines>
      FILE_GUID = D1D74FE9-7A4E-41D3-A0B3-67F13AD34D94
    <LibraryClasses>
      SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/SmmCpuFeaturesLibStm.inf
  }
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuDxeSmm.inf {
    <Defines>
      FILE_GUID = B7242C74-BD21-49EE-84B4-07162E8C080D
    <LibraryClasses>
      SmmCpuFeaturesLib|UefiCpuPkg/Library/SmmCpuFeaturesLib/AmdSmmCpuFeaturesLib.inf
      MmSaveStateLib|UefiCpuPkg/Library/MmSaveStateLib/AmdMmSaveStateLib.inf
  }
  UefiCpuPkg/Universal/Acpi/S3Resume2Pei/S3Resume2Pei.inf
  UefiCpuPkg/ResetVector/Vtf0/Vtf0.inf
  UefiCpuPkg/Library/SmmCpuRendezvousLib/SmmCpuRendezvousLib.inf
  UefiCpuPkg/Library/CpuPageTableLib/CpuPageTableLib.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/UnitTest/PeiCpuExceptionHandlerLibUnitTest.inf
  UefiCpuPkg/Test/UnitTest/EfiMpServicesPpiProtocol/EdkiiPeiMpServices2PpiPeiUnitTest.inf
  UefiCpuPkg/Test/UnitTest/EfiMpServicesPpiProtocol/EfiMpServiceProtocolDxeUnitTest.inf
  UefiCpuPkg/Test/UnitTest/EfiMpServicesPpiProtocol/EfiMpServiceProtocolDynamicCmdUnitTest.inf {
    <LibraryClasses>
      UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibConOut.inf
  }
  UefiCpuPkg/Test/UnitTest/EfiMpServicesPpiProtocol/EfiMpServiceProtocolShellUnitTest.inf {
    <LibraryClasses>
      UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibConOut.inf
  }
  UefiCpuPkg/Library/MmSaveStateLib/AmdMmSaveStateLib.inf
  UefiCpuPkg/Library/MmSaveStateLib/IntelMmSaveStateLib.inf
  UefiCpuPkg/Library/SmmCpuFeaturesLib/AmdSmmCpuFeaturesLib.inf
  UefiCpuPkg/Library/SmmRelocationLib/SmmRelocationLib.inf
  UefiCpuPkg/Library/SmmRelocationLib/AmdSmmRelocationLib.inf

[Components.X64]
  UefiCpuPkg/PiSmmCpuDxeSmm/PiSmmCpuStandaloneMm.inf
  UefiCpuPkg/Library/CpuExceptionHandlerLib/UnitTest/DxeCpuExceptionHandlerLibUnitTest.inf

[Components.RISCV64]
  UefiCpuPkg/Library/BaseRiscV64CpuExceptionHandlerLib/BaseRiscV64CpuExceptionHandlerLib.inf
  UefiCpuPkg/Library/BaseRiscV64CpuTimerLib/BaseRiscV64CpuTimerLib.inf
  UefiCpuPkg/Library/BaseRiscVFpuLib/BaseRiscVFpuLib.inf
  UefiCpuPkg/Library/BaseRiscVMmuLib/BaseRiscVMmuLib.inf
  UefiCpuPkg/CpuTimerDxeRiscV64/CpuTimerDxeRiscV64.inf
  UefiCpuPkg/CpuDxeRiscV64/CpuDxeRiscV64.inf
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf

[Components.LOONGARCH64]
  UefiCpuPkg/Library/CpuMmuLib/CpuMmuLib.inf
  UefiCpuPkg/CpuMmio2Dxe/CpuMmio2Dxe.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
