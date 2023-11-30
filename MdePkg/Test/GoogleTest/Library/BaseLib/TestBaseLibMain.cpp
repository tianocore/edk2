/** @file
  Main routine for BaseLib google tests.

  Copyright (c) 2023 Pedro Falcato. All rights reserved<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <gtest/gtest.h>

// Note: Until we can --whole-archive libs, we're forced to include secondary files from the main one.
// Yuck.

#include "TestCheckSum.cpp"

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
