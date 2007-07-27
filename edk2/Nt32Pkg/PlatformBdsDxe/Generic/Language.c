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
#include "Bds.h"
#include "BdsString.h"
#include "Language.h"


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
  UINTN       Size;
  CHAR8       *Lang;
  CHAR8       LangCode[ISO_639_2_ENTRY_SIZE];
  CHAR8       *LangCodes;
  CHAR8       *PlatformLang;
  CHAR8       *PlatformLangCodes;
  UINTN       Index;
  BOOLEAN     Invalid;


  LangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLangCodes);
  if (LangCodesSettingRequired) {
    if (!FeaturePcdGet (PcdUefiVariableDefaultLangDepricate)) {
      //
      // UEFI 2.1 depricated this variable so we support turning it off
      //
      Status = gRT->SetVariable (
                      L"LangCodes",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrLen (LangCodes),
                      LangCodes
                      );
    }


    PlatformLangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes);
    Status = gRT->SetVariable (
                    L"PlatformLangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLangCodes),
                    PlatformLangCodes
                    );
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDepricate)) {
    //
    // UEFI 2.1 depricated this variable so we support turning it off
    //

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
      Lang = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLang);
      Status = gRT->SetVariable (
                      L"Lang",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      ISO_639_2_ENTRY_SIZE,
                      Lang
                      );
    }
  }

  Invalid = FALSE;
  PlatformLang = BdsLibGetVariableAndSize (L"PlatformLang", &gEfiGlobalVariableGuid, &Size);
  if (PlatformLang != NULL) {
    //
    // Check Current PlatformLang value against PlatformLangCode. Need a library that is TBD
    // Set Invalid based on state of PlatformLang.
    //

    FreePool (PlatformLang);
  } else {
    // No valid variable is set
    Invalid = TRUE;
  }

  if (Invalid) {
    PlatformLang = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLang);
    Status = gRT->SetVariable (
                    L"PlatformLang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLang),
                    PlatformLang
                    );
  }
}
