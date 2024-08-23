/** @file
  Base Panic Library with empty functions.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/PanicLib.h>

/**
  This would usually print a panic message containing a filename, line number,
  and description.  For the NULL implementation nothing is done when panicking.

  @param  FileName     The pointer to the name of the source file that generated the panic condition.
  @param  LineNumber   The line number in the source file that generated the panic condition
  @param  Description  The pointer to the description of the panic condition.

**/
VOID
EFIAPI
PanicReport (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
}
