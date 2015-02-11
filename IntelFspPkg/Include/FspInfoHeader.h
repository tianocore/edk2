/** @file
  Intel FSP Info Header definition from Intel Firmware Support Package External
  Architecture Specification, April 2014, revision 001.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  /// Byte 0x18: FSP binary preferred base address
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
  /// Byte 0x24: Size of the FSP configuration region
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
  /// Below field is added in FSP 1.1
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
/// Below structure is added in FSP 1.1
///
typedef struct  {
  ///
  /// Byte 0x00: Signature ('FSPE') for the FSP Extended Information Header
  ///
  UINT32  Signature;
  ///
  /// Byte 0x04: Length of the FSP Extended Header
  ///
  UINT32  HeaderLength;
  ///
  /// Byte 0x08: Revision of the FSP Extended Header
  ///
  UINT8   Revision;
  ///
  /// Byte 0x09: Reserved for future use.
  ///
  UINT8   Reserved;
  ///
  /// Byte 0x0A: An OEM-supplied string that defines the OEM
  ///
  CHAR8   OemId[6];
  ///
  /// Byte 0x10: An OEM-supplied revision number. Larger numbers are assumed to be newer revisions.
  ///
  UINT32  OemRevision;

} FSP_EXTENTED_HEADER;

#pragma pack()

#endif
