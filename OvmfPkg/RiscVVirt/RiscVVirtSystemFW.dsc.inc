## @file
#  FmpDxe driver for system firmware update.
#
#  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # ESRT and FMP GUID for system device capsule update
      #
      FILE_GUID = $(FMP_SYSTEM_DEVICE)

    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"RISC-V VIRT System Firmware Device"

      #
      # Firmware revision that is reported by FMP
      # Can be overridden at build time with -D FIRMWARE_REVISION=0x...
      #
      !ifdef $(FIRMWARE_REVISION)
      gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareRevision|$(FIRMWARE_REVISION)
      !endif

      #
      # ESRT and FMP Lowest Supported Version for this capsule update module
      # Override the DEC default (0x0) at build time with -D LSV=0x...
      #
      !ifdef $(LSV)
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|$(LSV)
      !endif

      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|2

      #
      # Capsule Update Progress Bar Color.  Set to Green (RGB) (0, 255, 0)
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x0000FF00

      #
      # Certificates used to authenticate capsule update image
      # EDKII Test certificate
      #
      !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc

    <LibraryClasses>
      #
      # Generic libraries that are used "as is" by all FMP modules
      #
      FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
      FmpDependencyLib|FmpDevicePkg/Library/FmpDependencyLib/FmpDependencyLib.inf
      FmpDependencyCheckLib|FmpDevicePkg/Library/FmpDependencyCheckLibNull/FmpDependencyCheckLibNull.inf
      FmpDependencyDeviceLib|FmpDevicePkg/Library/FmpDependencyDeviceLibNull/FmpDependencyDeviceLibNull.inf
      #
      # Platform specific capsule policy library
      #
      CapsuleUpdatePolicyLib|OvmfPkg/RiscVVirt/Feature/Capsule/Library/CapsuleUpdatePolicyLib/CapsuleUpdatePolicyLib.inf
      #
      # Device specific library that processes a capsule and updates the FW storage device
      #
      FmpDeviceLib|OvmfPkg/RiscVVirt/Feature/Capsule/Library/FmpDeviceLib/FmpDeviceLib.inf
      VirtNorFlashDeviceLib|OvmfPkg/Library/VirtNorFlashDeviceLib/VirtNorFlashDeviceLib.inf
      PlatformFlashAccessLib|OvmfPkg/RiscVVirt/Feature/Capsule/Library/PlatformFlashAccessLib/PlatformFlashAccessLib.inf
  }
