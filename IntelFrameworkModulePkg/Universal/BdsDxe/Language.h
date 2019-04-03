/** @file
  Language setting

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

#include "String.h"

/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
EFIAPI
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  );

/**
  Determine the current language that will be used
  based on language related EFI Variables.

  @param LangCodesSettingRequired If required to set LangCode variable

**/
VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  );

#endif // _LANGUAGE_H_
