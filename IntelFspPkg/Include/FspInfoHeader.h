/** @file
  Intel FSP Info Header definition from Intel Firmware Support Package External
  Architecture Specification, April 2014, revision 001.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_INFO_HEADER_H_
#define _FSP_INFO_HEADER_H_

///
/// Fixed FSP header offset in the FSP image
///
#define  FSP_INFO_HEADER_OFF    0x94

#define  OFFSET_IN_FSP_INFO_HEADER(x)  (UINT32)&((FSP_INFO_HEADER *)(UINTN)0)->x

#pragma pack(1)

typedef struct  {
  ///
  /// Signature ('FSPH') for the FSP Information Header
  ///
  UINT32  Signature;
  ///
  /// Length of the FSP Information Header
  ///
  UINT32  HeaderLength;
  ///
  /// Reserved
  ///
  UINT8   Reserved1[3];
  ///
  /// Revision of the FSP Information Header
  ///
  UINT8   HeaderRevision;
  ///
  /// Revision of the FSP binary
  ///
  UINT32  ImageRevision;


  ///
  /// Signature string that will help match the FSP Binary to a supported
  /// hardware configuration.
  ///
  CHAR8   ImageId[8];
  ///
  /// Size of the entire FSP binary
  ///
  UINT32  ImageSize;
  ///
  /// FSP binary preferred base address
  ///
  UINT32  ImageBase;


  ///
  /// Attribute for the FSP binary
  ///
  UINT32  ImageAttribute;
  ///
  /// Offset of the FSP configuration region
  ///
  UINT32  CfgRegionOffset;
  ///
  /// Size of the FSP configuration region
  ///
  UINT32  CfgRegionSize;
  ///
  /// Number of API entries this FSP supports
  ///
  UINT32  ApiEntryNum;


  ///
  /// TempRamInit API entry offset
  ///
  UINT32  TempRamInitEntryOffset;
  ///
  /// FspInit API entry offset
  ///
  UINT32  FspInitEntryOffset;
  ///
  /// NotifyPhase API entry offset
  ///
  UINT32  NotifyPhaseEntryOffset;
  ///
  /// Reserved
  ///
  UINT32  Reserved2;

} FSP_INFO_HEADER;

#pragma pack()

#endif
