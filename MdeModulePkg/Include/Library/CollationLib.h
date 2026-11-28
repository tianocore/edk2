/** @file
  Provides interface to Unicode Collation protocol functions.

  Copyright 2014 Insyde Software Corp. All Rights Reserved.
**/

#pragma once

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
  );

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
   );

/**
 *  Convert a string to all lower case using Unicode Collation.
 *
 *  @param[in,out]  Str           The null-terminated string.
 *
 */
 VOID
 CollationStrLwr(
   IN OUT CHAR16 *Str
   );

/**
 *  Convert a string to all upper case using Unicode Collation.
 *
 *  @param[in,out]  Str           The null-terminated string.
 *
 */
 VOID
 CollationStrUpr(
   IN OUT CHAR16 *Str
   );


