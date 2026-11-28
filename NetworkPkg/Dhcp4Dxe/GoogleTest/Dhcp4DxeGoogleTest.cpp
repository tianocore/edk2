/** @file
  Acts as the main entry point for the tests for the Dhcp4Dxe module.

  Copyright (c) 2026 Loongson Technology Corporation Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

//////////////////////////////////////////////////////////////////////////////
// Run the tests
//////////////////////////////////////////////////////////////////////////////
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
