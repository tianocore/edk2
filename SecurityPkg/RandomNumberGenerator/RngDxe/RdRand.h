/** @file
  Header for the RDRAND APIs used by RNG DXE driver.

  Support API definitions for RDRAND instruction access, which will leverage
  Intel Secure Key technology to provide high-quality random numbers for use
  in applications, or entropy for seeding other random number generators.
  Refer to http://software.intel.com/en-us/articles/intel-digital-random-number
  -generator-drng-software-implementation-guide/ for more information about Intel
  Secure Key technology.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __RD_RAND_H__
#define __RD_RAND_H__

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Protocol/Rng.h>

//
// The maximun number of retries to obtain one available random number. 
//
#define RETRY_LIMIT    10

/**
  Determines whether or not RDRAND instruction is supported by the host hardware.

  @retval EFI_SUCCESS          RDRAND instruction supported.
  @retval EFI_UNSUPPORTED      RDRAND instruction not supported.

**/
EFI_STATUS
EFIAPI
IsRdRandSupported (
  VOID
  );

/**
  Generates a 16-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand16Step (
  OUT UINT16       *Rand
  );

/**
  Generates a 32-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand32Step (
  OUT UINT32       *Rand
  );

/**
  Generates a 64-bit random number through RDRAND instruction.

  @param[out]  Rand          Buffer pointer to store the random result.

  @retval TRUE               RDRAND call was successful.
  @retval FALSE              Failed attempts to call RDRAND.

**/
BOOLEAN
EFIAPI
RdRand64Step  (
  OUT UINT64   *Rand
  );

/**
  Calls RDRAND to obtain a 16-bit random number.

  @param[out]  Rand          Buffer pointer to store the random result.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        RDRAND call was successful.
  @retval EFI_NOT_READY      Failed attempts to call RDRAND.

**/
EFI_STATUS
EFIAPI
RdRand16 (
  OUT UINT16       *Rand,
  IN BOOLEAN       NeedRetry
  );

/**
  Calls RDRAND to obtain a 32-bit random number.

  @param[out]  Rand          Buffer pointer to store the random result.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        RDRAND call was successful.
  @retval EFI_NOT_READY      Failed attempts to call RDRAND.

**/
EFI_STATUS
EFIAPI
RdRand32 (
  OUT UINT32       *Rand,
  IN BOOLEAN       NeedRetry
  );

/**
  Calls RDRAND to obtain a 64-bit random number.

  @param[out]  Rand          Buffer pointer to store the random result.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        RDRAND call was successful.
  @retval EFI_NOT_READY      Failed attempts to call RDRAND.

**/
EFI_STATUS
EFIAPI
RdRand64 (
  OUT UINT64       *Rand,
  IN BOOLEAN       NeedRetry
  );
  
/**
  Calls RDRAND to request a word-length random number.

  @param[out]  Rand          Buffer pointer to store the random number.
  @param[in]   NeedRetry     Determine whether or not to loop retry.

  @retval EFI_SUCCESS        Random word generation succeeded.
  @retval EFI_NOT_READY      Failed to request random word.

**/
EFI_STATUS
EFIAPI
RdRandWord (
  OUT UINTN        *Rand,
  IN BOOLEAN       NeedRetry
  );

/**
  Calls RDRAND to request multiple word-length random numbers.

  @param[in]   Length        Size of the buffer, in words, to fill with.
  @param[out]  RandBuffer    Pointer to the buffer to store the random result.

  @retval EFI_SUCCESS        Random words generation succeeded.
  @retval EFI_NOT_READY      Failed to request random words.

**/
EFI_STATUS
EFIAPI
RdRandGetWords (
  IN UINTN         Length,
  OUT UINTN        *RandBuffer
  );

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