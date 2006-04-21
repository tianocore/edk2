/** @file
  Defines for the EFI Capsule functionality.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Capsule.h

  @par Revision Reference:
  These definitions are from Capsule Spec Version 0.9.

**/

#ifndef _EFI_CAPSULE_H_
#define _EFI_CAPSULE_H_

//
// Bits in the flags field of the capsule header
//
#define EFI_CAPSULE_HEADER_FLAG_SETUP 0x00000001  // supports setup changes


#define CAPSULE_BLOCK_DESCRIPTOR_SIGNATURE  EFI_SIGNATURE_32 ('C', 'B', 'D', 'S')

//
// An array of these describe the blocks that make up a capsule for
// a capsule update.
//
typedef struct {
  UINT64                Length;     // length of the data block
  EFI_PHYSICAL_ADDRESS  Data;       // physical address of the data block
  UINT32                Signature;  // CBDS
  UINT32                CheckSum;   // to sum this structure to 0
} EFI_CAPSULE_BLOCK_DESCRIPTOR;

typedef struct {
  EFI_GUID  OemGuid;
  UINT32    HeaderSize;
  //
  // UINT8                       OemHdrData[];
  //
} EFI_CAPSULE_OEM_HEADER;

typedef struct {
  EFI_GUID  CapsuleGuid;
  UINT32    HeaderSize;
  UINT32    Flags;
  UINT32    CapsuleImageSize;
  UINT32    SequenceNumber;
  EFI_GUID  InstanceId;
  UINT32    OffsetToSplitInformation;
  UINT32    OffsetToCapsuleBody;
  UINT32    OffsetToOemDefinedHeader;
  UINT32    OffsetToAuthorInformation;
  UINT32    OffsetToRevisionInformation;
  UINT32    OffsetToShortDescription;
  UINT32    OffsetToLongDescription;
  UINT32    OffsetToApplicableDevices;
} EFI_CAPSULE_HEADER;

#endif // #ifndef _EFI_CAPSULE_H_
