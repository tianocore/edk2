/** @file
  This file defines the GUID and data structure used to pass information about
  a variable store mapped on flash (i.e. a MMIO firmware volume) to the DXE and MM environment.

  Copyright (c) Microsoft Corporation.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef VARIABLE_FLASH_INFO_H_
#define VARIABLE_FLASH_INFO_H_

#define VARIABLE_FLASH_INFO_HOB_GUID \
  { 0x5d11c653, 0x8154, 0x4ac3, { 0xa8, 0xc2, 0xfb, 0xa2, 0x89, 0x20, 0xfc, 0x90 }}

#define VARIABLE_FLASH_INFO_HOB_VERSION  1

extern EFI_GUID  gVariableFlashInfoHobGuid;

#pragma pack (push, 1)

///
/// This structure can be used to describe UEFI variable
/// flash information.
///
typedef struct {
  ///
  /// Version of this structure.
  ///
  /// Increment the value when the structure is modified.
  ///
  UINT32                  Version;
  ///
  /// Base address of the non-volatile variable range in the flash device.
  ///
  /// Note that this address should align with the block size requirements of the flash device.
  ///
  EFI_PHYSICAL_ADDRESS    NvVariableBaseAddress;
  ///
  /// Size of the non-volatile variable range in the flash device.
  ///
  /// Note that this value should be less than or equal to FtwSpareLength to support reclaim of
  /// entire variable store area.
  /// Note that this address should align with the block size requirements of the flash device.
  ///
  UINT64                  NvVariableLength;
  ///
  /// Base address of the FTW spare block range in the flash device.
  ///
  /// Note that this address should align with the block size requirements of the flash device.
  ///
  EFI_PHYSICAL_ADDRESS    FtwSpareBaseAddress;
  ///
  /// Size of the FTW spare block range in the flash device.
  ///
  /// Note that this value should be greater than or equal to NvVariableLength.
  /// Note that this address should align with the block size requirements of the flash device.
  ///
  UINT64                  FtwSpareLength;
  ///
  /// Base address of the FTW working block range in the flash device.
  ///
  /// Note that if FtwWorkingLength is larger than on block size, this value should be block size aligned.
  ///
  EFI_PHYSICAL_ADDRESS    FtwWorkingBaseAddress;
  ///
  /// Size of the FTW working block range in the flash device.
  ///
  /// Note that if the value is less than on block size, the range should not span blocks.
  /// Note that if the value is larger than one block size, this value should be block size aligned.
  ///
  UINT64                  FtwWorkingLength;
} VARIABLE_FLASH_INFO;

#pragma pack (pop)

#endif
