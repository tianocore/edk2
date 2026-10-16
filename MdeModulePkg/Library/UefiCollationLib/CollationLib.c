/** @file
  Unicode Collation library.

  Copyright 2014 Insyde Software Corp. All Rights Reserved.

**/

#include "InternalCollationLib.h"


///
/// Pointer to the UEFI Unicode Collation 2 Protocol
///
EFI_UNICODE_COLLATION_PROTOCOL  *gUnicodeCollation = NULL;


/**
  The constructor function retrieves pointers to the UEFI Unicode Collation protocol
  instances

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UefiCollationLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Retrieve the pointer to the UEFI HII Database Protocol
  //
  return gBS->LocateProtocol (&gEfiUnicodeCollation2ProtocolGuid, NULL, (VOID **) &gUnicodeCollation);
}

/**
   Check if strings are equal using Unicode Collation.

   @param[in] Str1                The first null-terminated string.
   @param[in] Str2                The second null-terminated string.

   @retval 0                      Strings are equal.
   @retval 1                      Str1 is lexically greater than Str2
   @retval -1                     Str1 is lexically less than Str2
**/
INT32
CollationStriColl (
   IN CONST CHAR16 *Str1,
   IN CONST CHAR16 *Str2
   )
 {
  return (INT32) gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16 *)Str1, (CHAR16 *)Str2);
 }

 /**
    Check whether the string matches the pattern, using Unicode Collation

   @param[in] Str                 The string.
   @param[in] Pattern             The pattern.

   @retval    TRUE                Match
   @retval    FALSE               No match.
**/
BOOLEAN
CollationMetaiMatch(
  IN CONST CHAR16 *Str,
  IN CONST CHAR16 *Pattern
  )
{
  return gUnicodeCollation->MetaiMatch(gUnicodeCollation, (CHAR16 *)Str, (CHAR16 *)Pattern);
}

/**
    Convert a string to all lower case characters.

   @param[in,out]   Str                 The string.

**/
VOID
CollationStrLwr(
  IN OUT CHAR16 *Str
  )
{
  gUnicodeCollation->StrLwr(gUnicodeCollation, Str);
}

/**
    Convert a string to all upper case characters.

   @param[in,out]   Str                 The string.

**/
VOID
CollationStrUpr(
  IN OUT CHAR16 *Str
  )
{
  gUnicodeCollation->StrUpr(gUnicodeCollation, Str);
}
