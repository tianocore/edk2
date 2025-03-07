/** @file
  This is a sample to demostrate the usage of the Unit Test Library that
  supports the host execution environments.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "TestBaseCryptLib.h"

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  ProcessLibraryConstructorList ();
  return UefiTestMain ();
}
