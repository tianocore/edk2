/** @file
  Unicode Collation Support component that hides the trivial difference of Unicode Collation
  and Unicode collation 2 Protocol.

  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollationInterface = NULL;

/**
  Worker function to initialize Unicode Collation support.

  It tries to locate Unicode Collation (2) protocol and matches it with current
  platform language code.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.
  @param  ProtocolGuid         The pointer to Unicode Collation (2) protocol GUID.
  @param  VariableName         The name of the RFC 4646 or ISO 639-2 language variable.
  @param  DefaultLanguage      The default language in case the RFC 4646 or ISO 639-2 language is absent.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.

**/
EFI_STATUS
InitializeUnicodeCollationSupportWorker (
  IN EFI_HANDLE    AgentHandle,
  IN EFI_GUID      *ProtocolGuid,
  IN CONST CHAR16  *VariableName,
  IN CONST CHAR8   *DefaultLanguage
  )
{
  EFI_STATUS                      ReturnStatus;
  EFI_STATUS                      Status;
  UINTN                           NumHandles;
  UINTN                           Index;
  EFI_HANDLE                      *Handles;
  EFI_UNICODE_COLLATION_PROTOCOL  *Uci;
  BOOLEAN                         Iso639Language;
  CHAR8                           *Language;
  CHAR8                           *BestLanguage;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolGuid,
                  NULL,
                  &NumHandles,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Iso639Language = (BOOLEAN)(ProtocolGuid == &gEfiUnicodeCollationProtocolGuid);
  GetEfiGlobalVariable2 (VariableName, (VOID **)&Language, NULL);

  ReturnStatus = EFI_UNSUPPORTED;
  for (Index = 0; Index < NumHandles; Index++) {
    //
    // Open Unicode Collation Protocol
    //
    Status = gBS->OpenProtocol (
                    Handles[Index],
                    ProtocolGuid,
                    (VOID **)&Uci,
                    AgentHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Find the best matching matching language from the supported languages
    // of Unicode Collation (2) protocol.
    //
    BestLanguage = GetBestLanguage (
                     Uci->SupportedLanguages,
                     Iso639Language,
                     (Language == NULL) ? "" : Language,
                     DefaultLanguage,
                     NULL
                     );
    if (BestLanguage != NULL) {
      FreePool (BestLanguage);
      mUnicodeCollationInterface = Uci;
      ReturnStatus               = EFI_SUCCESS;
      break;
    }
  }

  if (Language != NULL) {
    FreePool (Language);
  }

  FreePool (Handles);

  return ReturnStatus;
}

/**
  Initialize Unicode Collation support.

  It tries to locate Unicode Collation 2 protocol and matches it with current
  platform language code. If for any reason the first attempt fails, it then tries to
  use Unicode Collation Protocol.

  @param  AgentHandle          The handle used to open Unicode Collation (2) protocol.

  @retval EFI_SUCCESS          The Unicode Collation (2) protocol has been successfully located.
  @retval Others               The Unicode Collation (2) protocol has not been located.

**/
EFI_STATUS
InitializeUnicodeCollationSupport (
  IN EFI_HANDLE  AgentHandle
  )
{
  EFI_STATUS  Status;

  Status = EFI_UNSUPPORTED;

  //
  // First try to use RFC 4646 Unicode Collation 2 Protocol.
  //
  Status = InitializeUnicodeCollationSupportWorker (
             AgentHandle,
             &gEfiUnicodeCollation2ProtocolGuid,
             L"PlatformLang",
             (CONST CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLang)
             );
  //
  // If the attempt to use Unicode Collation 2 Protocol fails, then we fall back
  // on the ISO 639-2 Unicode Collation Protocol.
  //
  if (EFI_ERROR (Status)) {
    Status = InitializeUnicodeCollationSupportWorker (
               AgentHandle,
               &gEfiUnicodeCollationProtocolGuid,
               L"Lang",
               (CONST CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLang)
               );
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
  IN CHAR16  *S1,
  IN CHAR16  *S2
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

  @param  String                   The string which will be upper-cased.


**/
VOID
FatStrUpr (
  IN OUT CHAR16  *String
  )
{
  ASSERT (StrSize (String) != 0);
  ASSERT (mUnicodeCollationInterface != NULL);

  mUnicodeCollationInterface->StrUpr (mUnicodeCollationInterface, String);
}

/**
  Lowercase a string

  @param  String                   The string which will be lower-cased.


**/
VOID
FatStrLwr (
  IN OUT CHAR16  *String
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
  IN  UINTN   FatSize,
  IN  CHAR8   *Fat,
  OUT CHAR16  *String
  )
{
  ASSERT (Fat != NULL);
  ASSERT (String != NULL);
  ASSERT (((UINTN)String & 0x01) == 0);
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
  IN  CHAR16  *String,
  IN  UINTN   FatSize,
  OUT CHAR8   *Fat
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
