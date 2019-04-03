/** @file

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_LOCK_BOX_LIB_PRIVATE_H_
#define _SMM_LOCK_BOX_LIB_PRIVATE_H_

#include <Uefi.h>

#pragma pack(1)

//
// Below data structure is used for lockbox registration in SMST
//

#define SMM_LOCK_BOX_SIGNATURE_32 SIGNATURE_64 ('L','O','C','K','B','_','3','2')
#define SMM_LOCK_BOX_SIGNATURE_64 SIGNATURE_64 ('L','O','C','K','B','_','6','4')

typedef struct {
  UINT64                   Signature;
  EFI_PHYSICAL_ADDRESS     LockBoxDataAddress;
} SMM_LOCK_BOX_CONTEXT;

//
// Below data structure is used for lockbox management
//

#define SMM_LOCK_BOX_DATA_SIGNATURE SIGNATURE_64 ('L','O','C','K','B','O','X','D')

typedef struct {
  UINT64                         Signature;
  EFI_GUID                       Guid;
  EFI_PHYSICAL_ADDRESS           Buffer;
  UINT64                         Length;
  UINT64                         Attributes;
  EFI_PHYSICAL_ADDRESS           SmramBuffer;
  LIST_ENTRY                     Link;
} SMM_LOCK_BOX_DATA;

#pragma pack()

#endif

