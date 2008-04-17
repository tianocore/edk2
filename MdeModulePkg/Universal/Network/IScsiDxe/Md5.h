/** @file
  Header file for Md5

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Md5.h

Abstract:

  Header file for Md5

**/

#ifndef _MD5_H_
#define _MD5_H_

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/NetLib.h>

#define MD5_HASHSIZE  16

typedef struct _MD5_CTX {
  EFI_STATUS  Status;
  UINT64      Length;
  UINT32      States[MD5_HASHSIZE / sizeof (UINT32)];
  UINT8       M[64];
  UINTN       Count;
} MD5_CTX;

/**
  Initialize four 32-bits chaining variables and use them to do the Md5 transform.

  @param  Md5Ctx[in]  the data structure of Md5

  @retval EFI_SUCCESS initialization is ok

**/
EFI_STATUS
MD5Init (
  IN MD5_CTX  *Md5Ctx
  )
;

/**
  the external interface of Md5 algorithm

  @param  Md5Ctx[in]  the data structure of storing the original data
                      segment and the final result.

  @param  Data[in]    the data wanted to be transformed.

  @param  DataLen[in] the length of data.

  @retval EFI_SUCCESS the transform is ok.

**/
EFI_STATUS
MD5Update (
  IN MD5_CTX  *Md5Ctx,
  IN VOID     *Data,
  IN UINTN    DataLen
  )
;

/**
  accumulate the MD5 value of every data segment and generate the finial
  result according to MD5 algorithm

  @param  Md5Ctx[in]   the data structure of storing the original data
                       segment and the final result.

  @param  HashVal[out] the final 128-bits output.

  @retval EFI_SUCCESS  the transform is ok.

**/
EFI_STATUS
MD5Final (
  IN  MD5_CTX  *Md5Ctx,
  OUT UINT8    *HashVal
  )
;

#endif // _MD5_H
