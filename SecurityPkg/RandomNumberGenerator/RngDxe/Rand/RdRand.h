/** @file
  Header for the RDRAND APIs used by RNG DXE driver.

  Support API definitions for RDRAND instruction access, which will leverage
  Intel Secure Key technology to provide high-quality random numbers for use
  in applications, or entropy for seeding other random number generators.
  Refer to http://software.intel.com/en-us/articles/intel-digital-random-number
  -generator-drng-software-implementation-guide/ for more information about Intel
  Secure Key technology.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __RD_RAND_H__
#define __RD_RAND_H__

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Protocol/Rng.h>

/**
  Calls RDRAND to fill a buffer of arbitrary size with random bytes.

  @param[in]   Length        Size of the buffer, in bytes,  to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random bytes generation succeeded.
  @retval EFI_NOT_READY      Failed to request random bytes.

**/
EFI_STATUS
EFIAPI
RdRandGetBytes (
  IN UINTN         Length,
  OUT UINT8        *RandBuffer
  );

/**
  Generate high-quality entropy source through RDRAND.

  @param[in]   Length        Size of the buffer, in bytes, to fill with.
  @param[out]  Entropy       Pointer to the buffer to store the entropy data.

  @retval EFI_SUCCESS        Entropy generation succeeded.
  @retval EFI_NOT_READY      Failed to request random data.

**/
EFI_STATUS
EFIAPI
RdRandGenerateEntropy (
  IN UINTN         Length,
  OUT UINT8        *Entropy
  );

#endif  // __RD_RAND_H__
