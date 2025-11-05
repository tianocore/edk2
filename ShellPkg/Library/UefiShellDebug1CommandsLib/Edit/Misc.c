/** @file
  Implementation of various string and line routines.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TextEditor.h"
#include "Misc.h"

/**
  Duplicate a EFI_EDITOR_LINE structure.

  @param Src                    The line structure to copy from.

  @retval NULL                  A memory allocation failed.
  @return                       a pointer to the newly allcoated line.
**/
EFI_EDITOR_LINE *
LineDup (
  IN  EFI_EDITOR_LINE  *Src
  )
{
  EFI_EDITOR_LINE  *Dest;

  //
  // allocate for the line structure
  //
  Dest = AllocateZeroPool (sizeof (EFI_EDITOR_LINE));
  if (Dest == NULL) {
    return NULL;
  }

  //
  // allocate and set the line buffer
  //
  Dest->Buffer = CatSPrint (NULL, L"%s", Src->Buffer);
  if (Dest->Buffer == NULL) {
    FreePool (Dest);
    return NULL;
  }

  //
  // set the other structure members
  //
  Dest->Signature = LINE_LIST_SIGNATURE;
  Dest->Size      = Src->Size;
  Dest->TotalSize = Dest->Size;
  Dest->Type      = Src->Type;
  Dest->Link      = Src->Link;

  return Dest;
}

/**
  Free a EFI_EDITOR_LINE structure.

  @param Src                    The line structure to free.
**/
VOID
LineFree (
  IN  EFI_EDITOR_LINE  *Src
  )
{
  if (Src == NULL) {
    return;
  }

  //
  // free the line buffer and then the line structure itself
  //
  SHELL_FREE_NON_NULL (Src->Buffer);
  SHELL_FREE_NON_NULL (Src);
}
