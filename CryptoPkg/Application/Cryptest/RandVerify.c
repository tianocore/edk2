/** @file  
  Application for Pseudorandom Number Generator Validation.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Cryptest.h"

#define  RANDOM_NUMBER_SIZE  256

CONST  UINT8  SeedString[] = "This is the random seed for PRNG verification.";

UINT8  PreviousRandomBuffer[RANDOM_NUMBER_SIZE] = { 0x0 };

UINT8  RandomBuffer[RANDOM_NUMBER_SIZE] = { 0x0 };

/**
  Validate UEFI-OpenSSL pseudorandom number generator interfaces.

  @retval  EFI_SUCCESS  Validation succeeded.
  @retval  EFI_ABORTED  Validation failed.

**/
EFI_STATUS
ValidateCryptPrng (
  VOID
  )
{
  UINTN    Index;
  BOOLEAN  Status;

  Print (L" \nUEFI-OpenSSL PRNG Engine Testing:\n");

  Print (L"- Random Generation...");

  Status = RandomSeed (SeedString, sizeof (SeedString));
  if (!Status) {
    Print (L"[Fail]");
    return EFI_ABORTED;
  }

  for (Index = 0; Index < 10; Index ++) {
    Status = RandomBytes (RandomBuffer, RANDOM_NUMBER_SIZE);
    if (!Status) {
      Print (L"[Fail]");
      return EFI_ABORTED;
    }

    if (CompareMem (PreviousRandomBuffer, RandomBuffer, RANDOM_NUMBER_SIZE) == 0) {
      Print (L"[Fail]");
      return EFI_ABORTED;
    }

    CopyMem (PreviousRandomBuffer, RandomBuffer, RANDOM_NUMBER_SIZE);
  }

  Print (L"[Pass]\n");

  return EFI_SUCCESS;

}
