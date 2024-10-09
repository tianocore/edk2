## @file
#  Cryptographic Library Package for UEFI Security Implementation.
#  PEIM, DXE Driver, and SMM Driver with all crypto services enabled.
#
#  Copyright (c) 2009 - 2022, Intel Corporation. All rights reserved.<BR>
#  Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#  Copyright (c) 2022, Loongson Technology Corporation Limited. All rights reserved.<BR>
#  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = CryptoPkg
  PLATFORM_GUID                  = E1063286-6C8C-4c25-AEF0-67A9A5B6E6B6
  PLATFORM_VERSION               = 0.98
  DSC_SPECIFICATION              = 0x00010005
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64|RISCV64|LOONGARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  #
  # Flavor of PEI, DXE, SMM modules to build.
  # Must be one of ALL, NONE, MIN_PEI, MIN_DXE_MIN_SMM, TARGET_UINT_TESTS.
  # Default is ALL that is used for package build verification.
  #   ALL             - Build PEIM, DXE, and SMM drivers.  Protocols and PPIs
  #                     publish all services.
  #   NONE            - Build PEIM, DXE, and SMM drivers.  Protocols and PPIs
  #                     publish no services.  Used to verify compiler/linker
  #                     optimizations are working correctly.
  #   MIN_PEI         - Build PEIM with PPI that publishes minimum required
  #                     services.
  #   MIN_DXE_MIN_SMM - Build DXE and SMM drivers with Protocols that publish
  #                     minimum required services.
  #   TARGET_UNIT_TESTS - Build target-based unit tests
  #
  DEFINE CRYPTO_SERVICES = ALL
!if $(CRYPTO_SERVICES) IN "ALL NONE MIN_PEI MIN_DXE_MIN_SMM TARGET_UNIT_TESTS"
!else
  !error CRYPTO_SERVICES must be set to one of ALL NONE MIN_PEI MIN_DXE_MIN_SMM TARGET_UNIT_TESTS.
!endif

#
# Define different OUTPUT_DIRECTORY for each CRYPTO_SERVICES profile
#
!if $(CRYPTO_SERVICES) == ALL
  OUTPUT_DIRECTORY             = Build/CryptoPkg/All
!endif
!if $(CRYPTO_SERVICES) == NONE
  OUTPUT_DIRECTORY              = Build/CryptoPkg/None
!endif
!if $(CRYPTO_SERVICES) == MIN_PEI
  OUTPUT_DIRECTORY              = Build/CryptoPkg/MinPei
!endif
!if $(CRYPTO_SERVICES) == MIN_DXE_MIN_SMM
  OUTPUT_DIRECTORY              = Build/CryptoPkg/MinDxeMinSmm
!endif
!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
  OUTPUT_DIRECTORY              = Build/CryptoPkg/TagetUnitTests
!endif

#
# Define FILE_GUID names/values for CryptoPei, CryptopDxe, and CryptoSmm
# drivers that are linked with different OpensslLib instances
#
  DEFINE  PEI_CRYPTO_GUID     = C693A250-6B36-49B9-B7F3-7283F8136A72
  DEFINE  PEI_STD_GUID        = EBD49F5C-6D8B-40D1-A56D-9AFA485A8661
  DEFINE  PEI_FULL_GUID       = D51FCE59-6860-49C0-9B35-984470735D17
  DEFINE  PEI_STD_ACCEL_GUID  = DCC9CB49-7BE2-47C6-864E-6DCC932360F9
  DEFINE  PEI_FULL_ACCEL_GUID = A10827AD-7598-4955-B661-52EE2B62B057
  DEFINE  DXE_CRYPTO_GUID     = 31C17C54-325D-47D5-8622-888098F10E44
  DEFINE  DXE_STD_GUID        = ADD6D05A-52A2-437B-98E7-DBFDA89352CD
  DEFINE  DXE_FULL_GUID       = AA83B296-F6EA-447F-B013-E80E98629CF8
  DEFINE  DXE_STD_ACCEL_GUID  = 9FBDAD27-910C-4229-9EFF-A93BB5FE18C6
  DEFINE  DXE_FULL_ACCEL_GUID = 41A491D1-A972-468B-A299-DABF415A43B7
  DEFINE  SMM_CRYPTO_GUID     = 1A1C9E13-5722-4636-AB73-31328EDE8BAF
  DEFINE  SMM_STD_GUID        = E4D7D1E3-E886-4412-A442-EFD6F2502DD3
  DEFINE  SMM_FULL_GUID       = 1930CE7E-6598-48ED-8AB1-EBE7E85EC254
  DEFINE  SMM_STD_ACCEL_GUID  = 828959D3-CEA6-4B79-B1FC-5AFA0D7F2144
  DEFINE  SMM_FULL_ACCEL_GUID = C1760694-AB3A-4532-8C6D-52D8F86EB1AA

!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc
!endif

################################################################################
#
# Library Class section - list of all Library Classes needed by this Platform.
#
################################################################################

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DebugLib|MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf

[LibraryClasses.IA32, LibraryClasses.X64, LibraryClasses.AARCH64]
  RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf

[LibraryClasses.ARM]
  ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf

[LibraryClasses.common.SEC]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  # StackCheckLib is not linked for SEC modules by default, this package can link it against its SEC modules
  NULL|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf

[LibraryClasses.common.PEIM]
  PeimEntryPoint|MdePkg/Library/PeimEntryPoint/PeimEntryPoint.inf
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLib/PeiServicesTablePointerLib.inf
  PeiServicesLib|MdePkg/Library/PeiServicesLib/PeiServicesLib.inf
  MemoryAllocationLib|MdePkg/Library/PeiMemoryAllocationLib/PeiMemoryAllocationLib.inf
  HobLib|MdePkg/Library/PeiHobLib/PeiHobLib.inf
  PcdLib|MdePkg/Library/PeiPcdLib/PeiPcdLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/PeiReportStatusCodeLib/PeiReportStatusCodeLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

[LibraryClasses.IA32.PEIM, LibraryClasses.X64.PEIM]
  PeiServicesTablePointerLib|MdePkg/Library/PeiServicesTablePointerLibIdt/PeiServicesTablePointerLibIdt.inf

[LibraryClasses.common.DXE_DRIVER]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/DxeReportStatusCodeLib/DxeReportStatusCodeLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf

[LibraryClasses.common.DXE_SMM_DRIVER]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  SmmServicesTableLib|MdePkg/Library/SmmServicesTableLib/SmmServicesTableLib.inf
  MmServicesTableLib|MdePkg/Library/MmServicesTableLib/MmServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/SmmMemoryAllocationLib/SmmMemoryAllocationLib.inf
  ReportStatusCodeLib|MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
  PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf

[LibraryClasses.common.UEFI_APPLICATION]
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################
[PcdsFixedAtBuild]
  gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x0f
  gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
  gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x06

#
# For ALL and TARGET_UINT_TESTS profiles, enable all non-deprecated families
# and services in PcdCryptoServiceFamilyEnable.
#
!if $(CRYPTO_SERVICES) IN "ALL TARGET_UINT_TESTS"
[PcdsFixedAtBuild]
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Dh.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Arc4.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.RsaPss.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.ParallelHash.Family                      | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Bn.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Ec.Family                                | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
!endif

#
# Enable minimum set of families/services in PcdCryptoServiceFamilyEnable
# required by typical PEI phase.
#
!if $(CRYPTO_SERVICES) == MIN_PEI
[PcdsFixedAtBuild]
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                   | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha384.Family                   | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha512.Family                   | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sm3.Family                      | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free               | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize     | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init               | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                     | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
!endif

#
# Enable minimum set of families/services in PcdCryptoServiceFamilyEnable
# required by typical DXE and SMM phases.
#
!if $(CRYPTO_SERVICES) == MIN_DXE_MIN_SMM
[PcdsFixedAtBuild]
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha256.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.HmacSha384.Family                        | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs1v2Encrypt             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs5HashPassword          | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7Verify                | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.VerifyEKUsInPkcs7Signature | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7GetSigners            | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.Pkcs7FreeSigners           | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Pkcs.Services.AuthenticodeVerify         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Random.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Pkcs1Verify                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.New                         | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.Free                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.SetKey                      | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Rsa.Services.GetPublicKeyFromX509        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha1.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Sha256.Services.HashAll                  | FALSE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetSubjectName             | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetCommonName              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetOrganizationName        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.X509.Services.GetTBSCert                 | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Tls.Family                               | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsSet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.TlsGet.Family                            | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.GetContextSize              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.Init                        | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcEncrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Aes.Services.CbcDecrypt                  | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Encrypt              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.AeadAesGcm.Services.Decrypt              | TRUE
  gEfiCryptoPkgTokenSpaceGuid.PcdCryptoServiceFamilyEnable.Hkdf.Family                              | PCD_CRYPTO_SERVICE_ENABLE_FAMILY
!endif

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

#
# If profile is TARGET_UNIT_TESTS, then build target-based unit tests
# using the OpensslLib, BaseCryptLib, and TlsLib with the largest set of
# available services.
#
!if $(CRYPTO_SERVICES) == TARGET_UNIT_TESTS
[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  #
  # Target based unit tests
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
    <BuildOptions>
      MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096 /SUBSYSTEM:CONSOLE
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:NOOPT_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
  }

[Components.IA32, Components.X64, Components.AARCH64]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <Defines>
      FILE_GUID = B91B9A95-4D52-4501-A98F-A1711C14ED93
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
    <BuildOptions>
      MSFT:*_*_*_DLINK_FLAGS     = /ALIGN:4096 /FILEALIGN:4096 /SUBSYSTEM:CONSOLE
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:DEBUG_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
      MSFT:NOOPT_*_*_DLINK_FLAGS = /EXPORT:InitializeDriver=$(IMAGE_ENTRY_POINT) /BASE:0x10000
  }

[Components.RISCV64]
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  }
!endif

#
# If profile is ALL, then do verification build of all library instances.
#
!if $(CRYPTO_SERVICES) == ALL
[Components]
  #
  # Build verification of all library instances
  #
  CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/BaseCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/SecCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/SmmCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/RuntimeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibMbedTls/TestBaseCryptLib.inf
  CryptoPkg/Library/MbedTlsLib/MbedTlsLib.inf
  CryptoPkg/Library/MbedTlsLib/MbedTlsLibFull.inf
  CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
  CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  CryptoPkg/Library/TlsLib/TlsLib.inf
  CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  CryptoPkg/Library/OpensslLib/OpensslLib.inf
  CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  CryptoPkg/Library/OpensslLib/OpensslLibSm3.inf
  CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf
  #
  # Build verification of target-based unit tests
  #
  CryptoPkg/Test/UnitTest/Library/BaseCryptLib/TestBaseCryptLibShell.inf {
    <LibraryClasses>
      UnitTestLib|UnitTestFrameworkPkg/Library/UnitTestLib/UnitTestLib.inf
      UnitTestPersistenceLib|UnitTestFrameworkPkg/Library/UnitTestPersistenceLibNull/UnitTestPersistenceLibNull.inf
      UnitTestResultReportLib|UnitTestFrameworkPkg/Library/UnitTestResultReportLib/UnitTestResultReportLibConOut.inf
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
      BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
      TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  }

[Components.IA32, Components.X64, Components.AARCH64]
  #
  # Build verification of IA32/X64/AARCH64 specific libraries
  #
  CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
  CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
!endif

#
# If profile is ALL or NONE or MIN_PEI, then build CryptoPei with all supported
# OpensslLib instances.
#
!if $(CRYPTO_SERVICES) in "ALL NONE MIN_PEI"
[Components]
  #
  # CryptoPei with OpensslLib instance without SSL or EC services
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_CRYPTO_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  }
  #
  # CryptoPei with OpensslLib instance without EC services
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_STD_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  }
[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  #
  # CryptoPei with OpensslLib instance with all services
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_FULL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  }

[Components.IA32, Components.X64, Components.AARCH64]
  #
  # CryptoPei with IA32/X64/AARCH64 performance optimized OpensslLib instance without EC services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_STD_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }

  #
  # CryptoPei with IA32/X64/AARCH64 performance optimized OpensslLib instance all services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoPei.inf {
    <Defines>
      FILE_GUID = $(PEI_FULL_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:4096
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:4096
  }
!endif

#
# If profile is ALL or NONE or MIN_DXE_MIN_SMM, then build CryptoDxe and
# CryptoSmm using all supported OpensslLib instances.
#
!if $(CRYPTO_SERVICES) in "ALL NONE MIN_DXE_MIN_SMM"
[Components]
  #
  # CryptoDxe with OpensslLib instance with no SSL or EC services
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_CRYPTO_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
      TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  }
  #
  # CryptoDxe with OpensslLib instance with no EC services
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_STD_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  }
[Components.IA32, Components.X64, Components.ARM, Components.AARCH64]
  #
  # CryptoDxe with OpensslLib instance with all services
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_FULL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  }

[Components.IA32, Components.X64, Components.AARCH64]
  #
  # CryptoDxe with IA32/X64/AARCH64 performance optimized OpensslLib instance with no EC services
  # with TLS feature enabled.
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_STD_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
  #
  # CryptoDxe with IA32/X64/AARCH64 performance optimized OpensslLib instance with all services.
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoDxe.inf {
    <Defines>
      FILE_GUID = $(DXE_FULL_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:4096
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:4096
  }
  #
  # CryptoSmm with OpensslLib instance with no SSL or EC services
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_CRYPTO_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
  }
  #
  # CryptoSmm with OpensslLib instance with no SSL services
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_STD_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  }
  #
  # CryptoSmm with OpensslLib instance with no all services
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_FULL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFull.inf
  }
  #
  # CryptoSmm with IA32/X64/AARCH64 performance optimized OpensslLib instance with no EC services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_STD_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:64
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:256
  }
  #
  # CryptoSmm with IA32/X64/AARCH64 performance optimized OpensslLib instance with all services
  # IA32/X64 assembly optimizations required larger alignments
  #
  CryptoPkg/Driver/CryptoSmm.inf {
    <Defines>
      FILE_GUID = $(SMM_FULL_ACCEL_GUID)
    <LibraryClasses>
      OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibFullAccel.inf
    <BuildOptions>
      MSFT:*_*_IA32_DLINK_FLAGS = /ALIGN:4096
      MSFT:*_*_X64_DLINK_FLAGS  = /ALIGN:4096
  }
!endif

[BuildOptions]
  RELEASE_*_*_CC_FLAGS = -DMDEPKG_NDEBUG
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
