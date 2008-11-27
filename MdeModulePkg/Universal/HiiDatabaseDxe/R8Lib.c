/** @file
Implement a utility function named R8_EfiLibCompareLanguage.

  Copyright (c) 2007 - 2008, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "HiiDatabase.h"


/**
  Compare whether two names of languages are identical.

  @param  Language1              Name of language 1
  @param  Language2              Name of language 2

  @retval TRUE                   same
  @retval FALSE                  not same

**/
BOOLEAN
R8_EfiLibCompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
{
  //
  // Porting Guide:
  // This library interface is simply obsolete.
  // Include the source code to user code.
  //
  UINTN Index;

  for (Index = 0; (Language1[Index] != 0) && (Language2[Index] != 0); Index++) {
    if (Language1[Index] != Language2[Index]) {
      return FALSE;
    }
  }

  if (((Language1[Index] == 0) && (Language2[Index] == 0))   || 
  	  ((Language1[Index] == 0) && (Language2[Index] != ';')) ||
  	  ((Language1[Index] == ';') && (Language2[Index] != 0)) ||
  	  ((Language1[Index] == ';') && (Language2[Index] != ';'))) {
    return TRUE;
  }

  return FALSE;
}


/**
  Determine what is the current language setting. The space reserved for Lang
  must be at least RFC_3066_ENTRY_SIZE bytes;

  If Lang is NULL, then ASSERT.

  @param  Lang                   Pointer of system language. Lang will always be filled with 
                                 a valid RFC 3066 language string. If "PlatformLang" is not
                                 set in the system, the default language specifed by PcdUefiVariableDefaultPlatformLang
                                 is returned.

  @return  EFI_SUCCESS     If the EFI Variable with "PlatformLang" is set and return in Lang.
  @return  EFI_NOT_FOUND If the EFI Variable with "PlatformLang" is not set, but a valid default language is return in Lang.

**/
EFI_STATUS
EFIAPI
GetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  ASSERT (Lang != NULL);

  //
  // Get current language setting
  //
  Size = RFC_3066_ENTRY_SIZE;
  Status = gRT->GetVariable (
                  L"PlatformLang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  Lang
                  );

  if (EFI_ERROR (Status)) {
    AsciiStrCpy (Lang, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang));
  }

  return Status;
}


