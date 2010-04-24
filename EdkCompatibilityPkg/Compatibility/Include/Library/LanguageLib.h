/** @file
  Provides functions for language conversion between ISO 639-2 and RFC 4646 styles.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LANGUAGE_LIB__
#define __LANGUAGE_LIB__

/**
  Converts an ISO 639-2 language code to an RFC 4646 language code.
  If the ISO 639-2 language code has a corresponding ISO 639-1 code, then that ISO 639-1
  code is returned in the out parameter. Else the original ISO 639-2 code is returned. The returned RFC 4646
  language code is composed of only a primary language subtag.

  If Iso639Language is NULL, then ASSERT().
  If Rfc4646Language is NULL, then ASSERT().

  @param[out] Rfc4646Language  Pointers to a buffer large enough for an ASCII string
                               representing an RFC 4646 language code containing only
                               either a ISO 639-1 or ISO 639-2 primary language subtag.
                               This string is Null-terminated.
  @param[in]  Iso639Language   The pointer to a 3-letter ASCII string that represents
                               an ISO 639-2 language code. This string is not required
                               to be Null-terminated.

  @retval TRUE                 The ISO 639-2 language code was converted to an ISO 639-1 code.
  @retval FALSE                The language code does not have a corresponding ISO 639-1 code.

**/
BOOLEAN
EFIAPI
ConvertIso639ToRfc4646 (
  OUT CHAR8        *Rfc4646Language,
  IN  CONST CHAR8  *Iso639Language
  );

/**
  Converts an RFC 4646 language code to an ISO 639-2 language code. The primary language
  subtag of the RFC 4646 code must be either an ISO 639-1 or 639-2 code. If the primary
  language subtag is an ISO 639-1 code, then it is converted to its corresponding ISO 639-2
  code (T code if applies). Else the ISO 639-2 code is returned.

  If Rfc4646Language is NULL, then ASSERT().
  If Iso639Language is NULL, then ASSERT().

  @param[out] Iso639Language   Pointers to a buffer large enough for a 3-letter ASCII string
                               representing an ISO 639-2 language code. The string 
                               is Null-terminated.
  @param[in]  Rfc4646Language  The pointer to a RFC 4646 language code string. 
                               This string is terminated
                               by a NULL or a ';' character.

  @retval TRUE                 Language code converted successfully.
  @retval FALSE                The RFC 4646 language code is invalid or unsupported.

**/
BOOLEAN
EFIAPI
ConvertRfc4646ToIso639 (
  OUT CHAR8        *Iso639Language,
  IN  CONST CHAR8  *Rfc4646Language
  );

/**
  Converts ISO 639-2 language codes to RFC 4646 codes and returns the converted codes.
  Caller is responsible for freeing the allocated buffer.

  If Iso639Languages is NULL, then ASSERT.

  @param[in] Iso639Languages  Pointers to Null-terminated ISO 639-2 language code strings containing
                              one or more ISO 639-2 3-letter language codes.
  
  @retval NULL                Invalid ISO 639-2 language code found.
  @retval NULL                Out of memory.
  @return                     The pointer to the allocate buffer containing the 
                              Null-terminated converted language codes string.
                              This string is composed of one or more RFC4646 
                              language codes each of which has only
                              ISO 639-1 2-letter primary language subtag.

**/
CHAR8 *
EFIAPI
ConvertLanguagesIso639ToRfc4646 (
  IN CONST CHAR8  *Iso639Languages
  );

/**
  Converts RFC 4646 language codes to ISO 639-2 codes and returns the converted codes.
  The primary language subtag of the RFC 4646 code must be either an ISO 639-1 or 639-2 code.
  Caller is responsible for freeing the allocated buffer.

  If Rfc4646Languages is NULL, then ASSERT.

  @param[in] Rfc4646Languages  Pointers to a Null-terminated RFC 4646 language codes 
                               string containing one or more RFC 4646 language codes.
  
  @retval NULL                 Invalid or unsupported RFC 4646 language code found.
  @retval NULL                 Out of memory.
  @return                      The pointer to the allocate buffer containing the 
                               Null-terminated converted language codes string.
                               This string is composed of one or more ISO 639-2 
                               language codes.

**/
CHAR8 *
EFIAPI
ConvertLanguagesRfc4646ToIso639 (
  IN CONST CHAR8  *Rfc4646Languages
  );


#endif
