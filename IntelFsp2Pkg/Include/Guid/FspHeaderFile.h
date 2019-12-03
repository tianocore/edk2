/** @file
  Intel FSP Header File definition from Intel Firmware Support Package External
  Architecture Specification v2.0.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FSP_HEADER_FILE_H__
#define __FSP_HEADER_FILE_H__

#define FSP_HEADER_REVISION_3   3

#define FSPE_HEADER_REVISION_1  1
#define FSPP_HEADER_REVISION_1  1

///
/// Fixed FSP header offset in the FSP image
///
#define FSP_INFO_HEADER_OFF    0x94

#define OFFSET_IN_FSP_INFO_HEADER(x)  (UINT32)&((FSP_INFO_HEADER *)(UINTN)0)->x

#define FSP_INFO_HEADER_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'H')

#pragma pack(1)

///
/// FSP Information Header as described in FSP v2.0 Spec section 5.1.1.
///
typedef struct {
  ///
  /// Byte 0x00: Signature ('FSPH') for the FSP Information Header.
  ///
  UINT32  Signature;
  ///
  /// Byte 0x04: Length of the FSP Information Header.
  ///
  UINT32  HeaderLength;
  ///
  /// Byte 0x08: Reserved.
  ///
  UINT8   Reserved1[2];
  ///
  /// Byte 0x0A: Indicates compliance with a revision of this specification in the BCD format.
  ///
  UINT8   SpecVersion;
  ///
  /// Byte 0x0B: Revision of the FSP Information Header.
  ///
  UINT8   HeaderRevision;
  ///
  /// Byte 0x0C: Revision of the FSP binary.
  ///
  UINT32  ImageRevision;
  ///
  /// Byte 0x10: Signature string that will help match the FSP Binary to a supported HW configuration.
  ///
  CHAR8   ImageId[8];
  ///
  /// Byte 0x18: Size of the entire FSP binary.
  ///
  UINT32  ImageSize;
  ///
  /// Byte 0x1C: FSP binary preferred base address.
  ///
  UINT32  ImageBase;
  ///
  /// Byte 0x20: Attribute for the FSP binary.
  ///
  UINT16  ImageAttribute;
  ///
  /// Byte 0x22: Attributes of the FSP Component.
  ///
  UINT16  ComponentAttribute;
  ///
  /// Byte 0x24: Offset of the FSP configuration region.
  ///
  UINT32  CfgRegionOffset;
  ///
  /// Byte 0x28: Size of the FSP configuration region.
  ///
  UINT32  CfgRegionSize;
  ///
  /// Byte 0x2C: Reserved2.
  ///
  UINT32  Reserved2;
  ///
  /// Byte 0x30: The offset for the API to setup a temporary stack till the memory is initialized.
  ///
  UINT32  TempRamInitEntryOffset;
  ///
  /// Byte 0x34: Reserved3.
  ///
  UINT32  Reserved3;
  ///
  /// Byte 0x38: The offset for the API to inform the FSP about the different stages in the boot process.
  ///
  UINT32  NotifyPhaseEntryOffset;
  ///
  /// Byte 0x3C: The offset for the API to initialize the memory.
  ///
  UINT32  FspMemoryInitEntryOffset;
  ///
  /// Byte 0x40: The offset for the API to tear down temporary RAM.
  ///
  UINT32  TempRamExitEntryOffset;
  ///
  /// Byte 0x44: The offset for the API to initialize the CPU and chipset.
  ///
  UINT32  FspSiliconInitEntryOffset;
} FSP_INFO_HEADER;

///
/// Signature of the FSP Extended Header
///
#define FSP_INFO_EXTENDED_HEADER_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'E')

///
/// FSP Information Extended Header as described in FSP v2.0 Spec section 5.1.2.
///
typedef struct {
  ///
  /// Byte 0x00: Signature ('FSPE') for the FSP Extended Information Header.
  ///
  UINT32  Signature;
  ///
  /// Byte 0x04: Length of the table in bytes, including all additional FSP producer defined data.
  ///
  UINT32  Length;
  ///
  /// Byte 0x08: FSP producer defined revision of the table.
  ///
  UINT8   Revision;
  ///
  /// Byte 0x09: Reserved for future use.
  ///
  UINT8   Reserved;
  ///
  /// Byte 0x0A: FSP producer identification string
  ///
  CHAR8   FspProducerId[6];
  ///
  /// Byte 0x10: FSP producer implementation revision number. Larger numbers are assumed to be newer revisions.
  ///
  UINT32  FspProducerRevision;
  ///
  /// Byte 0x14: Size of the FSP producer defined data (n) in bytes.
  ///
  UINT32  FspProducerDataSize;
  ///
  /// Byte 0x18: FSP producer defined data of size (n) defined by FspProducerDataSize.
  ///
} FSP_INFO_EXTENDED_HEADER;

//
// A generic table search algorithm for additional tables can be implemented with a
// signature search algorithm until a terminator signature 'FSPP' is found.
//
#define FSP_FSPP_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'P')
#define FSP_PATCH_TABLE_SIGNATURE  FSP_FSPP_SIGNATURE

///
/// FSP Patch Table as described in FSP v2.0 Spec section 5.1.5.
///
typedef struct {
  ///
  /// Byte 0x00: FSP Patch Table Signature "FSPP".
  ///
  UINT32  Signature;
  ///
  /// Byte 0x04: Size including the PatchData.
  ///
  UINT16  HeaderLength;
  ///
  /// Byte 0x06: Revision is set to 0x01.
  ///
  UINT8   HeaderRevision;
  ///
  /// Byte 0x07: Reserved for future use.
  ///
  UINT8   Reserved;
  ///
  /// Byte 0x08: Number of entries to Patch.
  ///
  UINT32  PatchEntryNum;
  ///
  /// Byte 0x0C: Patch Data.
  ///
//UINT32  PatchData[];
} FSP_PATCH_TABLE;

#pragma pack()

extern EFI_GUID gFspHeaderFileGuid;

#endif
