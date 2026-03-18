/** @file
  Unit tests for the GCC-style -fstack-protector stack cookie checking
  in StackCheckLib.

  This uses the actual StackCheckLib implementation in edk2 to validate
  that overflowing a buffer is caught.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Base.h>
  #include <Library/BaseLib.h>
}

using namespace testing;

//
// Overflow a stack buffer into the stack cookie. With stack cookies disabled (or StackCheckLibNull used)
// this function returns normally.
//
static
VOID
OverflowStackBuffer (
  VOID
  )
{
  UINTN           Index;
  volatile CHAR8  Buffer[16];

  for (Index = 0; Index < 30; Index++) {
    ((volatile CHAR8 *)Buffer)[Index] = (CHAR8)Index;
  }
}

TEST (StackCheckGccDeathTest, StackBufferOverflowDetected) {
  EXPECT_DEATH (OverflowStackBuffer (), "");
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
