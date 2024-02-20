/** @file
  Acts as the main entry point for the tests for the UefiPxeBcDxe module.
  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <gtest/gtest.h>

////////////////////////////////////////////////////////////////////////////////
// Run the tests
////////////////////////////////////////////////////////////////////////////////
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
