/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiDriverLib.c

Abstract:

  Light weight lib to support EFI drivers.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

VOID
EFIAPI
OnStatusCodeInstall (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

#endif

//
// Global Interface for Debug Mask Protocol
//
EFI_DEBUG_MASK_PROTOCOL *gDebugMaskInterface = NULL;

EFI_STATUS
EfiInitializeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - Standard EFI Image entry parameter
  
  SystemTable     - Standard EFI Image entry parameter

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
{
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  VOID *Registration;
#endif
  
  gST = SystemTable;

  ASSERT (gST != NULL);

  gBS = gST->BootServices;
  gRT = gST->RuntimeServices;

  ASSERT (gBS != NULL);
  ASSERT (gRT != NULL);

  //
  // Get driver debug mask protocol interface
  //
#ifdef EFI_DEBUG
  gBS->HandleProtocol (
        ImageHandle,
        &gEfiDebugMaskProtocolGuid,
        (VOID *) &gDebugMaskInterface
        );
#endif

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  //
  // Register EFI_STATUS_CODE_PROTOCOL notify function
  //
  EfiLibCreateProtocolNotifyEvent (
    &gEfiStatusCodeRuntimeProtocolGuid,
    EFI_TPL_CALLBACK,
    OnStatusCodeInstall,
    NULL,
    &Registration
    );

#endif

  //
  // Should be at EFI_D_INFO, but lets us know things are running
  //
  DEBUG ((EFI_D_INFO, "EfiInitializeDriverLib: Started\n"));

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
IsIso639LanguageCode (
  IN CHAR8                *Languages
  )
/*++

Routine Description:

  Tests whether a language code has format of ISO639-2.

Arguments:

  Languages - The language code to be tested.

Returns:

  TRUE      - Language code format is ISO 639-2.
  FALSE     - Language code format is not ISO 639-2.

--*/
{
  UINTN  Index;

  //
  // Find out format of Languages
  //
  for (Index = 0; Languages[Index] != 0 && Languages[Index] != ';' && Languages[Index] != '-'; Index++);
  if (Languages[Index] != 0) {
    //
    // RFC4646 language code
    //
    return FALSE;
  }

  //
  // No ';' and '-', it's either ISO639-2 code (list) or single RFC4646 code
  //
  if (Index == 2) {
    //
    // Single RFC4646 language code without country code, e.g. "en"
    //
    return FALSE;
  }

  //
  // Languages in format of ISO639-2
  //
  return TRUE;
}

BOOLEAN
EfiLibCompareLanguage (
  IN  CHAR8               *Language1,
  IN  CHAR8               *Language2
  )
/*++

Routine Description:

  Compare the first language instance of two language codes, either could be a
  single language code or a language code list. This function assume Language1
  and Language2 has the same language code format, i.e. either ISO639-2 or RFC4646.

Arguments:

  Language1 - The first language code to be tested.
  Language2 - The second language code to be tested.

Returns:

  TRUE      - Language code match.
  FALSE     - Language code mismatch.

--*/
{
  UINTN Index;

  //
  // Compare first two bytes of language tag
  //
  if ((Language1[0] != Language2[0]) || (Language1[1] != Language2[1])) {
    return FALSE;
  }

  if (IsIso639LanguageCode (Language1)) {
    //
    // ISO639-2 language code, compare the third byte of language tag
    //
    return (BOOLEAN) ((Language1[2] == Language2[2]) ? TRUE : FALSE);
  }

  //
  // RFC4646 language code
  //
  for (Index = 0; Language1[Index] != 0 && Language1[Index] != ';'; Index++);
  if ((EfiAsciiStrnCmp (Language1, Language2, Index) == 0) && (Language2[Index] == 0 || Language2[Index] == ';')) {
    return TRUE;
  }

  return FALSE;
}

STATIC
CHAR8 *
NextSupportedLanguage (
  IN CHAR8                *Languages
  )
/*++

Routine Description:

  Step to next language code of a language code list.

Arguments:

  Languages - The language code list to traverse.

Returns:

  Pointer to next language code or NULL terminator if it's the last one.

--*/
{
  UINTN    Index;

  if (IsIso639LanguageCode (Languages)) {
    //
    // ISO639-2 language code
    //
    return (Languages + 3);
  }

  //
  // Search in RFC4646 language code list
  //
  for (Index = 0; Languages[Index] != 0 && Languages[Index] != ';'; Index++);
  if (Languages[Index] == ';') {
    Index++;
  }
  return (Languages + Index);
}

EFI_STATUS
EfiLibLookupUnicodeString (
  IN  CHAR8                     *Language,
  IN  CHAR8                     *SupportedLanguages,
  IN  EFI_UNICODE_STRING_TABLE  *UnicodeStringTable,
  OUT CHAR16                    **UnicodeString
  )
/*++

Routine Description:

  Translate a unicode string to a specified language if supported.
  
Arguments:

  Language              - The name of language to translate to
  SupportedLanguages    - Supported languages set
  UnicodeStringTable    - Pointer of one item in translation dictionary
  UnicodeString         - The translated string

Returns: 

  EFI_INVALID_PARAMETER - Invalid parameter
  EFI_UNSUPPORTED       - System not supported this language or this string translation
  EFI_SUCCESS           - String successfully translated

--*/
{
  //
  // Make sure the parameters are valid
  //
  if (Language == NULL || UnicodeString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If there are no supported languages, or the Unicode String Table is empty, then the
  // Unicode String specified by Language is not supported by this Unicode String Table
  //
  if (SupportedLanguages == NULL || UnicodeStringTable == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Make sure Language is in the set of Supported Languages
  //
  while (*SupportedLanguages != 0) {
    if (EfiLibCompareLanguage (Language, SupportedLanguages)) {

      //
      // Search the Unicode String Table for the matching Language specifier
      //
      while (UnicodeStringTable->Language != NULL) {
        if (EfiLibCompareLanguage (Language, UnicodeStringTable->Language)) {

          //
          // A matching string was found, so return it
          //
          *UnicodeString = UnicodeStringTable->UnicodeString;
          return EFI_SUCCESS;
        }

        UnicodeStringTable++;
      }

      return EFI_UNSUPPORTED;
    }

    SupportedLanguages = NextSupportedLanguage (SupportedLanguages);
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EfiLibAddUnicodeString (
  IN      CHAR8                     *Language,
  IN      CHAR8                     *SupportedLanguages,
  IN OUT  EFI_UNICODE_STRING_TABLE  **UnicodeStringTable,
  IN      CHAR16                    *UnicodeString
  )
/*++

Routine Description:

  Add an translation to the dictionary if this language if supported.
  
Arguments:

  Language              - The name of language to translate to
  SupportedLanguages    - Supported languages set
  UnicodeStringTable    - Translation dictionary
  UnicodeString         - The corresponding string for the language to be translated to

Returns: 

  EFI_INVALID_PARAMETER - Invalid parameter
  EFI_UNSUPPORTED       - System not supported this language
  EFI_ALREADY_STARTED   - Already has a translation item of this language
  EFI_OUT_OF_RESOURCES  - No enough buffer to be allocated
  EFI_SUCCESS           - String successfully translated

--*/
{
  UINTN                     NumberOfEntries;
  EFI_UNICODE_STRING_TABLE  *OldUnicodeStringTable;
  EFI_UNICODE_STRING_TABLE  *NewUnicodeStringTable;
  UINTN                     UnicodeStringLength;

  //
  // Make sure the parameter are valid
  //
  if (Language == NULL || UnicodeString == NULL || UnicodeStringTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If there are no supported languages, then a Unicode String can not be added
  //
  if (SupportedLanguages == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // If the Unicode String is empty, then a Unicode String can not be added
  //
  if (UnicodeString[0] == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make sure Language is a member of SupportedLanguages
  //
  while (*SupportedLanguages != 0) {
    if (EfiLibCompareLanguage (Language, SupportedLanguages)) {

      //
      // Determine the size of the Unicode String Table by looking for a NULL Language entry
      //
      NumberOfEntries = 0;
      if (*UnicodeStringTable != NULL) {
        OldUnicodeStringTable = *UnicodeStringTable;
        while (OldUnicodeStringTable->Language != NULL) {
          if (EfiLibCompareLanguage (Language, OldUnicodeStringTable->Language)) {
            return EFI_ALREADY_STARTED;
          }

          OldUnicodeStringTable++;
          NumberOfEntries++;
        }
      }

      //
      // Allocate space for a new Unicode String Table.  It must hold the current number of
      // entries, plus 1 entry for the new Unicode String, plus 1 entry for the end of table
      // marker
      //
      NewUnicodeStringTable = EfiLibAllocatePool ((NumberOfEntries + 2) * sizeof (EFI_UNICODE_STRING_TABLE));
      if (NewUnicodeStringTable == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // If the current Unicode String Table contains any entries, then copy them to the
      // newly allocated Unicode String Table.
      //
      if (*UnicodeStringTable != NULL) {
        EfiCopyMem (
          NewUnicodeStringTable,
          *UnicodeStringTable,
          NumberOfEntries * sizeof (EFI_UNICODE_STRING_TABLE)
          );
      }

      //
      // Allocate space for a copy of the Language specifier
      //
      NewUnicodeStringTable[NumberOfEntries].Language = EfiLibAllocateCopyPool (EfiAsciiStrSize (Language), Language);
      if (NewUnicodeStringTable[NumberOfEntries].Language == NULL) {
        gBS->FreePool (NewUnicodeStringTable);
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Compute the length of the Unicode String
      //
      for (UnicodeStringLength = 0; UnicodeString[UnicodeStringLength] != 0; UnicodeStringLength++)
        ;

      //
      // Allocate space for a copy of the Unicode String
      //
      NewUnicodeStringTable[NumberOfEntries].UnicodeString = EfiLibAllocateCopyPool (
                                                              (UnicodeStringLength + 1) * sizeof (CHAR16),
                                                              UnicodeString
                                                              );
      if (NewUnicodeStringTable[NumberOfEntries].UnicodeString == NULL) {
        gBS->FreePool (NewUnicodeStringTable[NumberOfEntries].Language);
        gBS->FreePool (NewUnicodeStringTable);
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Mark the end of the Unicode String Table
      //
      NewUnicodeStringTable[NumberOfEntries + 1].Language       = NULL;
      NewUnicodeStringTable[NumberOfEntries + 1].UnicodeString  = NULL;

      //
      // Free the old Unicode String Table
      //
      if (*UnicodeStringTable != NULL) {
        gBS->FreePool (*UnicodeStringTable);
      }

      //
      // Point UnicodeStringTable at the newly allocated Unicode String Table
      //
      *UnicodeStringTable = NewUnicodeStringTable;

      return EFI_SUCCESS;
    }

    SupportedLanguages = NextSupportedLanguage (SupportedLanguages);
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EfiLibFreeUnicodeStringTable (
  IN OUT  EFI_UNICODE_STRING_TABLE  *UnicodeStringTable
  )
/*++

Routine Description:

  Free a string table.

Arguments:

  UnicodeStringTable      - The string table to be freed.

Returns: 

  EFI_SUCCESS       - The table successfully freed.

--*/
{
  UINTN Index;

  //
  // If the Unicode String Table is NULL, then it is already freed
  //
  if (UnicodeStringTable == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Loop through the Unicode String Table until we reach the end of table marker
  //
  for (Index = 0; UnicodeStringTable[Index].Language != NULL; Index++) {

    //
    // Free the Language string from the Unicode String Table
    //
    gBS->FreePool (UnicodeStringTable[Index].Language);

    //
    // Free the Unicode String from the Unicode String Table
    //
    if (UnicodeStringTable[Index].UnicodeString != NULL) {
      gBS->FreePool (UnicodeStringTable[Index].UnicodeString);
    }
  }

  //
  // Free the Unicode String Table itself
  //
  gBS->FreePool (UnicodeStringTable);

  return EFI_SUCCESS;
}
