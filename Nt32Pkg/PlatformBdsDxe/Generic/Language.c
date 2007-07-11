/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  language.c

Abstract:

 Language settings

Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "BdsString.h"
#include "Language.h"

//
// Default language code, currently is English
//
CHAR8 *mDefaultLangCode = "eng";


VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
/*++

Routine Description:
  Determine the current language that will be used
  based on language related EFI Variables

Arguments:
  LangCodesSettingRequired - If required to set LangCode variable

Returns:

--*/
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       Size;
  CHAR8       LangCode[ISO_639_2_ENTRY_SIZE];
  CHAR8       *LangCodes;
  CHAR16      *LanguageString;

  LanguageString  = NULL;
  LangCodes       = NULL;

  //
  // Collect the languages from what our current Language support is based on our VFR
  //
  gHii->GetPrimaryLanguages (gHii, gStringPackHandle, &LanguageString);

  LangCodes = AllocatePool (StrLen (LanguageString));
  ASSERT (LangCodes);

  //
  // Convert LanguageString from Unicode to EFI defined ASCII LangCodes
  //
  for (Index = 0; LanguageString[Index] != 0x0000; Index++) {
    LangCodes[Index] = (CHAR8) LanguageString[Index];
  }

  LangCodes[Index] = 0;

  if (LangCodesSettingRequired) {
    Status = gRT->SetVariable (
                    L"LangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrLen (LangCodes),
                    LangCodes
                    );
  }
  //
  // Find current LangCode from Lang NV Variable
  //
  Size = ISO_639_2_ENTRY_SIZE;
  Status = gRT->GetVariable (
                  L"Lang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  &LangCode
                  );

  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    for (Index = 0; LangCodes[Index] != 0; Index += ISO_639_2_ENTRY_SIZE) {
      if (CompareMem (&LangCodes[Index], LangCode, ISO_639_2_ENTRY_SIZE) == 0) {
        Status = EFI_SUCCESS;
        break;
      }
    }
  }
  //
  // If we cannot get language code from Lang variable,
  // or LangCode cannot be found from language table,
  // set the mDefaultLangCode to Lang variable.
  //
  if (EFI_ERROR (Status)) {
    Status = gRT->SetVariable (
                    L"Lang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    ISO_639_2_ENTRY_SIZE,
                    mDefaultLangCode
                    );
  }

  if (LangCodes) {
    FreePool (LangCodes);
  }

  if (LanguageString != NULL) {
    FreePool (LanguageString);
  }

}
