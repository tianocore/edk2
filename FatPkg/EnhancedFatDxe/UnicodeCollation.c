/** @file
  Unicode Collation Library that hides the trival difference of Unicode Collation
  and Unicode collation 2 Protocol.

  Copyright (c) 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Fat.h"

STATIC EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollationInterface = NULL;

typedef
BOOLEAN
(* SEARCH_LANG_CODE) (
  IN CONST CHAR8    *Languages,
  IN CONST CHAR8    *MatchLangCode
  );

struct _UNICODE_INTERFACE {
  CHAR16            *VariableName;
  CHAR8             *DefaultLangCode;
  SEARCH_LANG_CODE  SearchLangCode;
  EFI_GUID          *UnicodeProtocolGuid;
};

typedef struct _UNICODE_INTERFACE UNICODE_INTERFACE;

STATIC
BOOLEAN
SearchIso639LangCode (
  IN CONST CHAR8    *Languages,
  IN CONST CHAR8    *MatchLangCode
  )
{
  CONST CHAR8       *LangCode;

  for (LangCode = Languages; *LangCode != '\0'; LangCode += 3) {
    if (CompareMem (LangCode, MatchLangCode, 3) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
BOOLEAN
SearchRfc3066LangCode (
  IN CONST CHAR8    *Languages,
  IN CONST CHAR8    *MatchLangCode
  )
{
  CHAR8       *SubStr;
  CHAR8       Terminal;

  SubStr = AsciiStrStr (Languages, MatchLangCode);
  if (SubStr == NULL) {
    return FALSE;
  }

  if (SubStr != Languages && *(SubStr - 1) != ';') {
    return FALSE;
  }

  Terminal = *(SubStr + AsciiStrLen (MatchLangCode));
  if (Terminal != '\0' && Terminal != ';') {
    return FALSE;
  }

  return TRUE;
}

GLOBAL_REMOVE_IF_UNREFERENCED UNICODE_INTERFACE mIso639Lang = {
  L"Lang",
  (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultLang),
  SearchIso639LangCode,
  &gEfiUnicodeCollationProtocolGuid,
};

GLOBAL_REMOVE_IF_UNREFERENCED UNICODE_INTERFACE mRfc3066Lang = {
  L"PlatformLang",
  (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang),
  SearchRfc3066LangCode,
  &gEfiUnicodeCollation2ProtocolGuid,
};

STATIC
EFI_STATUS
InitializeUnicodeCollationSupportWithConfig (
  IN EFI_HANDLE         AgentHandle,
  IN UNICODE_INTERFACE  *UnicodeInterface
  )
{
  EFI_STATUS                      Status;
  CHAR8                           Buffer[100];
  UINTN                           BufferSize;
  UINTN                           Index;
  CHAR8                           *LangCode;
  UINTN                           NoHandles;
  EFI_HANDLE                      *Handles;
  EFI_UNICODE_COLLATION_PROTOCOL  *Uci;

  LangCode   = Buffer;
  BufferSize = sizeof (Buffer);
  Status = gRT->GetVariable (
                  UnicodeInterface->VariableName,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &BufferSize,
                  Buffer
                  );
  if (EFI_ERROR (Status)) {
    LangCode = UnicodeInterface->DefaultLangCode;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  UnicodeInterface->UnicodeProtocolGuid,
                  NULL,
                  &NoHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    //
    // Open Unicode Collation Protocol
    //
    Status = gBS->OpenProtocol (
                    Handles[Index],
                    UnicodeInterface->UnicodeProtocolGuid,
                    (VOID **) &Uci,
                    AgentHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (UnicodeInterface->SearchLangCode (Uci->SupportedLanguages, LangCode)) {
      mUnicodeCollationInterface = Uci;
      break;
    }
  }

  FreePool (Handles);

  return (mUnicodeCollationInterface != NULL)? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Initialize Unicode Collation support.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.

**/
EFI_STATUS
InitializeUnicodeCollationSupport (
  IN EFI_HANDLE    AgentHandle
  )
{

  EFI_STATUS       Status;

  Status = EFI_UNSUPPORTED;
  if (FeaturePcdGet (PcdUnicodeCollation2Support)) {
    Status = InitializeUnicodeCollationSupportWithConfig (AgentHandle, &mRfc3066Lang);
  }

  if (FeaturePcdGet (PcdUnicodeCollationSupport) && EFI_ERROR (Status)) {
    Status = InitializeUnicodeCollationSupportWithConfig (AgentHandle, &mIso639Lang);
  }

  return Status;
}

/**
  Performs a case-insensitive comparison of two Null-terminated Unicode strings.

  @param  S1                   A pointer to a Null-terminated Unicode string.
  @param  S2                   A pointer to a Null-terminated Unicode string.

  @retval 0                    S1 is equivalent to S2.
  @retval >0                   S1 is lexically greater than S2.
  @retval <0                   S1 is lexically less than S2.
**/
INTN
FatStriCmp (
  IN CHAR16       *S1,
  IN CHAR16       *S2
  )
{
  ASSERT (StrSize (S1) != 0);
  ASSERT (StrSize (S2) != 0);
  ASSERT (mUnicodeCollationInterface != NULL);

  return mUnicodeCollationInterface->StriColl (
                                       mUnicodeCollationInterface,
                                       S1,
                                       S2
                                       );
}


/**
  Uppercase a string.

  @param  Str                   The string which will be upper-cased.

  @return None.

**/
VOID
FatStrUpr (
  IN OUT CHAR16   *String
  )
{
  ASSERT (StrSize (String) != 0);
  ASSERT (mUnicodeCollationInterface != NULL);

  mUnicodeCollationInterface->StrUpr (mUnicodeCollationInterface, String);
}


/**
  Lowercase a string

  @param  Str                   The string which will be lower-cased.

  @return None

**/
VOID
FatStrLwr (
  IN OUT CHAR16   *String
  )
{
  ASSERT (StrSize (String) != 0);
  ASSERT (mUnicodeCollationInterface != NULL);

  mUnicodeCollationInterface->StrLwr (mUnicodeCollationInterface, String);
}


/**
  Convert FAT string to unicode string.

  @param  FatSize               The size of FAT string.
  @param  Fat                   The FAT string.
  @param  String                The unicode string.

  @return None.

**/
VOID
FatFatToStr (
  IN  UINTN                            FatSize,
  IN  CHAR8                            *Fat,
  OUT CHAR16                           *String
  )
{
  ASSERT (Fat != NULL);
  ASSERT (String != NULL);
  ASSERT (((UINTN) String & 0x01) != 0);
  ASSERT (mUnicodeCollationInterface != NULL);

  mUnicodeCollationInterface->FatToStr (mUnicodeCollationInterface, FatSize, Fat, String);
}


/**
  Convert unicode string to Fat string.

  @param  String                The unicode string.
  @param  FatSize               The size of the FAT string.
  @param  Fat                   The FAT string.

  @retval TRUE                  Convert successfully.
  @retval FALSE                 Convert error.

**/
BOOLEAN
FatStrToFat (
  IN  CHAR16                          *String,
  IN  UINTN                           FatSize,
  OUT CHAR8                           *Fat
  )
{
  ASSERT (Fat != NULL);
  ASSERT (StrSize (String) != 0);
  ASSERT (mUnicodeCollationInterface != NULL);

  return mUnicodeCollationInterface->StrToFat (
                                       mUnicodeCollationInterface,
                                       String,
                                       FatSize,
                                       Fat
                                       );
}
