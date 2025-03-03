/** @file
  C Run-Time Libraries (CRT) Utility apis for BoringSSL-based
  Cryptographic Library.

Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CrtLibSupport.h>
#include <Uefi/UefiBaseType.h>
#include <Library/RngLib.h>
#include <Library/SafeIntLib.h>

/* Performs a binary search */
void *
bsearch (
  const void *key,
  const void *base,
  size_t nmemb,
  size_t size,
  int         ( *compar )(const void *, const void *)
  )
{
  void           *Mid;
  int            Sign;
  RETURN_STATUS  Status = RETURN_INVALID_PARAMETER;
  size_t         Result;

  if (!key || !base || !nmemb || !size) {
    return NULL;
  }

  Status = SafeUintnMult ((UINTN)size, (UINTN)(nmemb/2), (UINTN *)&Result);

  if ((Status == RETURN_BUFFER_TOO_SMALL) ||
      (Status == RETURN_INVALID_PARAMETER))
  {
    return NULL;
  }

  while (nmemb > 0) {
    Mid  = (char *)base + size * (nmemb/2);
    Sign = compar (key, Mid);
    if (Sign < 0) {
      nmemb /= 2;
    } else if (Sign > 0) {
      base   = (char *)Mid + size;
      nmemb -= nmemb/2 + 1;
    } else {
      return Mid;
    }
  }

  return NULL;
}

/* Returns entropy of requested length in provided buffer */
int
getentropy (
  void    *buffer,
  size_t  length
  )
{
  UINT8   *EntropyBuffer = (UINT8 *)buffer;
  UINTN   Index;
  UINT64  RandNum;
  UINTN   CopyLength;

  if (length > GETENTROPY_MAX) {
    errno = EIO;
    return -1;
  }

  if (EntropyBuffer == NULL) {
    errno = EFAULT;
    return -1;
  }

  for (Index = 0; Index < length; Index += sizeof (UINT64)) {
    if (!GetRandomNumber64 (&RandNum)) {
      errno = ENOSYS;
      return -1;
    }

    CopyLength =
      (length - Index >= sizeof (UINT64)) ? sizeof (UINT64) : (length - Index);
    CopyMem (EntropyBuffer + Index, &RandNum, CopyLength);
  }

  return 0;
}
