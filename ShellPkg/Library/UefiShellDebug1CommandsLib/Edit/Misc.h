/** @file
  Declares generic editor helper functions.

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

#include "TextEditorTypes.h"



/**
  Free a EFI_EDITOR_LINE structure.

  @param Src                    The line structure to free.
**/
VOID
EFIAPI
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
EFIAPI
LineDup (
  IN  EFI_EDITOR_LINE *Src
  );






#endif
