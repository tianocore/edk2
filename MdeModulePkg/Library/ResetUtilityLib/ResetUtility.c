/** @file
  This contains the business logic for the module-specific Reset Helper functions.

  Copyright (c) 2017 - 2019 Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 Microsoft Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ResetSystemLib.h>

#pragma pack(1)
typedef struct {
  CHAR16    NullTerminator;
  GUID      ResetSubtype;
} RESET_UTILITY_GUID_SPECIFIC_RESET_DATA;
#pragma pack()

STATIC_ASSERT (
  sizeof (RESET_UTILITY_GUID_SPECIFIC_RESET_DATA) == 18,
  "sizeof (RESET_UTILITY_GUID_SPECIFIC_RESET_DATA) is expected to be 18 bytes"
  );

/**
  This is a shorthand helper function to reset with reset type and a subtype
  so that the caller doesn't have to bother with a function that has half
  a dozen parameters.

  This will generate a reset with status EFI_SUCCESS, a NULL string, and
  no custom data. The subtype will be formatted in such a way that it can be
  picked up by notification registrations and custom handlers.

  NOTE: This call will fail if the architectural ResetSystem underpinnings
        are not initialized. For DXE, you can add gEfiResetArchProtocolGuid
        to your DEPEX.

  @param[in]  ResetType     The default EFI_RESET_TYPE of the reset.
  @param[in]  ResetSubtype  GUID pointer for the reset subtype to be used.

**/
VOID
EFIAPI
ResetSystemWithSubtype (
  IN EFI_RESET_TYPE  ResetType,
  IN CONST  GUID     *ResetSubtype
  )
{
  RESET_UTILITY_GUID_SPECIFIC_RESET_DATA  ResetData;

  ResetData.NullTerminator = CHAR_NULL;
  CopyGuid (
    (GUID *)((UINT8 *)&ResetData + OFFSET_OF (RESET_UTILITY_GUID_SPECIFIC_RESET_DATA, ResetSubtype)),
    ResetSubtype
    );

  ResetSystem (ResetType, EFI_SUCCESS, sizeof (ResetData), &ResetData);
}

/**
  This is a shorthand helper function to reset with the reset type
  'EfiResetPlatformSpecific' and a subtype so that the caller doesn't
  have to bother with a function that has half a dozen parameters.

  This will generate a reset with status EFI_SUCCESS, a NULL string, and
  no custom data. The subtype will be formatted in such a way that it can be
  picked up by notification registrations and custom handlers.

  NOTE: This call will fail if the architectural ResetSystem underpinnings
        are not initialized. For DXE, you can add gEfiResetArchProtocolGuid
        to your DEPEX.

  @param[in]  ResetSubtype  GUID pointer for the reset subtype to be used.

**/
VOID
EFIAPI
ResetPlatformSpecificGuid (
  IN CONST  GUID  *ResetSubtype
  )
{
  ResetSystemWithSubtype (EfiResetPlatformSpecific, ResetSubtype);
}

/**
  This function examines the DataSize and ResetData parameters passed to
  to ResetSystem() and detemrines if the ResetData contains a Null-terminated
  Unicode string followed by a GUID specific subtype.  If the GUID specific
  subtype is present, then a pointer to the GUID value in ResetData is returned.

  @param[in]  DataSize    The size, in bytes, of ResetData.
  @param[in]  ResetData   Pointer to the data buffer passed into ResetSystem().

  @retval     Pointer     Pointer to the GUID value in ResetData.
  @retval     NULL        ResetData is NULL.
  @retval     NULL        ResetData does not start with a Null-terminated
                          Unicode string.
  @retval     NULL        A Null-terminated Unicode string is present, but there
                          are less than sizeof (GUID) bytes after the string.
  @retval     NULL        No subtype is found.

**/
GUID *
EFIAPI
GetResetPlatformSpecificGuid (
  IN UINTN       DataSize,
  IN CONST VOID  *ResetData
  )
{
  UINTN  ResetDataStringSize;
  GUID   *ResetSubtypeGuid;

  //
  // Make sure parameters are valid
  //
  if ((ResetData == NULL) || (DataSize < sizeof (GUID))) {
    return NULL;
  }

  //
  // Determine the number of bytes in the Null-terminated Unicode string
  // at the beginning of ResetData including the Null terminator.
  //
  ResetDataStringSize = StrnSizeS (ResetData, (DataSize / sizeof (CHAR16)));

  //
  // Now, assuming that we have enough data for a GUID after the string, the
  // GUID should be immediately after the string itself.
  //
  if ((ResetDataStringSize < DataSize) && ((DataSize - ResetDataStringSize) >= sizeof (GUID))) {
    ResetSubtypeGuid = (GUID *)((UINT8 *)ResetData + ResetDataStringSize);
    DEBUG ((DEBUG_VERBOSE, "%a - Detected reset subtype %g...\n", __func__, ResetSubtypeGuid));
    return ResetSubtypeGuid;
  }

  return NULL;
}

/**
  This is a helper function that creates the reset data buffer that can be
  passed into ResetSystem().

  The reset data buffer is returned in ResetData and contains ResetString
  followed by the ResetSubtype GUID followed by the ExtraData.

  NOTE: Strings are internally limited by MAX_UINT16.

  @param[in, out] ResetDataSize  On input, the size of the ResetData buffer. On
                                 output, either the total number of bytes
                                 copied, or the required buffer size.
  @param[in, out] ResetData      A pointer to the buffer in which to place the
                                 final structure.
  @param[in]      ResetSubtype   Pointer to the GUID specific subtype.  This
                                 parameter is optional and may be NULL.
  @param[in]      ResetString    Pointer to a Null-terminated Unicode string
                                 that describes the reset.  This parameter is
                                 optional and may be NULL.
  @param[in]      ExtraDataSize  The size, in bytes, of ExtraData buffer.
  @param[in]      ExtraData      Pointer to a buffer of extra data.  This
                                 parameter is optional and may be NULL.

  @retval     RETURN_SUCCESS             ResetDataSize and ResetData are updated.
  @retval     RETURN_INVALID_PARAMETER   ResetDataSize is NULL.
  @retval     RETURN_INVALID_PARAMETER   ResetData is NULL.
  @retval     RETURN_INVALID_PARAMETER   ExtraData was provided without a
                                         ResetSubtype. This is not supported by the
                                         UEFI spec.
  @retval     RETURN_BUFFER_TOO_SMALL    An insufficient buffer was provided.
                                         ResetDataSize is updated with minimum size
                                         required.
**/
RETURN_STATUS
EFIAPI
BuildResetData (
  IN OUT   UINTN   *ResetDataSize,
  IN OUT   VOID    *ResetData,
  IN CONST GUID    *ResetSubtype  OPTIONAL,
  IN CONST CHAR16  *ResetString   OPTIONAL,
  IN       UINTN   ExtraDataSize  OPTIONAL,
  IN CONST VOID    *ExtraData     OPTIONAL
  )
{
  UINTN  ResetStringSize;
  UINTN  ResetDataBufferSize;
  UINT8  *Data;

  //
  // If the size return pointer is NULL.
  //
  if (ResetDataSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // If extra data is indicated, but pointer is NULL.
  //
  if ((ExtraDataSize > 0) && (ExtraData == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // If extra data is indicated, but no subtype GUID is supplied.
  //
  if ((ResetSubtype == NULL) && (ExtraDataSize > 0)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Determine the final string.
  //
  if (ResetString == NULL) {
    ResetString = L"";     // Use an empty string.
  }

  //
  // Calculate the total buffer required for ResetData.
  //
  ResetStringSize     = StrnSizeS (ResetString, MAX_UINT16);
  ResetDataBufferSize = ResetStringSize + ExtraDataSize;
  if (ResetSubtype != NULL) {
    ResetDataBufferSize += sizeof (GUID);
  }

  //
  // At this point, if the buffer isn't large enough (or if
  // the buffer is NULL) we cannot proceed.
  //
  if (*ResetDataSize < ResetDataBufferSize) {
    *ResetDataSize = ResetDataBufferSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  *ResetDataSize = ResetDataBufferSize;
  if (ResetData == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Fill in ResetData with ResetString, the ResetSubtype GUID, and extra data
  //
  Data = (UINT8 *)ResetData;
  CopyMem (Data, ResetString, ResetStringSize);
  Data += ResetStringSize;
  if (ResetSubtype != NULL) {
    CopyMem (Data, ResetSubtype, sizeof (GUID));
    Data += sizeof (GUID);
  }

  if (ExtraDataSize > 0) {
    CopyMem (Data, ExtraData, ExtraDataSize);
  }

  return RETURN_SUCCESS;
}
