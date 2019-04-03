/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
