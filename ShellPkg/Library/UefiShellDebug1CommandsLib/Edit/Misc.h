/** @file
  Declares generic editor helper functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_MISC_H_
#define _LIB_MISC_H_

#include "TextEditorTypes.h"



/**
  Free a EFI_EDITOR_LINE structure.

  @param Src                    The line structure to free.
**/
VOID
LineFree (
  IN  EFI_EDITOR_LINE *Src
  );

/**
  Duplicate a EFI_EDITOR_LINE structure.

  @param Src                    The line structure to copy from.

  @retval NULL                  A memory allocation failed.
  @return                       a pointer to the newly allcoated line.
**/
EFI_EDITOR_LINE *
LineDup (
  IN  EFI_EDITOR_LINE *Src
  );






#endif
