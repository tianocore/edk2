/** @file
  This library is used by FSP modules to measure data to TPM.

Copyright (c) 2020, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/FspMeasurementLib.h>
#include <Library/TcgEventLogRecordLib.h>
#include <Library/HashLib.h>

#include <Ppi/Tcg.h>
#include <IndustryStandard/UefiTcgPlatform.h>

/**
  Tpm measure and log data, and extend the measurement result into a specific PCR.

  @param[in]  PcrIndex         PCR Index.
  @param[in]  EventType        Event type.
  @param[in]  EventLog         Measurement event log.
  @param[in]  LogLen           Event log length in bytes.
  @param[in]  HashData         The start of the data buffer to be hashed, extended.
  @param[in]  HashDataLen      The length, in bytes, of the buffer referenced by HashData
  @param[in]  Flags            Bitmap providing additional information.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
**/
EFI_STATUS
EFIAPI
TpmMeasureAndLogDataWithFlags (
  IN UINT32  PcrIndex,
  IN UINT32  EventType,
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen,
  IN UINT64  Flags
  )
{
  EFI_STATUS         Status;
  EDKII_TCG_PPI      *TcgPpi;
  TCG_PCR_EVENT_HDR  TcgEventHdr;

  Status = PeiServicesLocatePpi (
             &gEdkiiTcgPpiGuid,
             0,
             NULL,
             (VOID **)&TcgPpi
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TcgEventHdr.PCRIndex  = PcrIndex;
  TcgEventHdr.EventType = EventType;
  TcgEventHdr.EventSize = LogLen;

  Status = TcgPpi->HashLogExtendEvent (
                     TcgPpi,
                     Flags,
                     HashData,
                     (UINTN)HashDataLen,
                     &TcgEventHdr,
                     EventLog
                     );
  return Status;
}

/**
  Measure a FSP FirmwareBlob.

  @param[in]  Description             Description for this FirmwareBlob.
  @param[in]  FirmwareBlobBase        Base address of this FirmwareBlob.
  @param[in]  FirmwareBlobLength      Size in bytes of this FirmwareBlob.
  @param[in]  CfgRegionOffset         Configuration region offset in bytes.
  @param[in]  CfgRegionSize           Configuration region in bytes.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
**/
STATIC
EFI_STATUS
EFIAPI
MeasureFspFirmwareBlobWithCfg (
  IN CHAR8                 *Description OPTIONAL,
  IN EFI_PHYSICAL_ADDRESS  FirmwareBlobBase,
  IN UINT64                FirmwareBlobLength,
  IN UINT32                CfgRegionOffset,
  IN UINT32                CfgRegionSize
  )
{
  EFI_PLATFORM_FIRMWARE_BLOB      FvBlob, UpdBlob;
  PLATFORM_FIRMWARE_BLOB2_STRUCT  FvBlob2, UpdBlob2;
  VOID                            *FvName;
  UINT32                          FvEventType;
  VOID                            *FvEventLog, *UpdEventLog;
  UINT32                          FvEventLogSize, UpdEventLogSize;
  EFI_STATUS                      Status;
  HASH_HANDLE                     HashHandle;
  UINT8                           *HashBase;
  UINTN                           HashSize;
  TPML_DIGEST_VALUES              DigestList;

  FvName = TpmMeasurementGetFvName (FirmwareBlobBase, FirmwareBlobLength);

  if (((Description != NULL) || (FvName != NULL)) &&
      (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105))
  {
    if (Description != NULL) {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "%a", Description);
      AsciiSPrint ((CHAR8 *)UpdBlob2.BlobDescription, sizeof (UpdBlob2.BlobDescription), "%aUDP", Description);
    } else {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
      AsciiSPrint ((CHAR8 *)UpdBlob2.BlobDescription, sizeof (UpdBlob2.BlobDescription), "(%g)UDP", FvName);
    }

    FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
    FvBlob2.BlobBase            = FirmwareBlobBase;
    FvBlob2.BlobLength          = FirmwareBlobLength;
    FvEventType                 = EV_EFI_PLATFORM_FIRMWARE_BLOB2;
    FvEventLog                  = &FvBlob2;
    FvEventLogSize              = sizeof (FvBlob2);

    UpdBlob2.BlobDescriptionSize = sizeof (UpdBlob2.BlobDescription);
    UpdBlob2.BlobBase            = CfgRegionOffset;
    UpdBlob2.BlobLength          = CfgRegionSize;
    UpdEventLog                  = &UpdBlob2;
    UpdEventLogSize              = sizeof (UpdBlob2);
  } else {
    FvBlob.BlobBase   = FirmwareBlobBase;
    FvBlob.BlobLength = FirmwareBlobLength;
    FvEventType       = EV_EFI_PLATFORM_FIRMWARE_BLOB;
    FvEventLog        = &FvBlob;
    FvEventLogSize    = sizeof (FvBlob);

    UpdBlob.BlobBase   = CfgRegionOffset;
    UpdBlob.BlobLength = CfgRegionSize;
    UpdEventLog        = &UpdBlob;
    UpdEventLogSize    = sizeof (UpdBlob);
  }

  /** Initialize a SHA hash context. **/
  Status = HashStart (&HashHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HashStart failed - %r\n", Status));
    return Status;
  }

  /** Hash FSP binary before UDP **/
  HashBase = (UINT8 *)(UINTN)FirmwareBlobBase;
  HashSize = (UINTN)CfgRegionOffset;
  Status   = HashUpdate (HashHandle, HashBase, HashSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HashUpdate failed - %r\n", Status));
    return Status;
  }

  /** Hash FSP binary after UDP **/
  HashBase = (UINT8 *)(UINTN)FirmwareBlobBase + CfgRegionOffset + CfgRegionSize;
  HashSize = (UINTN)(FirmwareBlobLength - CfgRegionOffset - CfgRegionSize);
  Status   = HashUpdate (HashHandle, HashBase, HashSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HashUpdate failed - %r\n", Status));
    return Status;
  }

  /** Finalize the SHA hash. **/
  Status = HashCompleteAndExtend (HashHandle, 0, NULL, 0, &DigestList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HashCompleteAndExtend failed - %r\n", Status));
    return Status;
  }

  Status = TpmMeasureAndLogDataWithFlags (
             0,
             FvEventType,
             FvEventLog,
             FvEventLogSize,
             (UINT8 *)&DigestList,
             (UINTN)sizeof (DigestList),
             EDKII_TCG_PRE_HASH_LOG_ONLY
             );

  Status = TpmMeasureAndLogData (
             1,
             EV_PLATFORM_CONFIG_FLAGS,
             UpdEventLog,
             UpdEventLogSize,
             (UINT8 *)(UINTN)FirmwareBlobBase + CfgRegionOffset,
             CfgRegionSize
             );

  return Status;
}

/**
  Measure a FSP FirmwareBlob.

  @param[in]  PcrIndex                PCR Index.
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
MeasureFspFirmwareBlob (
  IN UINT32                PcrIndex,
  IN CHAR8                 *Description OPTIONAL,
  IN EFI_PHYSICAL_ADDRESS  FirmwareBlobBase,
  IN UINT64                FirmwareBlobLength
  )
{
  UINT32           FspMeasureMask;
  FSP_INFO_HEADER  *FspHeaderPtr;

  FspMeasureMask = PcdGet32 (PcdFspMeasurementConfig);
  if ((FspMeasureMask & FSP_MEASURE_FSPUPD) != 0) {
    FspHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader (FirmwareBlobBase);
    if (FspHeaderPtr != NULL) {
      return MeasureFspFirmwareBlobWithCfg (
               Description,
               FirmwareBlobBase,
               FirmwareBlobLength,
               FspHeaderPtr->CfgRegionOffset,
               FspHeaderPtr->CfgRegionSize
               );
    }
  }

  return MeasureFirmwareBlob (PcrIndex, Description, FirmwareBlobBase, FirmwareBlobLength);
}
