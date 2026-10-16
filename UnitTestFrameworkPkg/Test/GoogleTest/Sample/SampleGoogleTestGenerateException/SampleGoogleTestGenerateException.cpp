/** @file
  This is a sample to demonstrates the use of GoogleTest that supports host
  execution environments for test case that generates an exception.  For some
  host-based environments, this is a fatal condition that terminates the unit
  tests and no additional test cases are executed. On other environments, this
  condition may be report a unit test failure and continue with additional unit
  tests.

  A NULL pointer write is used to generate the exception because it produces
  a fatal CPU exception (translation/page fault) on every architecture
  supported by EDK II host-based testing.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
}

VOID
WriteByteWithNoParameterChecking (
  VOID   *Address,
  UINT8  Value
  )
{
  //
  // Perform a byte write to the supplied address with no validity check.
  //
  *(volatile UINT8 *)Address = Value;
}

/**
  Sample unit test that generates an unexpected exception by writing to a
  NULL pointer.
**/
TEST (ExceptionTest, GenerateExceptionExpectTestFail) {
  UINT8  LocalByte;

  LocalByte = 0;

  //
  // Assertion that passes without generating an exception
  //
  WriteByteWithNoParameterChecking (&LocalByte, 0xA5);
  EXPECT_EQ (LocalByte, (UINT8)0xA5);
  //
  // Assertion that generates a NULL pointer access exception before the
  // following EXPECT_EQ result is evaluated
  //
  WriteByteWithNoParameterChecking (NULL, 0xA5);
  EXPECT_EQ (LocalByte, (UINT8)0xA5);
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
