/** @file

  EFI_REGULAR_EXPRESSION_PROTOCOL Implementation

  (C) Copyright 2015-2016 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RegularExpressionDxe.h"

STATIC
EFI_REGEX_SYNTAX_TYPE *CONST  mSupportedSyntaxes[] = {
  &gEfiRegexSyntaxTypePosixExtendedGuid,
  &gEfiRegexSyntaxTypePerlGuid
};

STATIC
EFI_REGULAR_EXPRESSION_PROTOCOL  mProtocolInstance = {
  RegularExpressionMatch,
  RegularExpressionGetInfo
};

#define CHAR16_ENCODING  ONIG_ENCODING_UTF16_LE

/**
  Call the Oniguruma regex match API.

  Same parameters as RegularExpressionMatch, except SyntaxType is required.

  @param String         A pointer to a NULL terminated string to match against the
                        regular expression string specified by Pattern.

  @param Pattern        A pointer to a NULL terminated string that represents the
                        regular expression.
  @param SyntaxType     A pointer to the EFI_REGEX_SYNTAX_TYPE that identifies the
                        regular expression syntax type to use. May be NULL in which
                        case the function will use its default regular expression
                        syntax type.

  @param Result         On return, points to TRUE if String fully matches against
                        the regular expression Pattern using the regular expression
                        SyntaxType. Otherwise, points to FALSE.

  @param Captures       A Pointer to an array of EFI_REGEX_CAPTURE objects to receive
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

  @param CapturesCount  On output, CapturesCount is the number of matching patterns
                        found in String. Zero means no matching patterns were found
                        in the string.

  @retval  EFI_SUCCESS       Regex compilation and match completed successfully.
  @retval  EFI_DEVICE_ERROR  Regex compilation failed.

**/
STATIC
EFI_STATUS
OnigurumaMatch (
  IN  CHAR16                 *String,
  IN  CHAR16                 *Pattern,
  IN  EFI_REGEX_SYNTAX_TYPE  *SyntaxType,
  OUT BOOLEAN                *Result,
  OUT EFI_REGEX_CAPTURE      **Captures      OPTIONAL,
  OUT UINTN                  *CapturesCount
  )
{
  regex_t         *OnigRegex;
  OnigSyntaxType  *OnigSyntax;
  OnigRegion      *Region;
  INT32           OnigResult;
  OnigErrorInfo   ErrorInfo;
  OnigUChar       ErrorMessage[ONIG_MAX_ERROR_MESSAGE_LEN];
  UINTN           Index;
  OnigUChar       *Start;
  EFI_STATUS      Status;

  Status = EFI_SUCCESS;

  //
  // Detemine the internal syntax type
  //
  OnigSyntax = ONIG_SYNTAX_DEFAULT;
  if (CompareGuid (SyntaxType, &gEfiRegexSyntaxTypePosixExtendedGuid)) {
    OnigSyntax = ONIG_SYNTAX_POSIX_EXTENDED;
  } else if (CompareGuid (SyntaxType, &gEfiRegexSyntaxTypePerlGuid)) {
    OnigSyntax = ONIG_SYNTAX_PERL;
  } else {
    DEBUG ((DEBUG_ERROR, "Unsupported regex syntax - using default\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Compile pattern
  //
  Start      = (OnigUChar *)Pattern;
  OnigResult = onig_new (
                 &OnigRegex,
                 Start,
                 Start + onigenc_str_bytelen_null (CHAR16_ENCODING, Start),
                 ONIG_OPTION_DEFAULT,
                 CHAR16_ENCODING,
                 OnigSyntax,
                 &ErrorInfo
                 );

  if (OnigResult != ONIG_NORMAL) {
    onig_error_code_to_str (ErrorMessage, OnigResult, &ErrorInfo);
    DEBUG ((DEBUG_ERROR, "Regex compilation failed: %a\n", ErrorMessage));
    return EFI_DEVICE_ERROR;
  }

  //
  // Try to match
  //
  Start  = (OnigUChar *)String;
  Region = onig_region_new ();
  if (Region == NULL) {
    onig_free (OnigRegex);
    return EFI_OUT_OF_RESOURCES;
  }

  OnigResult = onig_search (
                 OnigRegex,
                 Start,
                 Start + onigenc_str_bytelen_null (CHAR16_ENCODING, Start),
                 Start,
                 Start + onigenc_str_bytelen_null (CHAR16_ENCODING, Start),
                 Region,
                 ONIG_OPTION_NONE
                 );

  if (OnigResult >= 0) {
    *Result = TRUE;
  } else {
    *Result = FALSE;
    if (OnigResult != ONIG_MISMATCH) {
      onig_error_code_to_str (ErrorMessage, OnigResult);
      DEBUG ((DEBUG_ERROR, "Regex match failed: %a\n", ErrorMessage));
      onig_region_free (Region, 1);
      onig_free (OnigRegex);
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // If successful, copy out the region (capture) information
  //
  if (*Result && (Captures != NULL)) {
    *CapturesCount = Region->num_regs;
    *Captures      = AllocateZeroPool (*CapturesCount * sizeof (**Captures));
    if (*Captures != NULL) {
      for (Index = 0; Index < *CapturesCount; ++Index) {
        //
        // Region beg/end values represent bytes, not characters
        //
        (*Captures)[Index].Length     = (Region->end[Index] - Region->beg[Index]) / sizeof (CHAR16);
        (*Captures)[Index].CapturePtr = AllocateCopyPool (
                                          ((*Captures)[Index].Length) * sizeof (CHAR16),
                                          (CHAR16 *)((UINTN)String + Region->beg[Index])
                                          );
        if ((*Captures)[Index].CapturePtr == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }
      }

      if (EFI_ERROR (Status)) {
        for (Index = 0; Index < *CapturesCount; ++Index) {
          if ((*Captures)[Index].CapturePtr != NULL) {
            FreePool ((CHAR16 *)(*Captures)[Index].CapturePtr);
          }
        }

        FreePool (*Captures);
      }
    }
  }

  onig_region_free (Region, 1);
  onig_free (OnigRegex);

  return Status;
}

/**
  Returns information about the regular expression syntax types supported
  by the implementation.

  @param This                      A pointer to the EFI_REGULAR_EXPRESSION_PROTOCOL
                                   instance.

  @param  RegExSyntaxTypeListSize  On input, the size in bytes of RegExSyntaxTypeList.
                                   On output with a return code of EFI_SUCCESS, the
                                   size in bytes of the data returned in
                                   RegExSyntaxTypeList. On output with a return code
                                   of EFI_BUFFER_TOO_SMALL, the size of
                                   RegExSyntaxTypeList required to obtain the list.

  @param   RegExSyntaxTypeList     A caller-allocated memory buffer filled by the
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
  IN     EFI_REGULAR_EXPRESSION_PROTOCOL  *This,
  IN OUT UINTN                            *RegExSyntaxTypeListSize,
  OUT    EFI_REGEX_SYNTAX_TYPE            *RegExSyntaxTypeList
  )
{
  UINTN  SyntaxSize;
  UINTN  Index;

  if ((This == NULL) || (RegExSyntaxTypeListSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*RegExSyntaxTypeListSize != 0) && (RegExSyntaxTypeList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SyntaxSize = ARRAY_SIZE (mSupportedSyntaxes) * sizeof (**mSupportedSyntaxes);

  if (*RegExSyntaxTypeListSize < SyntaxSize) {
    *RegExSyntaxTypeListSize = SyntaxSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  for (Index = 0; Index < ARRAY_SIZE (mSupportedSyntaxes); ++Index) {
    CopyMem (&RegExSyntaxTypeList[Index], mSupportedSyntaxes[Index], sizeof (**mSupportedSyntaxes));
  }

  *RegExSyntaxTypeListSize = SyntaxSize;

  return EFI_SUCCESS;
}

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
  IN  EFI_REGULAR_EXPRESSION_PROTOCOL  *This,
  IN  CHAR16                           *String,
  IN  CHAR16                           *Pattern,
  IN  EFI_REGEX_SYNTAX_TYPE            *SyntaxType  OPTIONAL,
  OUT BOOLEAN                          *Result,
  OUT EFI_REGEX_CAPTURE                **Captures  OPTIONAL,
  OUT UINTN                            *CapturesCount
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  BOOLEAN     Supported;

  if ((This == NULL) || (String == NULL) || (Pattern == NULL) || (Result == NULL) || (CapturesCount == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Figure out which syntax to use
  //
  if (SyntaxType == NULL) {
    SyntaxType = mSupportedSyntaxes[0];
  } else {
    Supported = FALSE;
    for (Index = 0; Index < ARRAY_SIZE (mSupportedSyntaxes); ++Index) {
      if (CompareGuid (SyntaxType, mSupportedSyntaxes[Index])) {
        Supported = TRUE;
        break;
      }
    }

    if (!Supported) {
      return EFI_UNSUPPORTED;
    }
  }

  Status = OnigurumaMatch (String, Pattern, SyntaxType, Result, Captures, CapturesCount);

  return Status;
}

/**
  Entry point for RegularExpressionDxe.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval Status         Whether this function complete successfully.

**/
EFI_STATUS
EFIAPI
RegularExpressionDxeEntry (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiRegularExpressionProtocolGuid,
                  &mProtocolInstance,
                  NULL
                  );

  return Status;
}
