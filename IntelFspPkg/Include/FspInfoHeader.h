/** @file
  Intel FSP Info Header definition from Intel Firmware Support Package External
  Architecture Specification v1.1, April 2015, revision 001.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_INFO_HEADER_H_
#define _FSP_INFO_HEADER_H_

#define FSP_HEADER_REVISION_1   1
#define FSP_HEADER_REVISION_2   2

#define FSPE_HEADER_REVISION_1  1
#define FSPP_HEADER_REVISION_1  1

///
/// Fixed FSP header offset in the FSP image
///
#define  FSP_INFO_HEADER_OFF    0x94

#define  OFFSET_IN_FSP_INFO_HEADER(x)  (UINT32)&((FSP_INFO_HEADER *)(UINTN)0)->x

#define FSP_INFO_HEADER_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'H')

#pragma pack(1)

typedef struct  {
  ///
  /// Byte 0x00: Signature ('FSPH') for the FSP Information Header
  ///
  UINT32  Signature;
  ///
  /// Byte 0x04: Length of the FSP Information Header
  ///
  UINT32  HeaderLength;
  ///
  /// Byte 0x08: Reserved
  ///
  UINT8   Reserved1[3];
  ///
  /// Byte 0x0B: Revision of the FSP Information Header
  ///
  UINT8   HeaderRevision;
  ///
  /// Byte 0x0C: Revision of the FSP binary
  ///
  UINT32  ImageRevision;


  ///
  /// Byte 0x10: Signature string that will help match the FSP Binary to a supported
  /// hardware configuration.
  ///
  CHAR8   ImageId[8];
  ///
  /// Byte 0x18: Size of the entire FSP binary
  ///
  UINT32  ImageSize;
  ///
  /// Byte 0x1C: FSP binary preferred base address
  ///
  UINT32  ImageBase;


  ///
  /// Byte 0x20: Attribute for the FSP binary
  ///
  UINT32  ImageAttribute;
  ///
  /// Byte 0x24: Offset of the FSP configuration region
  ///
  UINT32  CfgRegionOffset;
  ///
  /// Byte 0x28: Size of the FSP configuration region
  ///
  UINT32  CfgRegionSize;
  ///
  /// Byte 0x2C: Number of API entries this FSP supports
  ///
  UINT32  ApiEntryNum;


  ///
  /// Byte 0x30: The offset for the API to setup a temporary stack till the memory
  ///            is initialized.
  ///
  UINT32  TempRamInitEntryOffset;
  ///
  /// Byte 0x34: The offset for the API to initialize the CPU and the chipset (SOC)
  ///
  UINT32  FspInitEntryOffset;
  ///
  /// Byte 0x38: The offset for the API to inform the FSP about the different stages
  ///            in the boot process
  ///
  UINT32  NotifyPhaseEntryOffset;

  ///
  /// Below fields are added in FSP Revision 2
  ///

  ///
  /// Byte 0x3C: The offset for the API to initialize the memory
  ///
  UINT32  FspMemoryInitEntryOffset;
  ///
  /// Byte 0x40: The offset for the API to tear down temporary RAM
  ///
  UINT32  TempRamExitEntryOffset;
  ///
  /// Byte 0x44: The offset for the API to initialize the CPU and chipset
  ///
  UINT32  FspSiliconInitEntryOffset;

} FSP_INFO_HEADER;

///
/// Below structure is added in FSP version 2
///
#define FSP_INFO_EXTENDED_HEADER_SIGNATURE  SIGNATURE_32 ('F', 'S', 'P', 'E')

typedef struct  {
  ///
  /// Byte 0x00: Signature ('FSPE') for the FSP Extended Information Header
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

#pragma pack()

#endif
