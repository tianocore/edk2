/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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
  //
  // Should be at EFI_D_INFO, but lets us know things are running
  //
  DEBUG ((EFI_D_INFO, "EfiInitializeDriverLib: Started\n"));

  return EFI_SUCCESS;
}

BOOLEAN
EfiLibCompareLanguage (
  IN  CHAR8  *Language1,
  IN  CHAR8  *Language2
  )
/*++

Routine Description:

  Compare whether two names of languages are identical.

Arguments:

  Language1 - Name of language 1
  Language2 - Name of language 2

Returns:

  TRUE      - same
  FALSE     - not same

--*/
{
  UINTN Index;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
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
#else
  for (Index = 0; Index < 3; Index++) {
    if (Language1[Index] != Language2[Index]) {
      return FALSE;
    }
  }

  return TRUE;
#endif
}

STATIC
CHAR8 *
NextSupportedLanguage (
  IN CHAR8                *Languages
  )
{
#ifdef LANGUAGE_RFC_3066   // LANGUAGE_RFC_3066
  for (; (*Languages != 0) && (*Languages != ';'); Languages++)
    ;

  if (*Languages == ';') {
    Languages++;
  }

  return Languages;
#else                      // LANGUAGE_ISO_639_2
  return (Languages + 3);
#endif
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

    SupportedLanguages = NextSupportedLanguage(SupportedLanguages);
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
      NewUnicodeStringTable[NumberOfEntries].Language = EfiLibAllocateCopyPool (EfiAsciiStrLen(Language) + 1, Language);
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

    SupportedLanguages = NextSupportedLanguage(SupportedLanguages);
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
