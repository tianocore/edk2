/** @file
  Support routines for RNDR instruction access.

  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/RngLib.h>

#include "Rndr.h"

/**
  Calls RNDR to fill a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RndrGetBytes (
  IN UINTN         Length,
  OUT UINT8        *RandBuffer
  )
{
  BOOLEAN     IsRandom;
  UINT64      TempRand;

  while (Length > 0) {
    IsRandom = GetRandomNumber64 (&TempRand);
    if (!IsRandom) {
      return EFI_NOT_READY;
    }
    if (Length >= sizeof (TempRand)) {
      WriteUnaligned64 ((UINT64*)RandBuffer, TempRand);
      RandBuffer += sizeof (UINT64);
      Length -= sizeof (TempRand);
    } else {
      CopyMem (RandBuffer, &TempRand, Length);
      Length = 0;
    }
  }

  return EFI_SUCCESS;
}

