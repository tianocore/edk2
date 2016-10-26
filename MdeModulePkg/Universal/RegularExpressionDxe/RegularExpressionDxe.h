/** @file
  EFI_REGULAR_EXPRESSION_PROTOCOL Header File.

  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __REGULAR_EXPRESSIONDXE_H__
#define __REGULAR_EXPRESSIONDXE_H__

#include "Oniguruma/oniguruma.h"

#include <Uefi.h>
#include <Protocol/RegularExpressionProtocol.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

/**
  Checks if the input string matches to the regular expression pattern.

  @param This          A pointer to the EFI_REGULAR_EXPRESSION_PROTOCOL instance.
                       Type EFI_REGULAR_EXPRESSION_PROTOCOL is defined in Section
                       XYZ.

  @param String        A pointer to a NULL terminated string to match against the
                       regular expression string specified by Pattern.

  @param Pattern       A pointer to a NULL terminated string that represents the
                       regular expression.

  @param SyntaxType    A pointer to the EFI_REGEX_SYNTAX_TYPE that identifies the
                       regular expression syntax type to use. May be NULL in which
                       case the function will use its default regular expression
                       syntax type.

  @param Result        On return, points to TRUE if String fully matches against
                       the regular expression Pattern using the regular expression
                       SyntaxType. Otherwise, points to FALSE.

  @param Captures      A Pointer to an array of EFI_REGEX_CAPTURE objects to receive
                       the captured groups in the event of a match. The full
                       sub-string match is put in Captures[0], and the results of N
                       capturing groups are put in Captures[1:N]. If Captures is
                       NULL, then this function doesn't allocate the memory for the
                       array and does not build up the elements. It only returns the
                       number of matching patterns in CapturesCount. If Captures is
                       not NULL, this function returns a pointer to an array and
                       builds up the elements in the array. CapturesCount is also
                       updated to the number of matching patterns found. It is the
                       caller's responsibility to free the memory pool in Captures
                       and in each CapturePtr in the array elements.

  @param CapturesCount On output, CapturesCount is the number of matching patterns
                found in String. Zero means no matching patterns were found
                in the string.

  @retval EFI_SUCCESS            The regular expression string matching
                                 completed successfully.
  @retval EFI_UNSUPPORTED        The regular expression syntax specified by
                                 SyntaxType is not supported by this driver.
  @retval EFI_DEVICE_ERROR       The regular expression string matching
                                 failed due to a hardware or firmware error.
  @retval EFI_INVALID_PARAMETER  String, Pattern, Result, or CapturesCountis
                                 NULL.

**/
EFI_STATUS
EFIAPI
RegularExpressionMatch (
  IN  EFI_REGULAR_EXPRESSION_PROTOCOL *This,
  IN  CHAR16                          *String,
  IN  CHAR16                          *Pattern,
  IN  EFI_REGEX_SYNTAX_TYPE           *SyntaxType, OPTIONAL
  OUT BOOLEAN                         *Result,
  OUT EFI_REGEX_CAPTURE               **Captures, OPTIONAL
  OUT UINTN                           *CapturesCount
  );

/**
  Returns information about the regular expression syntax types supported
  by the implementation.

  @param This                     A pointer to the EFI_REGULAR_EXPRESSION_PROTOCOL
                                  instance.

  @param RegExSyntaxTypeListSize  On input, the size in bytes of RegExSyntaxTypeList.
                                  On output with a return code of EFI_SUCCESS, the
                                  size in bytes of the data returned in
                                  RegExSyntaxTypeList. On output with a return code
                                  of EFI_BUFFER_TOO_SMALL, the size of
                                  RegExSyntaxTypeList required to obtain the list.

  @param RegExSyntaxTypeList      A caller-allocated memory buffer filled by the
                                  driver with one EFI_REGEX_SYNTAX_TYPE element
                                  for each supported Regular expression syntax
                                  type. The list must not change across multiple
                                  calls to the same driver. The first syntax
                                  type in the list is the default type for the
                                  driver.

  @retval EFI_SUCCESS            The regular expression syntax types list
                                 was returned successfully.
  @retval EFI_UNSUPPORTED        The service is not supported by this driver.
  @retval EFI_DEVICE_ERROR       The list of syntax types could not be
                                 retrieved due to a hardware or firmware error.
  @retval EFI_BUFFER_TOO_SMALL   The buffer RegExSyntaxTypeList is too small
                                 to hold the result.
  @retval EFI_INVALID_PARAMETER  RegExSyntaxTypeListSize is NULL

**/
EFI_STATUS
EFIAPI
RegularExpressionGetInfo (
  IN     EFI_REGULAR_EXPRESSION_PROTOCOL *This,
  IN OUT UINTN                           *RegExSyntaxTypeListSize,
  OUT    EFI_REGEX_SYNTAX_TYPE           *RegExSyntaxTypeList
  );

#endif
