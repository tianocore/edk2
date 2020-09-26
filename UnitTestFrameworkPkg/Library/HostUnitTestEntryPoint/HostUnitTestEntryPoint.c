/** @file -- HostUnitTestEntryPoint.c
An entry point lib to be consumed by host-based unit tests.
Turns a standard C entry point into one that passes ECC.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

VOID
EFIAPI
UnitTestMain (
    VOID
    );

/**
  Standard C entry point.
**/
INT32
main (
  VOID
  )
{
    UnitTestMain ();
    return 0;
}
