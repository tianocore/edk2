/** @file
  Header for the RNDR APIs used by RNG DXE driver.

  Support API definitions for RNDR instruction access.


  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RNDR_H_
#define RNDR_H_

#include <Library/BaseLib.h>
#include <Protocol/Rng.h>

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
  );

#endif  // RNDR_H_
