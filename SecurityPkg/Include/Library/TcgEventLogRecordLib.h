/** @file
  This library is used by other modules to measure Firmware to TPM.

Copyright (c) 2020, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TCG_EVENTLOGRECORD_LIB_H_
#define _TCG_EVENTLOGRECORD_LIB_H_

#include <Uefi.h>

#pragma pack (1)

#define PLATFORM_FIRMWARE_BLOB_DESC "Fv(XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)"
typedef struct {
  UINT8                             BlobDescriptionSize;
  UINT8                             BlobDescription[sizeof(PLATFORM_FIRMWARE_BLOB_DESC)];
  EFI_PHYSICAL_ADDRESS              BlobBase;
  UINT64                            BlobLength;
} PLATFORM_FIRMWARE_BLOB2_STRUCT;

#define HANDOFF_TABLE_POINTER_DESC  "1234567890ABCDEF"
typedef struct {
  UINT8                             TableDescriptionSize;
  UINT8                             TableDescription[sizeof(HANDOFF_TABLE_POINTER_DESC)];
  UINT64                            NumberOfTables;
  EFI_CONFIGURATION_TABLE           TableEntry[1];
} HANDOFF_TABLE_POINTERS2_STRUCT;

#pragma pack ()

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
  IN EFI_PHYSICAL_ADDRESS           FvBase,
  IN UINT64                         FvLength
  );

/**
  Measure a FirmwareBlob.

  @param[in]  PcrIndex                PCR Index.
  @param[in]  Description             Description for this FirmwareBlob.
  @param[in]  FirmwareBlobBase        Base address of this FirmwareBlob.
  @param[in]  FirmwareBlobLength      Size in bytes of this FirmwareBlob.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
*/
EFI_STATUS
EFIAPI
MeasureFirmwareBlob (
  IN UINT32                         PcrIndex,
  IN CHAR8                          *Description OPTIONAL,
  IN EFI_PHYSICAL_ADDRESS           FirmwareBlobBase,
  IN UINT64                         FirmwareBlobLength
  );

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
*/
EFI_STATUS
EFIAPI
MeasureHandoffTable (
  IN UINT32                         PcrIndex,
  IN CHAR8                          *Description OPTIONAL,
  IN EFI_GUID                       *TableGuid,
  IN VOID                           *TableAddress,
  IN UINTN                          TableLength
  );

#endif
