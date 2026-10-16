/** @file
  This is a sample to demonstrates the use of GoogleTest that supports host
  execution environments.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/GoogleTestLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/HostMemoryAllocationBelowAddressLib.h>
}

UINTN
DivideWithNoParameterChecking (
  UINTN  Dividend,
  UINTN  Divisor
  )
{
  //
  // Perform integer division with no check for divide by zero
  //
  return (Dividend / Divisor);
}

/**
  Sample unit test that performs a divide by 0
**/
TEST (SanitizerTests, DivideByZeroDeathTest) {
  //
  // Divide by 0 should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (DivideWithNoParameterChecking (10, 0), "ERROR: AddressSanitizer: ");
}
