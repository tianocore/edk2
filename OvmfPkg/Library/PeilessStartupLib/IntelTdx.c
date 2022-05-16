/** @file
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include "PeilessStartupInternal.h"

/**
  Check padding data all bit should be 1.

  @param[in] Buffer     - A pointer to buffer header
  @param[in] BufferSize - Buffer size

  @retval  TRUE   - The padding data is valid.
  @retval  TRUE  - The padding data is invalid.

**/
BOOLEAN
CheckPaddingData (
  IN UINT8   *Buffer,
  IN UINT32  BufferSize
  )
{
  UINT32  index;

  for (index = 0; index < BufferSize; index++) {
    if (Buffer[index] != 0xFF) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Check the integrity of CFV data.

  @param[in] TdxCfvBase - A pointer to CFV header
  @param[in] TdxCfvSize - CFV data size

  @retval  TRUE   - The CFV data is valid.
  @retval  FALSE  - The CFV data is invalid.

**/
BOOLEAN
EFIAPI
TdxValidateCfv (
  IN UINT8   *TdxCfvBase,
  IN UINT32  TdxCfvSize
  )
{
  UINT16                         Checksum;
  UINTN                          VariableBase;
  UINT32                         VariableOffset;
  UINT32                         VariableOffsetBeforeAlign;
  EFI_FIRMWARE_VOLUME_HEADER     *CfvFvHeader;
  VARIABLE_STORE_HEADER          *CfvVariableStoreHeader;
  AUTHENTICATED_VARIABLE_HEADER  *VariableHeader;

  static EFI_GUID  FvHdrGUID       = EFI_SYSTEM_NV_DATA_FV_GUID;
  static EFI_GUID  VarStoreHdrGUID = EFI_AUTHENTICATED_VARIABLE_GUID;

  VariableOffset = 0;

  if (TdxCfvBase == NULL) {
    DEBUG ((DEBUG_ERROR, "TDX CFV: CFV pointer is NULL\n"));
    return FALSE;
  }

  //
  // Verify the header zerovetor, filesystemguid,
  // revision, signature, attributes, fvlength, checksum
  // HeaderLength cannot be an odd number
  //
  CfvFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)TdxCfvBase;

  if ((!IsZeroBuffer (CfvFvHeader->ZeroVector, 16)) ||
      (!CompareGuid (&FvHdrGUID, &CfvFvHeader->FileSystemGuid)) ||
      (CfvFvHeader->Signature != EFI_FVH_SIGNATURE) ||
      (CfvFvHeader->Attributes != 0x4feff) ||
      (CfvFvHeader->Revision != EFI_FVH_REVISION) ||
      (CfvFvHeader->FvLength != TdxCfvSize)
      )
  {
    DEBUG ((DEBUG_ERROR, "TDX CFV: Basic FV headers were invalid\n"));
    return FALSE;
  }

  //
  // Verify the header checksum
  //
  Checksum = CalculateSum16 ((VOID *)CfvFvHeader, CfvFvHeader->HeaderLength);

  if (Checksum != 0) {
    DEBUG ((DEBUG_ERROR, "TDX CFV: FV checksum was invalid\n"));
    return FALSE;
  }

  //
  // Verify the header signature, size, format, state
  //
  CfvVariableStoreHeader = (VARIABLE_STORE_HEADER *)(TdxCfvBase + CfvFvHeader->HeaderLength);
  if ((!CompareGuid (&VarStoreHdrGUID, &CfvVariableStoreHeader->Signature)) ||
      (CfvVariableStoreHeader->Format != VARIABLE_STORE_FORMATTED) ||
      (CfvVariableStoreHeader->State != VARIABLE_STORE_HEALTHY) ||
      (CfvVariableStoreHeader->Size > (CfvFvHeader->FvLength - CfvFvHeader->HeaderLength)) ||
      (CfvVariableStoreHeader->Size < sizeof (VARIABLE_STORE_HEADER))
      )
  {
    DEBUG ((DEBUG_ERROR, "TDX CFV: Variable Store header was invalid\n"));
    return FALSE;
  }

  //
  // Verify the header startId, state
  // Verify data to the end
  //
  VariableBase = (UINTN)TdxCfvBase + CfvFvHeader->HeaderLength + sizeof (VARIABLE_STORE_HEADER);
  while (VariableOffset  < (CfvVariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER))) {
    VariableHeader = (AUTHENTICATED_VARIABLE_HEADER *)(VariableBase + VariableOffset);
    if (VariableHeader->StartId != VARIABLE_DATA) {
      if (!CheckPaddingData ((UINT8 *)VariableHeader, CfvVariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER) - VariableOffset)) {
        DEBUG ((DEBUG_ERROR, "TDX CFV: Variable header was invalid\n"));
        return FALSE;
      }

      VariableOffset = CfvVariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);
    } else {
      if (!((VariableHeader->State == VAR_IN_DELETED_TRANSITION) ||
            (VariableHeader->State == VAR_DELETED) ||
            (VariableHeader->State == VAR_HEADER_VALID_ONLY) ||
            (VariableHeader->State == VAR_ADDED)))
      {
        DEBUG ((DEBUG_ERROR, "TDX CFV: Variable header was invalid\n"));
        return FALSE;
      }

      VariableOffset += sizeof (AUTHENTICATED_VARIABLE_HEADER) + VariableHeader->NameSize + VariableHeader->DataSize;
      // Verify VariableOffset should be less than or equal CfvVariableStoreHeader->Size - sizeof(VARIABLE_STORE_HEADER)
      if (VariableOffset > (CfvVariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER))) {
        DEBUG ((DEBUG_ERROR, "TDX CFV: Variable header was invalid\n"));
        return FALSE;
      }

      VariableOffsetBeforeAlign = VariableOffset;
      // 4 byte align
      VariableOffset = (VariableOffset  + 3) & (UINTN)(~3);

      if (!CheckPaddingData ((UINT8 *)(VariableBase + VariableOffsetBeforeAlign), VariableOffset - VariableOffsetBeforeAlign)) {
        DEBUG ((DEBUG_ERROR, "TDX CFV: Variable header was invalid\n"));
        return FALSE;
      }
    }
  }

  return TRUE;
}
