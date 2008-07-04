/** @file
  Language setting

Copyright (c) 2004 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LANGUAGE_H
#define _LANGUAGE_H

#include "String.h"

/**
  Determine the current language that will be used
  based on language related EFI Variables


  @param LangCodesSettingRequired - If required to set LangCode variable

  @return VOID

**/
VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
;

#endif // _LANGUAGE_H_
