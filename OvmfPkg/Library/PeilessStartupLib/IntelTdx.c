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
#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/TpmMeasurementLib.h>

#include "PeilessStartupInternal.h"

#pragma pack(1)

#define HANDOFF_TABLE_DESC  "TdxTable"
typedef struct {
  UINT8                      TableDescriptionSize;
  UINT8                      TableDescription[sizeof (HANDOFF_TABLE_DESC)];
  UINT64                     NumberOfTables;
  EFI_CONFIGURATION_TABLE    TableEntry[1];
} TDX_HANDOFF_TABLE_POINTERS2;

#define FV_HANDOFF_TABLE_DESC  "Fv(XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)"
typedef struct {
  UINT8                   BlobDescriptionSize;
  UINT8                   BlobDescription[sizeof (FV_HANDOFF_TABLE_DESC)];
  EFI_PHYSICAL_ADDRESS    BlobBase;
  UINT64                  BlobLength;
} FV_HANDOFF_TABLE_POINTERS2;

#pragma pack()

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

/**
  Measure the Hoblist passed from the VMM.

  @param[in] VmmHobList    The Hoblist pass the firmware

  @retval EFI_SUCCESS           Fv image is measured successfully
                                or it has been already measured.
  @retval Others                Other errors as indicated
**/
EFI_STATUS
EFIAPI
MeasureHobList (
  IN CONST VOID  *VmmHobList
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  TDX_HANDOFF_TABLE_POINTERS2  HandoffTables;
  EFI_STATUS                   Status;

  if (!TdIsEnabled ()) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  Hob.Raw = (UINT8 *)VmmHobList;

  //
  // Parse the HOB list until end of list.
  //
  while (!END_OF_HOB_LIST (Hob)) {
    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  //
  // Init the log event for HOB measurement
  //

  HandoffTables.TableDescriptionSize = sizeof (HandoffTables.TableDescription);
  CopyMem (HandoffTables.TableDescription, HANDOFF_TABLE_DESC, sizeof (HandoffTables.TableDescription));
  HandoffTables.NumberOfTables = 1;
  CopyGuid (&(HandoffTables.TableEntry[0].VendorGuid), &gUefiOvmfPkgTokenSpaceGuid);
  HandoffTables.TableEntry[0].VendorTable = (VOID *)VmmHobList;

  Status = TpmMeasureAndLogData (
             1,                                              // PCRIndex
             EV_EFI_HANDOFF_TABLES2,                         // EventType
             (VOID *)&HandoffTables,                         // EventData
             sizeof (HandoffTables),                         // EventSize
             (UINT8 *)(UINTN)VmmHobList,                     // HashData
             (UINTN)((UINT8 *)Hob.Raw - (UINT8 *)VmmHobList) // HashDataLen
             );

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
  }

  return Status;
}

/**
  Get the FvName from the FV header.

  Causion: The FV is untrusted input.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.

  @return FvName pointer
  @retval NULL   FvName is NOT found
**/
VOID *
GetFvName (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;

  if (FvBase >= MAX_ADDRESS) {
    return NULL;
  }

  if (FvLength >= MAX_ADDRESS - FvBase) {
    return NULL;
  }

  if (FvLength < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    return NULL;
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvBase;
  if (FvHeader->ExtHeaderOffset < sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    return NULL;
  }

  if (FvHeader->ExtHeaderOffset + sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER) > FvLength) {
    return NULL;
  }

  FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(UINTN)(FvBase + FvHeader->ExtHeaderOffset);

  return &FvExtHeader->FvName;
}

/**
  Measure FV image.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.
  @param[in]  PcrIndex          Index of PCR

  @retval EFI_SUCCESS           Fv image is measured successfully
                                or it has been already measured.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength,
  IN UINT8                 PcrIndex
  )
{
  EFI_STATUS                  Status;
  FV_HANDOFF_TABLE_POINTERS2  FvBlob2;
  VOID                        *FvName;

  //
  // Init the log event for FV measurement
  //
  FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
  CopyMem (FvBlob2.BlobDescription, FV_HANDOFF_TABLE_DESC, sizeof (FvBlob2.BlobDescription));
  FvName = GetFvName (FvBase, FvLength);
  if (FvName != NULL) {
    AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
  }

  FvBlob2.BlobBase   = FvBase;
  FvBlob2.BlobLength = FvLength;

  Status = TpmMeasureAndLogData (
             1,                              // PCRIndex
             EV_EFI_PLATFORM_FIRMWARE_BLOB2, // EventType
             (VOID *)&FvBlob2,               // EventData
             sizeof (FvBlob2),               // EventSize
             (UINT8 *)(UINTN)FvBase,         // HashData
             (UINTN)(FvLength)               // HashDataLen
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "The FV which failed to be measured starts at: 0x%x\n", FvBase));
    ASSERT (FALSE);
  }

  return Status;
}
