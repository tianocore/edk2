/** @file
  Big number API implementation based on OpenSSL

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseCryptLib.h>
#include <Library/DebugLib.h>

VOID *
EFIAPI
BigNumInit (
  VOID
  )
{
  ASSERT(FALSE);
  return NULL;
}

VOID *
EFIAPI
BigNumFromBin(
  IN CONST UINT8 *Buf,
  IN UINTN Len)
{
  ASSERT(FALSE);
  return NULL;
}

INTN
EFIAPI
BigNumToBin(
  IN VOID *Bn,
  OUT UINT8 *Buf)
{
  ASSERT(FALSE);
  return -1;
}

VOID
EFIAPI
BigNumFree(
  IN VOID *Bn,
  IN BOOLEAN Clear
  )
{
  ASSERT(FALSE);
}

EFI_STATUS
EFIAPI
BigNumAdd(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumSub(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumExpMod(
  IN VOID *BnA,
  IN VOID *BnB,
  IN VOID *BnC,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumInverseMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumDiv(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumMulMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  IN CONST VOID *BnC,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

INTN
EFIAPI
BigNumCmp(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB)
{
  ASSERT(FALSE);
  return 0;
}

UINTN
EFIAPI
BigNumBits(
  IN CONST VOID *Bn
  )
{
  ASSERT(FALSE);
  return 0;
}

UINTN
EFIAPI
BigNumBytes(
  IN CONST VOID *Bn
  )
{
  ASSERT(FALSE);
  return 0;
}

BOOLEAN
EFIAPI
BigNumIsWord(
  IN CONST VOID *Bn,
  IN UINTN Num)
{
  ASSERT(FALSE);
  return FALSE;
}

BOOLEAN
EFIAPI
BigNumIsOdd(
  IN CONST VOID *Bn
  )
{
  ASSERT(FALSE);
  return FALSE;
}

VOID *
EFIAPI
BigNumCopy(
  OUT VOID *BnDst,
  IN CONST VOID *BnSrc
  )
{
  ASSERT(FALSE);
  return NULL;
}

CONST VOID *
EFIAPI
BigNumValueOne(
  VOID
  )
{
  ASSERT(FALSE);
  return NULL;
}

EFI_STATUS
EFIAPI
BigNumRShift(
  IN CONST VOID *Bn,
  IN UINTN n,
  OUT VOID *BnRes)
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

VOID
EFIAPI
BigNumConsttime(
  IN VOID *Bn
  )
{
  ASSERT(FALSE);
}

EFI_STATUS
EFIAPI
BigNumSqrMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  OUT VOID *BnRes)
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

VOID *
EFIAPI
BigNumNewContext(
  VOID
  )
{
  ASSERT(FALSE);
  return NULL;
}

VOID
EFIAPI
BigNumContextFree(
  IN VOID *BnCtx
)
{
  ASSERT(FALSE);
}

EFI_STATUS
EFIAPI
BigNumSetUint(
  IN VOID *Bn,
  IN UINTN Val
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
BigNumAddMod(
  IN CONST VOID *BnA,
  IN CONST VOID *BnB,
  IN CONST VOID *BnM,
  OUT VOID *BnRes
  )
{
  ASSERT(FALSE);
  return EFI_UNSUPPORTED;}

