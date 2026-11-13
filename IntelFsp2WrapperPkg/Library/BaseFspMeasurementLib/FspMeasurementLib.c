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
#include <Library/HobLib.h>

#include <Guid/FspImageBaseHob.h>
#include <Ppi/Tcg.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#define FSP_NO_IMAGE_BASE_OFFSET  MAX_UINTN

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
  @param[in]  ImageBaseOffset         Offset in bytes of the ImageBase field to
                                      replace, or FSP_NO_IMAGE_BASE_OFFSET.
  @param[in]  OriginalImageBase       Value hashed in place of the ImageBase
                                      field when ImageBaseOffset is used.

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
  IN UINT32                CfgRegionSize,
  IN UINTN                 ImageBaseOffset,
  IN UINT32                OriginalImageBase
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
      AsciiSPrint ((CHAR8 *)UpdBlob2.BlobDescription, sizeof (UpdBlob2.BlobDescription), "%aUPD", Description);
    } else {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
      AsciiSPrint ((CHAR8 *)UpdBlob2.BlobDescription, sizeof (UpdBlob2.BlobDescription), "(%g)UPD", FvName);
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

  //
  // The FSP specification places the FSP_INFO_HEADER before the UPD region.
  // Hash ImageBase as is if it is not inside the blob before the UPD region.
  //
  if ((ImageBaseOffset > MAX_UINTN - sizeof (UINT32)) ||
      (ImageBaseOffset + sizeof (UINT32) > FirmwareBlobLength) ||
      (ImageBaseOffset + sizeof (UINT32) > CfgRegionOffset))
  {
    ImageBaseOffset = FSP_NO_IMAGE_BASE_OFFSET;
  }

  if (ImageBaseOffset == FSP_NO_IMAGE_BASE_OFFSET) {
    /** Hash FSP binary before UPD **/
    HashBase = (UINT8 *)(UINTN)FirmwareBlobBase;
    HashSize = (UINTN)CfgRegionOffset;
    Status   = HashUpdate (HashHandle, HashBase, HashSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HashUpdate failed - %r\n", Status));
      return Status;
    }
  } else {
    /** Hash FSP binary before ImageBase **/
    HashBase = (UINT8 *)(UINTN)FirmwareBlobBase;
    HashSize = ImageBaseOffset;
    Status   = HashUpdate (HashHandle, HashBase, HashSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HashUpdate failed - %r\n", Status));
      return Status;
    }

    /** Hash the original ImageBase in place of the current one **/
    Status = HashUpdate (HashHandle, &OriginalImageBase, sizeof (OriginalImageBase));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HashUpdate failed - %r\n", Status));
      return Status;
    }

    /** Hash FSP binary after ImageBase, before UPD **/
    HashBase = (UINT8 *)(UINTN)FirmwareBlobBase + ImageBaseOffset + sizeof (UINT32);
    HashSize = (UINTN)CfgRegionOffset - ImageBaseOffset - sizeof (UINT32);
    Status   = HashUpdate (HashHandle, HashBase, HashSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HashUpdate failed - %r\n", Status));
      return Status;
    }
  }

  /** Hash FSP binary after UPD **/
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
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "TpmMeasureAndLogDataWithFlags failed - %r\n", Status));
    return Status;
  }

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
  Find the ImageBase value an FSP component had before its wrapper wrote
  the load address into FSP_INFO_HEADER.ImageBase.

  @param[in]  FspBase                 Base address of the FSP component.
  @param[out] OriginalImageBase       The saved ImageBase value.

  @retval TRUE   One FSP_IMAGE_BASE_HOB value was found for FspBase.
  @retval FALSE  No HOB was found, or the HOBs for FspBase disagree.
**/
STATIC
BOOLEAN
FspGetOriginalImageBase (
  IN  EFI_PHYSICAL_ADDRESS  FspBase,
  OUT UINT32                *OriginalImageBase
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  FSP_IMAGE_BASE_HOB  *ImageBaseHob;
  BOOLEAN             Found;

  Found = FALSE;
  for (GuidHob = GetFirstGuidHob (&gFspImageBaseHobGuid);
       GuidHob != NULL;
       GuidHob = GetNextGuidHob (&gFspImageBaseHobGuid, GET_NEXT_HOB (GuidHob)))
  {
    if (GET_GUID_HOB_DATA_SIZE (GuidHob) < sizeof (FSP_IMAGE_BASE_HOB)) {
      continue;
    }

    ImageBaseHob = GET_GUID_HOB_DATA (GuidHob);
    if ((EFI_PHYSICAL_ADDRESS)ImageBaseHob->FspBase != FspBase) {
      continue;
    }

    if (Found && (*OriginalImageBase != ImageBaseHob->OriginalImageBase)) {
      DEBUG ((DEBUG_ERROR, "Conflicting ImageBase HOBs for FSP at 0x%lx\n", FspBase));
      return FALSE;
    }

    *OriginalImageBase = ImageBaseHob->OriginalImageBase;
    Found              = TRUE;
  }

  return Found;
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
  UINTN            ImageBaseOffset;
  UINT32           OriginalImageBase;

  FspMeasureMask = PcdGet32 (PcdFspMeasurementConfig);
  if ((FspMeasureMask & FSP_MEASURE_FSPUPD) != 0) {
    FspHeaderPtr = (FSP_INFO_HEADER *)FspFindFspHeader (FirmwareBlobBase);
    if (FspHeaderPtr != NULL) {
      ImageBaseOffset   = FSP_NO_IMAGE_BASE_OFFSET;
      OriginalImageBase = 0;
      if (((FspMeasureMask & FSP_MEASURE_ORIGINAL_IMAGEBASE) != 0) &&
          (FspHeaderPtr->ImageBase == FirmwareBlobBase) &&
          FspGetOriginalImageBase (FirmwareBlobBase, &OriginalImageBase))
      {
        ImageBaseOffset = (UINTN)FspHeaderPtr - (UINTN)FirmwareBlobBase + OFFSET_OF (FSP_INFO_HEADER, ImageBase);
      }

      return MeasureFspFirmwareBlobWithCfg (
               Description,
               FirmwareBlobBase,
               FirmwareBlobLength,
               FspHeaderPtr->CfgRegionOffset,
               FspHeaderPtr->CfgRegionSize,
               ImageBaseOffset,
               OriginalImageBase
               );
    }
  }

  return MeasureFirmwareBlob (PcrIndex, Description, FirmwareBlobBase, FirmwareBlobLength);
}
