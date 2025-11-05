/** @file
  This is a sample to demonstrates the use of GoogleTest that supports host
  execution environments for test case that generates an exception.  For some
  host-based environments, this is a fatal condition that terminates the unit
  tests and no additional test cases are executed. On other environments, this
  condition may be report a unit test failure and continue with additional unit
  tests.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
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
  Sample unit test that generates an unexpected exception
**/
TEST (ExceptionTest, GenerateExceptionExpectTestFail) {
  //
  // Assertion that passes without generating an exception
  //
  EXPECT_EQ (DivideWithNoParameterChecking (20, 1), (UINTN)20);
  //
  // Assertion that generates divide by zero exception before result evaluated
  //
  EXPECT_EQ (DivideWithNoParameterChecking (20, 0), MAX_UINTN);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
