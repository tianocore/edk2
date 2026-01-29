/** @file
  Language setting

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

/**
  Determine the current language that will be used
  based on language related EFI Variables.

  @param LangCodesSettingRequired If required to set LangCode variable

**/
VOID
InitializeLanguage (
  BOOLEAN  LangCodesSettingRequired
  );

#endif // _LANGUAGE_H_
