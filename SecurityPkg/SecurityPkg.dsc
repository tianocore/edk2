## @file
#  Security Module Package for All Architectures.
#
# Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2015-2020 Hewlett Packard Enterprise Development LP<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = SecurityPkg
  PLATFORM_GUID                  = B2C4614D-AE76-47ba-B876-5988BFED064F
  PLATFORM_VERSION               = 0.98
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/SecurityPkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|EBC|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf

  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  TpmCommLib|SecurityPkg/Library/TpmCommLib/TpmCommLib.inf
  PlatformSecureLib|SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  TcgPhysicalPresenceLib|SecurityPkg/Library/DxeTcgPhysicalPresenceLib/DxeTcgPhysicalPresenceLib.inf
  TpmMeasurementLib|SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  Tpm12CommandLib|SecurityPkg/Library/Tpm12CommandLib/Tpm12CommandLib.inf
  Tpm2CommandLib|SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
  TcgPpVendorLib|SecurityPkg/Library/TcgPpVendorLibNull/TcgPpVendorLibNull.inf
  Tcg2PpVendorLib|SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
  PciLib|MdePkg/Library/BasePciLibPciExpress/BasePciLibPciExpress.inf
  PciSegmentLib|MdePkg/Library/BasePciSegmentLibPci/BasePciSegmentLibPci.inf
  S3BootScriptLib|MdeModulePkg/Library/PiDxeS3BootScriptLib/DxeS3BootScriptLib.inf
  SmbusLib|MdePkg/Library/BaseSmbusLibNull/BaseSmbusLibNull.inf
  LockBoxLib|MdeModulePkg/Library/LockBoxNullLib/LockBoxNullLib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  TcgStorageCoreLib|SecurityPkg/Library/TcgStorageCoreLib/TcgStorageCoreLib.inf
  TcgStorageOpalLib|SecurityPkg/Library/TcgStorageOpalLib/TcgStorageOpalLib.inf
  ResetSystemLib|MdeModulePkg/Library/BaseResetSystemLibNull/BaseResetSystemLibNull.inf
  VariableKeyLib|SecurityPkg/Library/VariableKeyLibNull/VariableKeyLibNull.inf
  RpmcLib|SecurityPkg/Library/RpmcLibNull/RpmcLibNull.inf
  TcgEventLogRecordLib|SecurityPkg/Library/TcgEventLogRecordLib/TcgEventLogRecordLib.inf
  MmUnblockMemoryLib|MdePkg/Library/MmUnblockMemoryLib/MmUnblockMemoryLibNull.inf
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  SecureBootVariableProvisionLib|SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the intrinsic functions generate by a given compiler.
  # And NULL mean link this library into all ARM images.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for GCC stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
!endif
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/PeiTcg2PhysicalPresenceLib/PeiTcg2PhysicalPresenceLib.inf
  TpmMeasurementLib|SecurityPkg/Library/PeiTpmMeasurementLib/PeiTpmMeasurementLib.inf
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.common.DXE_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf

[LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.DXE_RUNTIME_DRIVER, LibraryClasses.common.DXE_SAL_DRIVER,]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibConOut/UefiDebugLibConOut.inf
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  ReportStatusCodeLib|MdeModulePkg/Library/RuntimeDxeReportStatusCodeLib/RuntimeDxeReportStatusCodeLib.inf
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
!endif
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf

[LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!endif
  HashLib|SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  SmmMemLib|MdePkg/Library/SmmMemLib/SmmMemLib.inf
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
!endif
  Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf
  Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/SmmTcg2PhysicalPresenceLib.inf
  SmmIoLib|MdePkg/Library/SmmIoLib/SmmIoLib.inf

[LibraryClasses.common.MM_STANDALONE]
  StandaloneMmDriverEntryPoint|MdePkg/Library/StandaloneMmDriverEntryPoint/StandaloneMmDriverEntryPoint.inf
  MmServicesTableLib|MdePkg/Library/StandaloneMmServicesTableLib/StandaloneMmServicesTableLib.inf
  Tcg2PhysicalPresenceLib|SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/StandaloneMmTcg2PhysicalPresenceLib.inf
  MemLib|StandaloneMmPkg/Library/StandaloneMmMemLib/StandaloneMmMemLib.inf
  HobLib|StandaloneMmPkg/Library/StandaloneMmHobLib/StandaloneMmHobLib.inf
  MemoryAllocationLib|StandaloneMmPkg/Library/StandaloneMmMemoryAllocationLib/StandaloneMmMemoryAllocationLib.inf

[PcdsDynamicDefault.common.DEFAULT]
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInstanceGuid|{0xb6, 0xe5, 0x01, 0x8b, 0x19, 0x4f, 0xe8, 0x46, 0xab, 0x93, 0x1c, 0x53, 0x67, 0x1b, 0x90, 0xcc}
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2InitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2SelfTestPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2ScrtmPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmInitializationPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpmScrtmPolicy|1
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2HashMask|3
  gEfiSecurityPkgTokenSpaceGuid.PcdTcg2HashAlgorithmBitmap|3

[PcdsDynamicHii.common.DEFAULT]
  gEfiSecurityPkgTokenSpaceGuid.PcdTcgPhysicalPresenceInterfaceVer|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x0|"1.3"|NV,BS
  gEfiSecurityPkgTokenSpaceGuid.PcdTpm2AcpiTableRev|L"TCG2_VERSION"|gTcg2ConfigFormSetGuid|0x8|3|NV,BS

[Components]
  SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
  SecurityPkg/Library/DxeImageAuthenticationStatusLib/DxeImageAuthenticationStatusLib.inf

  #
  # TPM
  #
  SecurityPkg/Library/DxeTpmMeasureBootLib/DxeTpmMeasureBootLib.inf
  SecurityPkg/Library/TpmCommLib/TpmCommLib.inf
  SecurityPkg/Library/DxeTcgPhysicalPresenceLib/DxeTcgPhysicalPresenceLib.inf
  SecurityPkg/Library/Tpm12CommandLib/Tpm12CommandLib.inf
  SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf
  SecurityPkg/Library/Tpm12DeviceLibTcg/Tpm12DeviceLibTcg.inf

  SecurityPkg/Tcg/PhysicalPresencePei/PhysicalPresencePei.inf
  SecurityPkg/Tcg/MemoryOverwriteControl/TcgMor.inf

  #
  # TPM2
  #
  SecurityPkg/Library/DxeTpm2MeasureBootLib/DxeTpm2MeasureBootLib.inf
  SecurityPkg/Library/DxeTcg2PhysicalPresenceLib/DxeTcg2PhysicalPresenceLib.inf
  SecurityPkg/Library/PeiTcg2PhysicalPresenceLib/PeiTcg2PhysicalPresenceLib.inf

  SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterDxe.inf
  SecurityPkg/Library/HashLibBaseCryptoRouter/HashLibBaseCryptoRouterPei.inf

  SecurityPkg/Library/Tpm2CommandLib/Tpm2CommandLib.inf
  SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
  SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
  SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpmStandaloneMm.inf
  SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
  SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterPei.inf

  SecurityPkg/Library/HashLibTpm2/HashLibTpm2.inf

  SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLib/PeiDxeTpmPlatformHierarchyLib.inf
  SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLibNull/PeiDxeTpmPlatformHierarchyLib.inf

  #
  # TCG Storage.
  #
  SecurityPkg/Library/TcgStorageCoreLib/TcgStorageCoreLib.inf
  SecurityPkg/Library/TcgStorageOpalLib/TcgStorageOpalLib.inf

  #
  # Variable Confidentiality & Integrity
  #
  SecurityPkg/Library/VariableKeyLibNull/VariableKeyLibNull.inf
  SecurityPkg/Library/RpmcLibNull/RpmcLibNull.inf

  #
  # Other
  #
  SecurityPkg/Library/DxeRsa2048Sha256GuidedSectionExtractLib/DxeRsa2048Sha256GuidedSectionExtractLib.inf
  SecurityPkg/Library/PeiRsa2048Sha256GuidedSectionExtractLib/PeiRsa2048Sha256GuidedSectionExtractLib.inf

  SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  SecurityPkg/Library/FmpAuthenticationLibRsa2048Sha256/FmpAuthenticationLibRsa2048Sha256.inf

  SecurityPkg/Library/PeiTpmMeasurementLib/PeiTpmMeasurementLib.inf
  SecurityPkg/Library/DxeTpmMeasurementLib/DxeTpmMeasurementLib.inf
  SecurityPkg/Library/PlatformSecureLibNull/PlatformSecureLibNull.inf
  SecurityPkg/Library/Tcg2PpVendorLibNull/Tcg2PpVendorLibNull.inf
  SecurityPkg/Library/TcgPpVendorLibNull/TcgPpVendorLibNull.inf
  SecurityPkg/Library/TcgEventLogRecordLib/TcgEventLogRecordLib.inf

[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  SecurityPkg/Library/AuthVariableLib/AuthVariableLib.inf
  SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  SecurityPkg/Library/SecureBootVariableProvisionLib/SecureBootVariableProvisionLib.inf
  SecurityPkg/EnrollFromDefaultKeysApp/EnrollFromDefaultKeysApp.inf
  SecurityPkg/VariableAuthenticated/SecureBootDefaultKeysDxe/SecureBootDefaultKeysDxe.inf

[Components.IA32, Components.X64, Components.AARCH64]
  #
  # Random Number Generator
  #
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf

[Components.IA32, Components.X64]
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf

  #
  # TPM
  #
  SecurityPkg/Tcg/TcgPei/TcgPei.inf
  SecurityPkg/Tcg/TcgDxe/TcgDxe.inf
  SecurityPkg/Tcg/TcgConfigDxe/TcgConfigDxe.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }

  #
  # TPM2
  #
  SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
  SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
  SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
  SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
  SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf

  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigPei.inf {
    <LibraryClasses>
      Tpm12DeviceLib|SecurityPkg/Library/Tpm12DeviceLibDTpm/Tpm12DeviceLibDTpm.inf
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2DeviceLibDTpm.inf
  }
  SecurityPkg/Tcg/Tcg2Pei/Tcg2Pei.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterPei.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
      NULL|SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf
  }

  SecurityPkg/Tcg/Tcg2PlatformPei/Tcg2PlatformPei.inf {
    <LibraryClasses>
      TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLib/PeiDxeTpmPlatformHierarchyLib.inf
  }

  SecurityPkg/Tcg/Tcg2Dxe/Tcg2Dxe.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibRouter/Tpm2DeviceLibRouterDxe.inf
      NULL|SecurityPkg/Library/Tpm2DeviceLibDTpm/Tpm2InstanceLibDTpm.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha1/HashInstanceLibSha1.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha256/HashInstanceLibSha256.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha384/HashInstanceLibSha384.inf
      NULL|SecurityPkg/Library/HashInstanceLibSha512/HashInstanceLibSha512.inf
      NULL|SecurityPkg/Library/HashInstanceLibSm3/HashInstanceLibSm3.inf
      PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  }
  SecurityPkg/Tcg/Tcg2Config/Tcg2ConfigDxe.inf {
    <LibraryClasses>
      Tpm2DeviceLib|SecurityPkg/Library/Tpm2DeviceLibTcg2/Tpm2DeviceLibTcg2.inf
  }

  SecurityPkg/Tcg/Tcg2PlatformDxe/Tcg2PlatformDxe.inf {
    <LibraryClasses>
      TpmPlatformHierarchyLib|SecurityPkg/Library/PeiDxeTpmPlatformHierarchyLib/PeiDxeTpmPlatformHierarchyLib.inf
  }

  #
  # Hash2
  #
  SecurityPkg/Hash2DxeCrypto/Hash2DxeCrypto.inf

  #
  # PKCS7 Verification
  #
  SecurityPkg/Pkcs7Verify/Pkcs7VerifyDxe/Pkcs7VerifyDxe.inf

[Components.IA32, Components.X64]

  SecurityPkg/Tcg/MemoryOverwriteRequestControlLock/TcgMorLockSmm.inf
  SecurityPkg/Tcg/TcgSmm/TcgSmm.inf
  SecurityPkg/Tcg/Tcg2Smm/Tcg2Smm.inf
  SecurityPkg/Tcg/Tcg2Smm/Tcg2StandaloneMm.inf
  SecurityPkg/Tcg/Tcg2Smm/Tcg2MmDependencyDxe.inf
  SecurityPkg/Tcg/Tcg2Acpi/Tcg2Acpi.inf
  SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/SmmTcg2PhysicalPresenceLib.inf
  SecurityPkg/Library/SmmTcg2PhysicalPresenceLib/StandaloneMmTcg2PhysicalPresenceLib.inf

  #
  # Opal Password solution
  #
  SecurityPkg/Tcg/Opal/OpalPassword/OpalPasswordDxe.inf
  SecurityPkg/Tcg/Opal/OpalPassword/OpalPasswordPei.inf

  #
  # HDD Password solution
  #
  SecurityPkg/HddPassword/HddPasswordDxe.inf
  SecurityPkg/HddPassword/HddPasswordPei.inf

  #
  # Common FV checker/verifier/reporter
  #
  SecurityPkg/FvReportPei/FvReportPei.inf

[BuildOptions]
   MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:256
  INTEL:*_*_IA32_DLINK_FLAGS = /ALIGN:256
        *_*_*_CC_FLAGS       = -D DISABLE_NEW_DEPRECATED_INTERFACES

