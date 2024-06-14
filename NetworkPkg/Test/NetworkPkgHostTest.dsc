## @file
# NetworkPkgHostTest DSC file used to build host-based unit tests.
#
# Copyright (c) Microsoft Corporation.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
  PLATFORM_NAME           = NetworkPkgHostTest
  PLATFORM_GUID           = 3b68324e-fc07-4d49-9520-9347ede65879
  PLATFORM_VERSION        = 0.1
  DSC_SPECIFICATION       = 0x00010005
  OUTPUT_DIRECTORY        = Build/NetworkPkg/HostTest
  SUPPORTED_ARCHITECTURES = IA32|X64|AARCH64
  BUILD_TARGETS           = NOOPT
  SKUID_IDENTIFIER        = DEFAULT

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc

[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[Components]
  #
  # Build HOST_APPLICATION that tests NetworkPkg
  #
  NetworkPkg/Dhcp6Dxe/GoogleTest/Dhcp6DxeGoogleTest.inf
  NetworkPkg/Ip6Dxe/GoogleTest/Ip6DxeGoogleTest.inf
  NetworkPkg/UefiPxeBcDxe/GoogleTest/UefiPxeBcDxeGoogleTest.inf {
    <LibraryClasses>
      UefiRuntimeServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      UefiBootServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiBootServicesTableLib/MockUefiBootServicesTableLib.inf
  }

# Despite these library classes being listed in [LibraryClasses] below, they are not needed for the host-based unit tests.
[LibraryClasses]
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
!else
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
!endif
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf

!if $(TOOL_CHAIN_TAG) == VS2019 or $(TOOL_CHAIN_TAG) == VS2022
[LibraryClasses.X64]
  # Provide StackCookie support lib so that we can link to /GS exports for VS builds
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
!endif

[LibraryClasses.common.UEFI_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf

[LibraryClasses.common.UEFI_APPLICATION]
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  ShellLib|ShellPkg/Library/UefiShellLib/UefiShellLib.inf

[LibraryClasses.ARM]
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

[LibraryClasses.RISCV64]
  RngLib|MdePkg/Library/BaseRngLibTimerLib/BaseRngLibTimerLib.inf

[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2
  gEfiNetworkPkgTokenSpaceGuid.PcdDhcp6UidType|0x4
