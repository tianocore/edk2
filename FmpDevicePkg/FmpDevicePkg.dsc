## @file
# Firmware Management Protocol Device Package
#
# This package provides an implementation of a Firmware Management Protocol
# instance that supports the update of firmware storage devices using UEFI
# Capsules.  The behavior of the Firmware Management Protocol instance is
# customized using libraries and PCDs.
#
# Copyright (c) Microsoft Corporation.<BR>
# Copyright (c) 2018 - 2021, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

[Defines]
  PLATFORM_NAME                  = FmpDevicePkg
  PLATFORM_GUID                  = 0af3d540-27c6-11e8-828b-f8597177a00a
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/FmpDevicePkg
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  #
  # Define ESRT GUIDs for Firmware Management Protocol instances
  #
  DEFINE SYSTEM_FMP_ESRT_GUID   = B461B3BD-E62A-4A71-841C-50BA4E500267
  DEFINE DEVICE_FMP_ESRT_GUID   = 226034C4-8B67-4536-8653-D6EE7CE5A316

  #
  # TRUE  - Build FmpDxe module for with storage access enabled
  # FALSE - Build FmpDxe module for with storage access disabled
  #
  DEFINE DEVICE_FMP_STORAGE_ACCESS_ENABLE = TRUE

!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc

!include MdePkg/MdeLibs.dsc.inc

[LibraryClasses]
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  DebugLib|MdePkg/Library/UefiDebugLibStdErr/UefiDebugLibStdErr.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
!ifdef CONTINUOUS_INTEGRATION
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibNull/BaseCryptLibNull.inf
!else
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  RngLib|MdePkg/Library/BaseRngLibNull/BaseRngLibNull.inf
!endif
  FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
  FmpDeviceLib|FmpDevicePkg/Library/FmpDeviceLibNull/FmpDeviceLibNull.inf
  FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
  FmpDependencyCheckLib|FmpDevicePkg/Library/FmpDependencyCheckLibNull/FmpDependencyCheckLibNull.inf
  FmpDependencyDeviceLib|FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the intrinsic functions generate by a given compiler.
  # [LibraryClasses.ARM, LibraryClasses.AARCH64] and NULL mean link this library
  # into all ARM and AARCH64 images.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.ARM]
  ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf

[PcdsPatchableInModule]
  gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageTypeIdGuid|{0}

[Components]
  #
  # Libraries
  #
  FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  FmpDevicePkg/Library/CapsuleUpdatePolicyLibOnProtocol/CapsuleUpdatePolicyLibOnProtocol.inf
  FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
  FmpDevicePkg/Library/FmpDeviceLibNull/FmpDeviceLibNull.inf
  FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
  FmpDevicePkg/Library/FmpDependencyCheckLib/FmpDependencyCheckLib.inf
  FmpDevicePkg/Library/FmpDependencyCheckLibNull/FmpDependencyCheckLibNull.inf
  FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
  FmpDevicePkg/FmpDxe/FmpDxeLib.inf

  #
  # Modules
  #
  FmpDevicePkg/CapsuleUpdatePolicyDxe/CapsuleUpdatePolicyDxe.inf {
    <LibraryClasses>
      CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  }
  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # FILE_GUID is used as ESRT GUID
      #
      FILE_GUID = $(SYSTEM_FMP_ESRT_GUID)
    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"Sample Firmware Device"
      #
      # Certificates used to authenticate capsule update image
      #
      !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
    <PcdsPatchableInModule>
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageTypeIdGuid|{GUID("$(SYSTEM_FMP_ESRT_GUID)")}
    <LibraryClasses>
      #
      # Use CapsuleUpdatePolicyLib that calls the Capsule Update Policy Protocol.
      # Depends on the CapsuleUpdatePolicyDxe module to produce the protocol.
      # Required for FmpDxe modules that are intended to be platform independent.
      #
      CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibOnProtocol/CapsuleUpdatePolicyLibOnProtocol.inf
  }

  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # FILE_GUID is used as ESRT GUID
      #
      FILE_GUID = $(DEVICE_FMP_ESRT_GUID)
    <PcdsFeatureFlag>
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceStorageAccessEnable|$(DEVICE_FMP_STORAGE_ACCESS_ENABLE)
    <PcdsFixedAtBuild>
!if $(DEVICE_FMP_STORAGE_ACCESS_ENABLE) == FALSE
      #
      # Disable test key detection
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceTestKeySha256Digest|{0}
!endif
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
!if $(DEVICE_FMP_STORAGE_ACCESS_ENABLE) == TRUE
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"Sample Firmware Device"
!else
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"Sample Firmware Device No Storage Access"
!endif
      #
      # Certificates used to authenticate capsule update image
      #
      !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
    <PcdsPatchableInModule>
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageTypeIdGuid|{GUID("$(DEVICE_FMP_ESRT_GUID)")}
    <LibraryClasses>
      #
      # Directly use a platform specific CapsuleUpdatePolicyLib instance.
      # Only works for FmpDxe modules that are build from sources and included
      # in a system firmware image.
      #
      CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  }

  #
  # Add UEFI Target Based Unit Tests
  #
  FmpDevicePkg/Test/UnitTest/Library/FmpDependencyLib/FmpDependencyLibUnitTestsUefi.inf

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
