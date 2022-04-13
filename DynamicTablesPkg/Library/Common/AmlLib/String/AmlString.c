/** @file
  AML String.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <String/AmlString.h>

#include <AmlDefines.h>
#include <IndustryStandard/AcpiAml.h>

/** Check NameString/path information is valid.

  Root, ParentPrefix and SegCount cannot be 0 at the same time.
  This function works for ASL and AML name strings.

  @param [in]   Root          Number of root char.
                              Must be 0 or 1.
  @param [in]   ParentPrefix  Number of carets char ('^').
                              Must be [0-255].
  @param [in]   SegCount      Number of NameSeg (s).
                              Must be [0-255].

  @retval TRUE id the input information is in the right boundaries.
          FALSE otherwise.
**/
BOOLEAN
EFIAPI
AmlIsNameString (
  IN  UINT32  Root,
  IN  UINT32  ParentPrefix,
  IN  UINT32  SegCount
  )
{
  if (((Root == 0) || (Root == 1))            &&
      (ParentPrefix <= MAX_UINT8)             &&
      (!((ParentPrefix != 0) && (Root != 0))) &&
      (SegCount <= MAX_UINT8)                 &&
      ((SegCount + Root + ParentPrefix) != 0))
  {
    return TRUE;
  }

  return FALSE;
}

/** Copy bytes from SrcBuffer to DstBuffer and convert to upper case.
    Don't copy more than MaxDstBufferSize bytes.

  @param  [out] DstBuffer         Destination buffer.
  @param  [in]  MaxDstBufferSize  Maximum size of DstBuffer.
                                  Must be non-zero.
  @param  [in]  SrcBuffer         Source buffer.
  @param  [in]  Count             Count of bytes to copy from SrcBuffer.
                                  Return success if 0.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlUpperCaseMemCpyS (
  OUT       CHAR8   *DstBuffer,
  IN        UINT32  MaxDstBufferSize,
  IN  CONST CHAR8   *SrcBuffer,
  IN        UINT32  Count
  )
{
  UINT32  Index;

  if ((DstBuffer == NULL) ||
      (SrcBuffer == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (Count == 0) {
    return EFI_SUCCESS;
  }

  if (Count > MaxDstBufferSize) {
    Count = MaxDstBufferSize;
  }

  for (Index = 0; Index < Count; Index++) {
    if ((SrcBuffer[Index] >= 'a') && (SrcBuffer[Index] <= 'z')) {
      DstBuffer[Index] = (CHAR8)((UINT8)SrcBuffer[Index] - ('a' - 'A'));
    } else {
      DstBuffer[Index] = SrcBuffer[Index];
    }
  }

  return EFI_SUCCESS;
}

/** Check whether Buffer is a root path ('\').

  This function works for both ASL and AML pathnames.
  Buffer must be at least 2 bytes long.

  @param  [in]  Buffer   An ASL/AML path.

  @retval TRUE    Buffer is a root path
  @retval FALSE   Buffer is not a root path.
**/
BOOLEAN
EFIAPI
AmlIsRootPath (
  IN  CONST  CHAR8  *Buffer
  )
{
  if (Buffer == NULL) {
    return FALSE;
  }

  if ((Buffer[0] == AML_ROOT_CHAR) && (Buffer[1] == '\0')) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/** Check whether Ch is an ASL/AML LeadName.

  This function works for both ASL and AML pathnames.

  ACPI 6.3 specification, s19.2.2. "ASL Name and Pathname Terms":
  LeadNameChar := 'A'-'Z' | 'a'-'z' | '_'

  ACPI 6.3 specification, s20.2.2. "Name Objects Encoding":
  LeadNameChar := 'A'-'Z' | 'a'-'z' | '_'

  @param  [in]  Ch    The char to test.

  @retval TRUE    Ch is an ASL/AML LeadName.
  @retval FALSE   Ch is not an ASL/AML LeadName.
**/
BOOLEAN
EFIAPI
AmlIsLeadNameChar (
  IN  CHAR8  Ch
  )
{
  if ((Ch == '_') || ((Ch >= 'A') && (Ch <= 'Z')) || ((Ch >= 'a') && (Ch <= 'z'))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/** Check whether Ch is an ASL/AML NameChar.

  This function works for both ASL and AML pathnames.

  ACPI 6.3 specification, s19.2.2. "ASL Name and Pathname Terms":
  NameChar := DigitChar | LeadNameChar
  LeadNameChar := 'A'-'Z' | 'a'-'z' | '_'
  DigitChar := '0'-'9'

  ACPI 6.3 specification, s20.2.2. "Name Objects Encoding":
  NameChar := DigitChar | LeadNameChar
  LeadNameChar := 'A'-'Z' | 'a'-'z' | '_'
  DigitChar := '0'-'9'

  @param  [in]  Ch    The char to test.

  @retval TRUE    Ch is an ASL/AML NameChar.
  @retval FALSE   Ch is not an ASL/AML NameChar.
**/
BOOLEAN
EFIAPI
AmlIsNameChar (
  IN  CHAR8  Ch
  )
{
  if (AmlIsLeadNameChar (Ch) || ((Ch >= '0') && (Ch <= '9'))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/** Check whether AslBuffer is an ASL NameSeg.

  This function only works for ASL NameStrings/pathnames.
  ASL NameStrings/pathnames are at most 4 chars long.

  @param [in]   AslBuffer   Pointer in an ASL NameString/pathname.
  @param [out]  Size        Size of the NameSeg.

  @retval TRUE    AslBuffer is an ASL NameSeg.
  @retval FALSE   AslBuffer is not an ASL NameSeg.
**/
BOOLEAN
EFIAPI
AslIsNameSeg (
  IN  CONST  CHAR8   *AslBuffer,
  OUT        UINT32  *Size
  )
{
  UINT32  Index;

  if ((AslBuffer == NULL) ||
      (Size == NULL))
  {
    return FALSE;
  }

  if (!AmlIsLeadNameChar (AslBuffer[0])) {
    return FALSE;
  }

  for (Index = 1; Index < AML_NAME_SEG_SIZE; Index++) {
    if ((AslBuffer[Index] == '.')   ||
        (AslBuffer[Index] == '\0'))
    {
      *Size = Index;
      return TRUE;
    } else if (!AmlIsNameChar (AslBuffer[Index])) {
      return FALSE;
    }
  }

  *Size = Index;
  return TRUE;
}

/** Check whether AmlBuffer is an AML NameSeg.

  This function only works for AML NameStrings/pathnames.
  AML NameStrings/pathnames must be 4 chars long.

  @param [in] AmlBuffer   Pointer in an AML NameString/pathname.

  @retval TRUE    AmlBuffer is an AML NameSeg.
  @retval FALSE   AmlBuffer is not an AML NameSeg.
**/
BOOLEAN
EFIAPI
AmlIsNameSeg (
  IN  CONST  CHAR8  *AmlBuffer
  )
{
  UINT32  Index;

  if (AmlBuffer == NULL) {
    return FALSE;
  }

  if (!AmlIsLeadNameChar (AmlBuffer[0])) {
    return FALSE;
  }

  for (Index = 1; Index < AML_NAME_SEG_SIZE; Index++) {
    if (!AmlIsNameChar (AmlBuffer[Index])) {
      return FALSE;
    }
  }

  return TRUE;
}

/** Parse an ASL NameString/path.

  An ASL NameString/path must be NULL terminated.
  Information found in the ASL NameString/path is returned via pointers:
  Root, ParentPrefix, SegCount.

  @param [in]    Buffer       ASL NameString/path.
  @param [out]   Root         Pointer holding the number of root char.
                              Can be 0 or 1.
  @param [out]   ParentPrefix Pointer holding the number of carets char ('^').
                              Can be [0-255].
  @param [out]   SegCount     Pointer holding the number of NameSeg (s).
                              Can be [0-255].

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AslParseNameStringInfo (
  IN  CONST CHAR8   *Buffer,
  OUT       UINT32  *Root,
  OUT       UINT32  *ParentPrefix,
  OUT       UINT32  *SegCount
  )
{
  UINT32  NameSegSize;

  if ((Buffer == NULL)        ||
      (Root == NULL)          ||
      (ParentPrefix == NULL)  ||
      (SegCount == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *Root         = 0;
  *ParentPrefix = 0;
  *SegCount     = 0;

  // Handle Root and ParentPrefix(s).
  if (*Buffer == AML_ROOT_CHAR) {
    *Root = 1;
    Buffer++;
  } else if (*Buffer == AML_PARENT_PREFIX_CHAR) {
    do {
      Buffer++;
      (*ParentPrefix)++;
    } while (*Buffer == AML_PARENT_PREFIX_CHAR);
  }

  // Handle SegCount(s).
  while (AslIsNameSeg (Buffer, &NameSegSize)) {
    // Safety checks on NameSegSize.
    if ((NameSegSize == 0) || (NameSegSize > AML_NAME_SEG_SIZE)) {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    // Increment the NameSeg count.
    (*SegCount)++;
    Buffer += NameSegSize;

    // Skip the '.' separator if present.
    if (*Buffer == '.') {
      Buffer++;
    }
  } // while

  // An ASL NameString/path must be NULL terminated.
  if (*Buffer != '\0') {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (!AmlIsNameString (*Root, *ParentPrefix, *SegCount)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Parse an AML NameString/path.

  It is possible to determine the size of an AML NameString/path just
  by sight reading it. So no overflow can occur.
  Information found in the AML NameString/path is returned via pointers:
  Root, ParentPrefix, SegCount.

  @param [in]    Buffer         AML NameString/path.
  @param [out]   Root           Pointer holding the number of root char.
                                Can be 0 or 1.
  @param [out]   ParentPrefix   Pointer holding the number of carets char ('^').
                                Can be [0-255].
  @param [out]   SegCount       Pointer holding the number of NameSeg(s).
                                Can be [0-255].

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlParseNameStringInfo (
  IN  CONST CHAR8   *Buffer,
  OUT       UINT32  *Root,
  OUT       UINT32  *ParentPrefix,
  OUT       UINT32  *SegCount
  )
{
  if ((Buffer == NULL) ||
      (Root == NULL)   ||
      (ParentPrefix == NULL) ||
      (SegCount == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *Root         = 0;
  *ParentPrefix = 0;
  *SegCount     = 0;

  // Handle Root and ParentPrefix(s).
  if (*Buffer == AML_ROOT_CHAR) {
    *Root = 1;
    Buffer++;
  } else if (*Buffer == AML_PARENT_PREFIX_CHAR) {
    do {
      Buffer++;
      (*ParentPrefix)++;
    } while (*Buffer == AML_PARENT_PREFIX_CHAR);
  }

  // Handle SegCount(s).
  if (*Buffer == AML_DUAL_NAME_PREFIX) {
    *SegCount = 2;
  } else if (*Buffer == AML_MULTI_NAME_PREFIX) {
    *SegCount = *((UINT8 *)(Buffer + 1));
  } else if (AmlIsNameSeg (Buffer)) {
    *SegCount = 1;
  } else if (*Buffer == AML_ZERO_OP) {
    *SegCount = 0;
  } else {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Safety checks on exit.
  if (!AmlIsNameString (*Root, *ParentPrefix, *SegCount)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Compute the ASL NameString/path size from NameString
    information (Root, ParentPrefix, SegCount).

  @param [in] Root          Number of root char.
                            Can be 0 or 1.
  @param [in] ParentPrefix  Number of carets char ('^').
                            Can be [0-255].
  @param [in] SegCount      Pointer holding the number of NameSeg(s).
                            Can be [0-255].

  @return Size of the ASL NameString/path.
**/
UINT32
EFIAPI
AslComputeNameStringSize (
  IN  UINT32  Root,
  IN  UINT32  ParentPrefix,
  IN  UINT32  SegCount
  )
{
  UINT32  TotalSize;

  if (!AmlIsNameString (Root, ParentPrefix, SegCount)) {
    ASSERT (0);
    return 0;
  }

  // Root and ParentPrefix(s).
  TotalSize = Root + ParentPrefix;

  // Add size required for NameSeg(s).
  TotalSize += (SegCount * AML_NAME_SEG_SIZE);

  // Add size required for '.' separator(s).
  TotalSize += (SegCount > 1) ? (SegCount - 1) : 0;

  // Add 1 byte for NULL termination '\0'.
  TotalSize += 1;

  return TotalSize;
}

/** Compute the AML NameString/path size from NameString
    information (Root, ParentPrefix, SegCount).

  @param [in] Root          Number of root char.
                            Can be 0 or 1.
  @param [in] ParentPrefix  Number of carets char ('^').
                            Can be [0-255].
  @param [in] SegCount      Pointer holding the number of NameSeg(s).
                            Can be [0-255].

  @return Size of the AML NameString/path.
**/
UINT32
EFIAPI
AmlComputeNameStringSize (
  IN  UINT32  Root,
  IN  UINT32  ParentPrefix,
  IN  UINT32  SegCount
  )
{
  UINT32  TotalSize;

  if (!AmlIsNameString (Root, ParentPrefix, SegCount)) {
    ASSERT (0);
    return 0;
  }

  // Root and ParentPrefix(s).
  TotalSize = Root + ParentPrefix;

  // If SegCount == 0, '\0' must end the AML NameString/path.
  TotalSize += (SegCount == 0) ? 1 : (SegCount * AML_NAME_SEG_SIZE);

  // AML prefix. SegCount > 2 = MultiNamePrefix, SegCount = 2 DualNamePrefix.
  TotalSize += (SegCount > 2) ? 2 : ((SegCount == 2) ? 1 : 0);

  return TotalSize;
}

/** Get the ASL NameString/path size.

  @param [in]   AslPath         An ASL NameString/path.
  @param [out]  AslPathSizePtr  Pointer holding the ASL NameString/path size.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AslGetNameStringSize (
  IN  CONST CHAR8   *AslPath,
  OUT       UINT32  *AslPathSizePtr
  )
{
  if ((AslPath == NULL) ||
      (AslPathSizePtr == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  *AslPathSizePtr = 0;
  do {
    (*AslPathSizePtr)++;
    AslPath++;
  } while (*AslPath != '\0');

  return EFI_SUCCESS;
}

/** Get the AML NameString/path size.

  @param [in]   AmlPath         An AML NameString/path.
  @param [out]  AmlPathSizePtr  Pointer holding the AML NameString/path size.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetNameStringSize (
  IN   CONST  CHAR8   *AmlPath,
  OUT         UINT32  *AmlPathSizePtr
  )
{
  EFI_STATUS  Status;

  UINT32  Root;
  UINT32  ParentPrefix;
  UINT32  SegCount;

  if ((AmlPath == NULL) ||
      (AmlPathSizePtr == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlParseNameStringInfo (
             AmlPath,
             &Root,
             &ParentPrefix,
             &SegCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  *AmlPathSizePtr = AmlComputeNameStringSize (Root, ParentPrefix, SegCount);
  if (*AmlPathSizePtr == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/** Convert an ASL NameString/path to an AML NameString/path.
    The caller must free the memory allocated in this function
    for AmlPath using FreePool ().

  @param  [in]  AslPath     An ASL NameString/path.
  @param  [out] OutAmlPath  Buffer containing the AML path.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
ConvertAslNameToAmlName (
  IN  CONST  CHAR8  *AslPath,
  OUT        CHAR8  **OutAmlPath
  )
{
  EFI_STATUS  Status;

  UINT32  Root;
  UINT32  ParentPrefix;
  UINT32  SegCount;
  UINT32  TotalSize;
  UINT32  NameSegSize;

  CONST CHAR8  *AslBuffer;
  CHAR8        *AmlBuffer;
  CHAR8        *AmlPath;

  if ((AslPath == NULL) ||
      (OutAmlPath == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Analyze AslPath. AslPath is checked in the call.
  Status = AslParseNameStringInfo (AslPath, &Root, &ParentPrefix, &SegCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Compute TotalSize.
  TotalSize = AmlComputeNameStringSize (Root, ParentPrefix, SegCount);
  if (TotalSize == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Allocate memory.
  AmlPath = AllocateZeroPool (TotalSize);
  if (AmlPath == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  AmlBuffer = AmlPath;
  AslBuffer = AslPath;

  // Handle Root and ParentPrefix(s).
  if (Root == 1) {
    *AmlBuffer = AML_ROOT_CHAR;
    AmlBuffer++;
    AslBuffer++;
  } else if (ParentPrefix > 0) {
    SetMem (AmlBuffer, ParentPrefix, AML_PARENT_PREFIX_CHAR);
    AmlBuffer += ParentPrefix;
    AslBuffer += ParentPrefix;
  }

  // Handle prefix and SegCount(s).
  if (SegCount > 2) {
    *AmlBuffer = AML_MULTI_NAME_PREFIX;
    AmlBuffer++;
    *AmlBuffer = (UINT8)SegCount;
    AmlBuffer++;
  } else if (SegCount == 2) {
    *AmlBuffer = AML_DUAL_NAME_PREFIX;
    AmlBuffer++;
  }

  if (SegCount != 0) {
    // Write NameSeg(s).
    while (1) {
      SegCount--;

      // Get the NameSeg size.
      if (!AslIsNameSeg (AslBuffer, &NameSegSize)) {
        ASSERT (0);
        Status = EFI_INVALID_PARAMETER;
        goto error_handler;
      }

      // Convert to Upper case and copy.
      Status = AmlUpperCaseMemCpyS (
                 AmlBuffer,
                 TotalSize,
                 AslBuffer,
                 NameSegSize
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        goto error_handler;
      }

      // Complete the NameSeg with an underscore ('_') if shorter than 4 bytes.
      SetMem (
        AmlBuffer + NameSegSize,
        AML_NAME_SEG_SIZE - NameSegSize,
        AML_NAME_CHAR__
        );

      // Go to the next NameSeg.
      AmlBuffer += AML_NAME_SEG_SIZE;
      AslBuffer += NameSegSize;

      // Skip the '.' separator.
      if (SegCount != 0) {
        if (*AslBuffer == '.') {
          AslBuffer++;
        } else {
          ASSERT (0);
          Status = EFI_INVALID_PARAMETER;
          goto error_handler;
        }
      } else {
        // (SegCount == 0)
        if (*AslBuffer == '\0') {
          break;
        } else {
          ASSERT (0);
          Status = EFI_INVALID_PARAMETER;
          goto error_handler;
        }
      }
    } // while
  } else {
    // (SegCount == 0)
    // '\0' needs to end the AML NameString/path.
    *AmlBuffer = AML_ZERO_OP;
    AmlBuffer++;
  }

  // Safety checks on exit.
  // Check that AmlPath has been filled with TotalSize bytes.
  if ((SegCount != 0)               ||
      (*AslBuffer != AML_ZERO_OP)   ||
      (((UINT32)(AmlBuffer - AmlPath)) != TotalSize))
  {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  *OutAmlPath = AmlPath;
  return EFI_SUCCESS;

error_handler:
  FreePool (AmlPath);
  return Status;
}

/** Convert an AML NameString/path to an ASL NameString/path.
    The caller must free the memory allocated in this function.
    using FreePool ().

  @param  [in]  AmlPath     An AML NameString/path.
  @param  [out] OutAslPath  Buffer containing the ASL path.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
ConvertAmlNameToAslName (
  IN  CONST CHAR8  *AmlPath,
  OUT       CHAR8  **OutAslPath
  )
{
  EFI_STATUS  Status;

  UINT32  Root;
  UINT32  ParentPrefix;
  UINT32  SegCount;
  UINT32  TotalSize;

  CONST CHAR8  *AmlBuffer;
  CHAR8        *AslBuffer;
  CHAR8        *AslPath;

  if ((AmlPath == NULL)   ||
      (OutAslPath == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Analyze AslPath. AmlPath is checked in the call.
  Status = AmlParseNameStringInfo (AmlPath, &Root, &ParentPrefix, &SegCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Compute TotalSize.
  TotalSize = AslComputeNameStringSize (Root, ParentPrefix, SegCount);
  if (TotalSize == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Allocate memory.
  AslPath = AllocateZeroPool (TotalSize);
  if (AslPath == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  AmlBuffer = AmlPath;
  AslBuffer = AslPath;

  // Handle prefix and SegCount(s).
  if (Root == 1) {
    *AslBuffer = AML_ROOT_CHAR;
    AslBuffer++;
    AmlBuffer++;
  } else if (ParentPrefix > 0) {
    SetMem (AslBuffer, ParentPrefix, AML_PARENT_PREFIX_CHAR);
    AslBuffer += ParentPrefix;
    AmlBuffer += ParentPrefix;
  }

  // Handle Root and Parent(s).
  // Skip the MultiName or DualName prefix chars.
  if (SegCount > 2) {
    AmlBuffer += 2;
  } else if (SegCount == 2) {
    AmlBuffer += 1;
  }

  // Write NameSeg(s).
  while (SegCount) {
    // NameSeg is already in upper case and always 4 bytes long.
    CopyMem (AslBuffer, AmlBuffer, AML_NAME_SEG_SIZE);
    AslBuffer += AML_NAME_SEG_SIZE;
    AmlBuffer += AML_NAME_SEG_SIZE;

    SegCount--;

    // Write the '.' separator if there is another NameSeg following.
    if (SegCount != 0) {
      *AslBuffer = '.';
      AslBuffer++;
    }
  } // while

  // NULL terminate the ASL NameString.
  *AslBuffer = '\0';
  AslBuffer++;

  // Safety checks on exit.
  // Check that AslPath has been filled with TotalSize bytes.
  if (((UINT32)(AslBuffer - AslPath)) != TotalSize) {
    ASSERT (0);
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  *OutAslPath = AslPath;
  return EFI_SUCCESS;

error_handler:
  FreePool (AslPath);
  return Status;
}

/** Compare two ASL NameStrings.

  @param [in] AslName1    First NameString to compare.
  @param [in] AslName2    Second NameString to compare.

  @retval TRUE if the two strings are identical.
  @retval FALSE otherwise, or if error.
**/
BOOLEAN
EFIAPI
AslCompareNameString (
  IN  CONST CHAR8  *AslName1,
  IN  CONST CHAR8  *AslName2
  )
{
  EFI_STATUS  Status;
  UINT32      AslName1Len;
  UINT32      AslName2Len;

  if ((AslName1 == NULL) ||
      (AslName2 == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  Status = AslGetNameStringSize (AslName1, &AslName1Len);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return FALSE;
  }

  Status = AslGetNameStringSize (AslName2, &AslName2Len);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return FALSE;
  }

  // AslName1 and AslName2 don't have the same length
  if (AslName1Len != AslName2Len) {
    return FALSE;
  }

  return (CompareMem (AslName1, AslName2, AslName1Len) == 0);
}

/** Compare two AML NameStrings.

  @param [in] AmlName1    First NameString to compare.
  @param [in] AmlName2    Second NameString to compare.

  @retval TRUE if the two strings are identical.
  @retval FALSE otherwise, or if error.
**/
BOOLEAN
EFIAPI
AmlCompareNameString (
  IN  CONST CHAR8  *AmlName1,
  IN  CONST CHAR8  *AmlName2
  )
{
  EFI_STATUS  Status;
  UINT32      AmlName1Len;
  UINT32      AmlName2Len;

  if ((AmlName1 == NULL) ||
      (AmlName2 == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  Status = AmlGetNameStringSize (AmlName1, &AmlName1Len);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return FALSE;
  }

  Status = AmlGetNameStringSize (AmlName2, &AmlName2Len);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return FALSE;
  }

  // AmlName1 and AmlName2 don't have the same length
  if (AmlName1Len != AmlName2Len) {
    return FALSE;
  }

  return (CompareMem (AmlName1, AmlName2, AmlName1Len) == 0);
}

/** Compare an AML NameString and an ASL NameString.

  The ASL NameString is converted to an AML NameString before
  being compared with the ASL NameString. This allows to expand
  NameSegs shorter than 4 chars.
  E.g.: AslName: "DEV" will be expanded to "DEV_" before being
        compared.

  @param [in] AmlName1   AML NameString to compare.
  @param [in] AslName2   ASL NameString to compare.

  @retval TRUE if the two strings are identical.
  @retval FALSE otherwise, or if error.
**/
BOOLEAN
EFIAPI
CompareAmlWithAslNameString (
  IN  CONST CHAR8  *AmlName1,
  IN  CONST CHAR8  *AslName2
  )
{
  EFI_STATUS  Status;

  CHAR8    *AmlName2;
  BOOLEAN  RetVal;

  if ((AmlName1 == NULL) ||
      (AslName2 == NULL))
  {
    ASSERT (0);
    return FALSE;
  }

  // Convert the AslName2 to an AmlName2.
  // AmlName2 must be freed.
  Status = ConvertAmlNameToAslName (AslName2, &AmlName2);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return FALSE;
  }

  RetVal = AmlCompareNameString (AmlName1, AmlName2);

  // Free AmlName2.
  FreePool (AmlName2);

  return RetVal;
}

/** Given an AmlPath, return the address of the first NameSeg.

  It is possible to determine the size of an AML NameString/path just
  by sight reading it. So no overflow can occur.

  @param  [in]  AmlPath       The AML pathname.
  @param  [in]  Root          The AML pathname starts with a root char.
                              It is an absolute path.
  @param  [in]  ParentPrefix  The AML pathname has ParentPrefix
                              carets in its name.

  @return Pointer to the first NameSeg of the NameString.
          Return NULL if AmlPath is NULL.
**/
CONST
CHAR8 *
EFIAPI
AmlGetFirstNameSeg (
  IN  CONST  CHAR8   *AmlPath,
  IN         UINT32  Root,
  IN         UINT32  ParentPrefix
  )
{
  if (AmlPath == NULL) {
    ASSERT (0);
    return NULL;
  }

  AmlPath += Root;
  AmlPath += ParentPrefix;
  AmlPath += ((*AmlPath == AML_MULTI_NAME_PREFIX) ? 2
               : (*AmlPath == AML_DUAL_NAME_PREFIX) ? 1 : 0);
  return AmlPath;
}
