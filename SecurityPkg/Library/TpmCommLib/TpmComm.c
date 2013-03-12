/** @file
  Basic TPM command functions.

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CommonHeader.h"

/**
  Single function calculates SHA1 digest value for all raw data. It
  combines Sha1Init(), Sha1Update() and Sha1Final().

  @param[in]  Data          Raw data to be digested.
  @param[in]  DataLen       Size of the raw data.
  @param[out] Digest        Pointer to a buffer that stores the final digest.
  
  @retval     EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
TpmCommHashAll (
  IN  CONST UINT8                   *Data,
  IN        UINTN                   DataLen,
  OUT       TPM_DIGEST              *Digest
  )
{
  VOID     *Sha1Ctx;
  UINTN    CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init (Sha1Ctx);
  Sha1Update (Sha1Ctx, Data, DataLen);
  Sha1Final (Sha1Ctx, (UINT8 *)Digest);

  FreePool (Sha1Ctx);

  return EFI_SUCCESS;
}

