/** @file
  AML String.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_STRING_H_
#define AML_STRING_H_

/* This header file does not include internal Node definition,
   i.e. AML_ROOT_NODE, AML_OBJECT_NODE, etc. The node definitions
   must be included by the caller file. The function prototypes must
   only expose AML_NODE_HANDLE, AML_ROOT_NODE_HANDLE, etc. node
   definitions.
   This allows to keep the functions defined here both internal and
   potentially external. If necessary, any function of this file can
   be exposed externally.
   The Api folder is internal to the AmlLib, but should only use these
   functions. They provide a "safe" way to interact with the AmlLib.
*/

#include <AmlInclude.h>

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
  IN  UINT32    Root,
  IN  UINT32    ParentPrefix,
  IN  UINT32    SegCount
  );

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
  OUT       CHAR8   * DstBuffer,
  IN        UINT32    MaxDstBufferSize,
  IN  CONST CHAR8   * SrcBuffer,
  IN        UINT32    Count
  );

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
  IN  CONST  CHAR8  * Buffer
  );

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
  IN  CHAR8   Ch
  );

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
  );

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
  IN  CONST  CHAR8    * AslBuffer,
  OUT        UINT32   * Size
  );

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
  IN  CONST  CHAR8    * AmlBuffer
  );

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
  IN  CONST CHAR8     * Buffer,
  OUT       UINT32    * Root,
  OUT       UINT32    * ParentPrefix,
  OUT       UINT32    * SegCount
  );

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
  IN  CONST CHAR8   * Buffer,
  OUT       UINT32  * Root,
  OUT       UINT32  * ParentPrefix,
  OUT       UINT32  * SegCount
  );

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
  IN  UINT32    Root,
  IN  UINT32    ParentPrefix,
  IN  UINT32    SegCount
  );

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
  IN  UINT32    Root,
  IN  UINT32    ParentPrefix,
  IN  UINT32    SegCount
  );

/** Get the ASL NameString/path size.

  @param [in]   AslPath         An ASL NameString/path.
  @param [out]  AslPathSizePtr  Pointer holding the ASL NameString/path size.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AslGetNameStringSize (
  IN  CONST CHAR8   * AslPath,
  OUT       UINT32  * AslPathSizePtr
  );

/** Get the AML NameString/path size.

  @param [in]   AmlPath         An AML NameString/path.
  @param [out]  AmlPathSizePtr  Pointer holding the AML NameString/path size.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetNameStringSize (
  IN   CONST  CHAR8   * AmlPath,
  OUT         UINT32  * AmlPathSizePtr
  );

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
  IN  CONST  CHAR8   * AslPath,
  OUT        CHAR8  ** OutAmlPath
  );

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
  IN  CONST CHAR8     * AmlPath,
  OUT       CHAR8    ** OutAslPath
  );

/** Compare two ASL NameStrings.

  @param [in] AslName1    First NameString to compare.
  @param [in] AslName2    Second NameString to compare.

  @retval TRUE if the two strings are identical.
  @retval FALSE otherwise, or if error.
**/
BOOLEAN
EFIAPI
AslCompareNameString (
  IN  CONST CHAR8 *   AslName1,
  IN  CONST CHAR8 *   AslName2
  );

/** Compare two AML NameStrings.

  @param [in] AmlName1    First NameString to compare.
  @param [in] AmlName2    Second NameString to compare.

  @retval TRUE if the two strings are identical.
  @retval FALSE otherwise, or if error.
**/
BOOLEAN
EFIAPI
AmlCompareNameString (
  IN  CONST CHAR8 *   AmlName1,
  IN  CONST CHAR8 *   AmlName2
  );

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
  IN  CONST CHAR8 *   AmlName1,
  IN  CONST CHAR8 *   AslName2
  );

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
  IN  CONST  CHAR8    * AmlPath,
  IN         UINT32     Root,
  IN         UINT32     ParentPrefix
  );

#endif // AML_STRING_H_
