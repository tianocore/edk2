#/** @file
# FmpDxe driver for Red Sample device firmware update.
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
#**/

  FmpDevicePkg/FmpDxe/FmpDxe.inf {
    <Defines>
      #
      # ESRT and FMP GUID for sample device capsule update
      #
      FILE_GUID = $(FMP_RED_SAMPLE_DEVICE)
    <PcdsFixedAtBuild>
      #
      # Unicode name string that is used to populate FMP Image Descriptor for this capsule update module
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceImageIdName|L"Sample Firmware Device"

      #
      # ESRT and FMP Lowest Support Version for this capsule update module
      # 000.000.000.000
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceBuildTimeLowestSupportedVersion|0x00000000

      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressWatchdogTimeInSeconds|2

      #
      # Capsule Update Progress Bar Color.  Set to Blue (RGB) (255, 0, 0)
      #
      gFmpDevicePkgTokenSpaceGuid.PcdFmpDeviceProgressColor|0x00FF0000

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
      # Device specific library that processes a capsule and updates the FW storage device
      #
      FmpDeviceLib|Vlv2TbltDevicePkg/Feature/Capsule/Library/FmpDeviceLibSample/FmpDeviceLib.inf
  }
