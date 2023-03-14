/** @file
  SMBIOS String Table Helper

  Copyright (c) 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - DSP0134 - SMBIOS Specification Version 3.6.0, 2022-06-17
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

/** Add a string to the string table.

  @param[in]   StrTable  Pointer to the string table
  @param[in]   Str       Pointer to the string
  @param[out]  StrRef    Optional pointer to retrieve the string field
                         reference of the string in the string table

  @return EFI_SUCCESS            Success
  @return EFI_INVALID_PARAMETER  Invalid string table pointer
  @return EFI_BUFFER_TOO_SMALL   Insufficient space to add string
**/
EFI_STATUS
EFIAPI
StringTableAddString (
  IN        STRING_TABLE *CONST  StrTable,
  IN  CONST CHAR8                *Str,
  OUT       UINT8                *StrRef      OPTIONAL
  )
{
  UINTN           StrLength;
  STRING_ELEMENT  *StrElement;

  if ((StrTable == NULL) || (Str == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (StrTable->StrCount >= StrTable->MaxStringElements) {
    return EFI_BUFFER_TOO_SMALL;
  }

  StrLength = AsciiStrLen (Str);
  if (StrLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  // Update the string element
  StrElement            = &StrTable->Elements[StrTable->StrCount];
  StrElement->StringLen = StrLength;
  StrElement->String    = Str;

  // Update String table information
  StrTable->TotalStrLen += StrLength;
  StrTable->StrCount++;

  // Return the index of the string in the string table if requested
  if (StrRef != NULL) {
    // Note: SMBIOS string field references start at 1. So, return the
    // StrCount as the string reference after it is updated.
    *StrRef = StrTable->StrCount;
  }

  return EFI_SUCCESS;
}

/** Returns the total size required to publish the strings to the SMBIOS
    string area.

  @param[in] StrTable              Pointer to the string table

  @return Total size required to publish the strings in the SMBIOS string area.
**/
UINTN
EFIAPI
StringTableGetStringSetSize (
  IN  STRING_TABLE *CONST  StrTable
  )
{
  if (StrTable == NULL) {
    ASSERT (0);
    return 0;
  }

  // See Section 6.1.3 Text strings, SMBIOS Specification Version 3.6.0
  // - If the formatted portion of the structure contains string-reference
  //   fields and all the string fields are set to 0 (no string references),
  //   the formatted section of the structure is followed by two null (00h)
  //   BYTES.
  // - Each string is terminated with a null (00h) BYTE
  // - and the set of strings is terminated with an additional null (00h) BYTE.

  // Therefore, if string count = 0, return 2
  // if string count > 0, the string set size =
  // StrTable->TotalStrLen (total length of the strings in the string table)
  // + StrTable->StrCount (add string count to include '\0' for each string)
  // +1 (an additional '\0' is required at the end of the string set).
  return (StrTable->StrCount == 0) ? 2 :
         (StrTable->TotalStrLen + StrTable->StrCount + 1);
}

/** Iterate through the string table and publish the strings in the SMBIOS
    string area.

  @param[in] StrTable              Pointer to the string table
  @param[in] SmbiosStringAreaStart Start address of the SMBIOS string area.
  @param[in] SmbiosStringAreaSize  Size of the SMBIOS string area.

  @return EFI_SUCCESS            Success
  @return EFI_INVALID_PARAMETER  Invalid string table pointer
  @return EFI_BUFFER_TOO_SMALL   Insufficient space to publish strings
**/
EFI_STATUS
EFIAPI
StringTablePublishStringSet (
  IN        STRING_TABLE  *CONST  StrTable,
  IN        CHAR8         *CONST  SmbiosStringAreaStart,
  IN  CONST UINTN                 SmbiosStringAreaSize
  )
{
  UINT8           Index;
  STRING_ELEMENT  *StrElement;
  CHAR8           *SmbiosString;
  UINTN           BytesRemaining;
  UINTN           BytesCopied;

  if ((StrTable == NULL) || (SmbiosStringAreaStart == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (SmbiosStringAreaSize < StringTableGetStringSetSize (StrTable)) {
    return EFI_BUFFER_TOO_SMALL;
  }

  SmbiosString   = SmbiosStringAreaStart;
  BytesRemaining = SmbiosStringAreaSize;

  if (StrTable->StrCount == 0) {
    // See Section 6.1.3 Text strings, SMBIOS Specification Version 3.6.0
    // If the formatted portion of the structure contains string-reference
    // fields and all the string fields are set to 0 (no string references),
    // the formatted section of the structure is followed by two null (00h)
    // BYTES.
    *SmbiosString++ = '\0';
  } else {
    for (Index = 0; Index < StrTable->StrCount; Index++) {
      StrElement = &StrTable->Elements[Index];
      AsciiStrCpyS (SmbiosString, BytesRemaining, StrElement->String);

      // See Section 6.1.3 Text strings, SMBIOS Specification Version 3.6.0
      // - Each string is terminated with a null (00h) BYTE
      // Bytes Copied = String length + 1 for the string NULL terminator.
      BytesCopied     = StrElement->StringLen + 1;
      BytesRemaining -= BytesCopied;
      SmbiosString   += BytesCopied;
    }
  }

  // See Section 6.1.3 Text strings, SMBIOS Specification Version 3.6.0
  // - the set of strings is terminated with an additional null (00h) BYTE.
  *SmbiosString = '\0';
  return EFI_SUCCESS;
}

/** Initialise the string table and allocate memory for the string elements.

  @param[in] StrTable           Pointer to the string table
  @param[in] MaxStringElements  Maximum number of strings that the string
                                table can hold.

  @return EFI_SUCCESS            Success
  @return EFI_INVALID_PARAMETER  Invalid string table pointer
  @return EFI_OUT_OF_RESOURCES   Failed to allocate memory for string elements
**/
EFI_STATUS
EFIAPI
StringTableInitialize (
  IN STRING_TABLE *CONST  StrTable,
  IN UINTN                MaxStringElements
  )
{
  STRING_ELEMENT  *Elements;

  if ((StrTable == NULL) || (MaxStringElements > MAX_UINT8)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (StrTable, sizeof (STRING_TABLE));

  Elements = (STRING_ELEMENT *)AllocateZeroPool (
                                 sizeof (STRING_ELEMENT) * MaxStringElements
                                 );
  if (Elements == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StrTable->Elements          = Elements;
  StrTable->MaxStringElements = (UINT8)MaxStringElements;
  return EFI_SUCCESS;
}

/** Free memory allocated for the string elements in the string table.

  @param[in] StrTable           Pointer to the string table

  @return EFI_SUCCESS            Success
  @return EFI_INVALID_PARAMETER  Invalid string table pointer or string elements
**/
EFI_STATUS
EFIAPI
StringTableFree (
  IN STRING_TABLE *CONST  StrTable
  )
{
  if ((StrTable == NULL) || (StrTable->Elements == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FreePool (StrTable->Elements);
  ZeroMem (StrTable, sizeof (STRING_TABLE));
  return EFI_SUCCESS;
}
