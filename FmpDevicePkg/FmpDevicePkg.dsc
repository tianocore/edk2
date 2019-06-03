## @file
# Firmware Management Protocol Device Package
#
# This package provides an implementation of a Firmware Management Protocol
# instance that supports the update of firmware storage devices using UEFI
# Capsules.  The behavior of the Firmware Management Protocol instance is
# customized using libraries and PCDs.
#
# Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
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
  SUPPORTED_ARCHITECTURES        = IA32|X64|ARM|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT

  #
  # Define ESRT GUIDs for Firmware Management Protocol instances
  #
  DEFINE SYSTEM_FMP_ESRT_GUID   = B461B3BD-E62A-4A71-841C-50BA4E500267
  DEFINE DEVICE_FMP_ESRT_GUID   = 226034C4-8B67-4536-8653-D6EE7CE5A316

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
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
  FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
  CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
  FmpDeviceLib|FmpDevicePkg/Library/FmpDeviceLibNull/FmpDeviceLibNull.inf
  TimerLib|MdePkg/Library/BaseTimerLibNullTemplate/BaseTimerLibNullTemplate.inf

[LibraryClasses.ARM, LibraryClasses.AARCH64]
  #
  # It is not possible to prevent the ARM compiler for generic intrinsic functions.
  # This library provides the instrinsic functions generate by a given compiler.
  # [LibraryClasses.ARM, LibraryClasses.AARCH64] and NULL mean link this library
  # into all ARM and AARCH64 images.
  #
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[LibraryClasses.ARM]
  ArmSoftFloatLib|ArmPkg/Library/ArmSoftFloatLib/ArmSoftFloatLib.inf

[Components]
  #
  # Libraries
  #
  FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
  FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
  FmpDevicePkg/Library/FmpDeviceLibNull/FmpDeviceLibNull.inf
  FmpDevicePkg/FmpDxe/FmpDxeLib.inf

  #
  # Modules
  #
  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # FILE_GUID is used as ESRT GUID
      #
      FILE_GUID = $(SYSTEM_FMP_ESRT_GUID)
  }

  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # FILE_GUID is used as ESRT GUID
      #
      FILE_GUID = $(DEVICE_FMP_ESRT_GUID)
  }

[BuildOptions]
  *_*_*_CC_FLAGS = -D DISABLE_NEW_DEPRECATED_INTERFACES
