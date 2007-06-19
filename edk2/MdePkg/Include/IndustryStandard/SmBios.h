/** @file
  Industry Standard Definitions of SMBIOS tables.


  Copyright (c) 2006 - 2007, Intel Corporation All rights
  reserved. This program and the accompanying materials are
  licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  SmBios.h

  @par Revision Reference: SMBIOS 2.0
  
**/

#ifndef __SMBIOS_STANDARD_H__
#define __SMBIOS_STANDARD_H__
//
// Smbios Table Entry Point Structure
//
#pragma pack(1)
typedef struct {
  UINT8   AnchorString[4];
  UINT8   EntryPointStructureChecksum;
  UINT8   EntryPointLength;
  UINT8   MajorVersion;
  UINT8   MinorVersion;
  UINT16  MaxStructureSize;
  UINT8   EntryPointRevision;
  UINT8   FormattedArea[5];
  UINT8   IntermediateAnchorString[5];
  UINT8   IntermediateChecksum;
  UINT16  TableLength;
  UINT32  TableAddress;
  UINT16  NumberOfSmbiosStructures;
  UINT8   SmbiosBcdRevision;
} SMBIOS_TABLE_ENTRY_POINT;

//
// The Smbios structure header
//
typedef struct {
  UINT8   Type;
  UINT8   Length;
  UINT16  Handle;
} SMBIOS_STRUCTURE;

#pragma pack()

#endif
