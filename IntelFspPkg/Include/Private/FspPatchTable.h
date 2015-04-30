/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_PATCH_TABLE_H_
#define _FSP_PATCH_TABLE_H_

#pragma pack(1)

#define FSP_PATCH_TABLE_SIGNATURE  FSP_FSPP_SIGNATURE

typedef struct  {
  UINT32  Signature;
  UINT16  HeaderLength;
  UINT8   HeaderRevision;
  UINT8   Reserved;
  UINT32  PatchEntryNum;
  UINT32  PatchData[FixedPcdGet32(PcdFspMaxPatchEntry)];
} FSP_PATCH_TABLE;

#pragma pack()

#endif
