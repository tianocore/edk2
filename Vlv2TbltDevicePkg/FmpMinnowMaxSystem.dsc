#/** @file
# FmpDxe driver for Minnow Max system firmware update.
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#
#**/

  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # ESRT and FMP GUID for system firmware capsule update
      #
      FILE_GUID = $(FMP_MINNOW_MAX_SYSTEM)
    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"Minnow Max System Firmware Device"

      #
      # ESRT and FMP Lowest Support Version for this capsule update module
      # 000.000.000.000
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|0x00000000

      gPlatformModuleTokenSpaceGuid.PcdSystemFirmwareFmpLowestSupportedVersion|0x00000000
      gPlatformModuleTokenSpaceGuid.PcdSystemFirmwareFmpVersion|0x00000000
      gPlatformModuleTokenSpaceGuid.PcdSystemFirmwareFmpVersionString|"000.000.000.000"

      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|4

      #
      # Capsule Update Progress Bar Color.  Set to Purple (RGB) (255, 0, 255)
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x00FF00FF

      #
      # Certificates used to authenticate capsule update image
      #
      !include Vlv2TbltDevicePkg/FmpCertificate.dsc

    <LibraryClasses>
      #
      # Generic libraries that are used "as is" by all FMP modules
      #
      FmpPayloadHeaderLib|FmpDevicePkg/Library/FmpPayloadHeaderLibV1/FmpPayloadHeaderLibV1.inf
      FmpAuthenticationLib|SecurityPkg/Library/FmpAuthenticationLibPkcs7/FmpAuthenticationLibPkcs7.inf
      #
      # Platform specific capsule policy library
      #
      CapsuleUpdatePolicyLib|FmpDevicePkg/Library/CapsuleUpdatePolicyLibNull/CapsuleUpdatePolicyLibNull.inf
      #
      # Platform specific library that processes a capsule and updates the FW storage device
      #
      FmpDeviceLib|Vlv2TbltDevicePkg/Feature/Capsule/Library/FmpDeviceLib/FmpDeviceLib.inf
  }
