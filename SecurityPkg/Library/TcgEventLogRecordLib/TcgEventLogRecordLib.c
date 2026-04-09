/** @file
  This library is used by other modules to measure data to TPM.

Copyright (c) 2020, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Pi/PiFirmwareVolume.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/TcgEventLogRecordLib.h>
#include <Library/TpmMeasurementLib.h>

#include <IndustryStandard/UefiTcgPlatform.h>

/**
  Get the FvName from the FV header.

  Causion: The FV is untrusted input.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.

  @return FvName pointer
  @retval NULL   FvName is NOT found
**/
VOID *
TpmMeasurementGetFvName (
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
  if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
    return NULL;
  }

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
  Measure a FirmwareBlob.

  @param[in]  PcrIndex                PcrIndex of the measurement.
  @param[in]  Description             Description for this FirmwareBlob.
  @param[in]  FirmwareBlobBase        Base address of this FirmwareBlob.
  @param[in]  FirmwareBlobLength      Size in bytes of this FirmwareBlob.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
**/
EFI_STATUS
EFIAPI
MeasureFirmwareBlob (
  IN UINT32                PcrIndex,
  IN CHAR8                 *Description OPTIONAL,
  IN EFI_PHYSICAL_ADDRESS  FirmwareBlobBase,
  IN UINT64                FirmwareBlobLength
  )
{
  EFI_PLATFORM_FIRMWARE_BLOB      FvBlob;
  PLATFORM_FIRMWARE_BLOB2_STRUCT  FvBlob2;
  VOID                            *FvName;
  UINT32                          EventType;
  VOID                            *EventLog;
  UINT32                          EventLogSize;
  EFI_STATUS                      Status;

  FvName = TpmMeasurementGetFvName (FirmwareBlobBase, FirmwareBlobLength);

  if (((Description != NULL) || (FvName != NULL)) &&
      (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105))
  {
    if (Description != NULL) {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "%a", Description);
    } else {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
    }

    FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
    FvBlob2.BlobBase            = FirmwareBlobBase;
    FvBlob2.BlobLength          = FirmwareBlobLength;

    EventType    = EV_EFI_PLATFORM_FIRMWARE_BLOB2;
    EventLog     = &FvBlob2;
    EventLogSize = sizeof (FvBlob2);
  } else {
    FvBlob.BlobBase   = FirmwareBlobBase;
    FvBlob.BlobLength = FirmwareBlobLength;

    EventType    = EV_EFI_PLATFORM_FIRMWARE_BLOB;
    EventLog     = &FvBlob;
    EventLogSize = sizeof (FvBlob);
  }

  Status = TpmMeasureAndLogData (
             PcrIndex,
             EventType,
             EventLog,
             EventLogSize,
             (VOID *)(UINTN)FirmwareBlobBase,
             FirmwareBlobLength
             );

  return Status;
}

/**
  Measure a HandoffTable.

  @param[in]  PcrIndex                PcrIndex of the measurement.
  @param[in]  Description             Description for this HandoffTable.
  @param[in]  TableGuid               GUID of this HandoffTable.
  @param[in]  TableAddress            Base address of this HandoffTable.
  @param[in]  TableLength             Size in bytes of this HandoffTable.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
**/
EFI_STATUS
EFIAPI
MeasureHandoffTable (
  IN UINT32    PcrIndex,
  IN CHAR8     *Description OPTIONAL,
  IN EFI_GUID  *TableGuid,
  IN VOID      *TableAddress,
  IN UINTN     TableLength
  )
{
  EFI_HANDOFF_TABLE_POINTERS      HandoffTables;
  HANDOFF_TABLE_POINTERS2_STRUCT  HandoffTables2;
  UINT32                          EventType;
  VOID                            *EventLog;
  UINT32                          EventLogSize;
  EFI_STATUS                      Status;

  if ((Description != NULL) &&
      (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105))
  {
    AsciiSPrint ((CHAR8 *)HandoffTables2.TableDescription, sizeof (HandoffTables2.TableDescription), "%a", Description);

    HandoffTables2.TableDescriptionSize = sizeof (HandoffTables2.TableDescription);
    HandoffTables2.NumberOfTables       = 1;
    CopyGuid (&(HandoffTables2.TableEntry[0].VendorGuid), TableGuid);
    HandoffTables2.TableEntry[0].VendorTable = TableAddress;

    EventType    = EV_EFI_HANDOFF_TABLES2;
    EventLog     = &HandoffTables2;
    EventLogSize = sizeof (HandoffTables2);
  } else {
    HandoffTables.NumberOfTables = 1;
    CopyGuid (&(HandoffTables.TableEntry[0].VendorGuid), TableGuid);
    HandoffTables.TableEntry[0].VendorTable = TableAddress;

    EventType    = EV_EFI_HANDOFF_TABLES;
    EventLog     = &HandoffTables;
    EventLogSize = sizeof (HandoffTables);
  }

  Status = TpmMeasureAndLogData (
             PcrIndex,
             EventType,
             EventLog,
             EventLogSize,
             TableAddress,
             TableLength
             );
  return Status;
}
