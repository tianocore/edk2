/** @file
    Definitions for various line and string routines

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_MISC_H_
#define _LIB_MISC_H_

#include "HexEditor.h"

VOID
HEditorClearLine (
  UINTN
  );
HEFI_EDITOR_LINE  *
HLineDup (
  HEFI_EDITOR_LINE *
  );
VOID
HLineFree (
  HEFI_EDITOR_LINE *
  );

HEFI_EDITOR_LINE  *
HMoveLine (
  INTN
  );
HEFI_EDITOR_LINE  *
HMoveCurrentLine (
  INTN
  );

UINTN
HLineStrInsert (
  HEFI_EDITOR_LINE  *,
  CHAR16,
  UINTN,
  UINTN
  );

VOID
HLineCat (
  HEFI_EDITOR_LINE *,
  HEFI_EDITOR_LINE *
  );

VOID
HLineDeleteAt (
  HEFI_EDITOR_LINE  *,
  UINTN
  );

UINTN
HUnicodeToAscii (
  CHAR16    *,
  UINTN,
  CHAR8     *
  );

UINTN
HStrStr (
  CHAR16    *,
  CHAR16    *
  );

EFI_STATUS
HFreeLines (
  LIST_ENTRY   *,
  HEFI_EDITOR_LINE *
  );

INT32
HGetTextX (
  INT32
  ) ;
INT32
HGetTextY (
  INT32
  ) ;

EFI_STATUS
HXtoi (
  CHAR16    *,
  UINTN     *
  );

#endif
