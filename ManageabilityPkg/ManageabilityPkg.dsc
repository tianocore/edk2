## @file
# Manageabilty Package
# This is the package provides edk2 drivers and libraries
# those are related to the platform management.
#
# Copyright (C) 2023-2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
# Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
# Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = ManageabilityPkg
  PLATFORM_GUID                  = 7A98123A-B194-40B6-A863-A52192F6D65D
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x0001001e
  OUTPUT_DIRECTORY               = Build/ManageabilityPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

[PcdsFeatureFlag]

  #
  # Manageability modules
  #
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiEnable              |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilitySmmIpmiEnable              |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityPeiIpmiEnable              |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxePldmEnable              |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeMctpEnable              |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxePldmSmbiosTransferEnable|TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiFru                 |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiOsWdt               |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiSolStatus           |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiBmcElog             |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiFrb                 |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityPeiIpmiFrb                 |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiBmcAcpi             |TRUE
  gManageabilityPkgTokenSpaceGuid.PcdManageabilityDxeIpmiSmbiosTransferEnable|TRUE

[Components]
  ManageabilityPkg/Library/PlatformBmcReadyLibNull/PlatformBmcReadyLibNull.inf
  ManageabilityPkg/Library/BaseManageabilityTransportNullLib/BaseManageabilityTransportNull.inf
  ManageabilityPkg/Library/ManageabilityTransportKcsLib/BaseManageabilityTransportKcs.inf
  ManageabilityPkg/Library/ManageabilityTransportSerialLib/Dxe/DxeManageabilityTransportSerial.inf
  ManageabilityPkg/Library/ManageabilityTransportSsifLib/Pei/PeiManageabilityTransportSsif.inf
  ManageabilityPkg/Library/ManageabilityTransportSsifLib/Dxe/DxeManageabilityTransportSsif.inf
  ManageabilityPkg/Library/ManageabilityTransportMctpLib/Dxe/DxeManageabilityTransportMctp.inf
  ManageabilityPkg/Library/PldmProtocolLibrary/Dxe/PldmProtocolLib.inf
  ManageabilityPkg/Library/IpmiCommandLib/IpmiCommandLib.inf

  #
  # Generic EDKII Lib
  #

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses.common]

  #
  # Entry point
  #
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  #
  # Basic
  #
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLibRepStr/BaseMemoryLibRepStr.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  #
  # UEFI & PI
  #
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  #
  # Misc
  #
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf

[LibraryClasses.common.SEC, LibraryClasses.common.PEI_CORE, LibraryClasses.common.PEIM]
  S3BootScriptLib|MdePkg/Library/BaseS3BootScriptLibNull/BaseS3BootScriptLibNull.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLibBase.inf

[LibraryClasses]
  ManageabilityTransportLib|ManageabilityPkg/Library/BaseManageabilityTransportNullLib/BaseManageabilityTransportNull.inf
  IpmiLib|MdeModulePkg/Library/BaseIpmiLibNull/BaseIpmiLibNull.inf
  PlatformBmcReadyLib|ManageabilityPkg/Library/PlatformBmcReadyLibNull/PlatformBmcReadyLibNull.inf

!include Include/Manageability.dsc
