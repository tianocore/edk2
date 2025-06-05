/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Guid/TcgEventHob.h>
#include <Guid/TpmInstance.h>

#include <IndustryStandard/UefiTcgPlatform.h>

#include <Library/ArmTransferListLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <Ppi/FirmwareVolume.h>

#pragma pack (1)

/*
 * Firmware volume description format which is the same to Tcg2Pei.
 */
#define FV_HANDOFF_TABLE_DESC  "Fv(XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)"

/*
 * TCG PC Client Platform Firmware Profile Specification:
 *   10.2.5 UEFI_PLATFORM_FIRMWARE_BLOB2.
 */
typedef struct {
  UINT8                   BlobDescriptionSize;
  UINT8                   BlobDescription[sizeof (FV_HANDOFF_TABLE_DESC)];
  EFI_PHYSICAL_ADDRESS    BlobBase;
  UINT64                  BlobLength;
} FV_HANDOFF_TABLE_POINTERS2;

#pragma pack ()

/**
  Add a new entry to the Event Log.

  @param[in]     DigestList    A list of digest.
  @param[in,out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]     NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
**/
STATIC
EFI_STATUS
EFIAPI
LogHashEvent (
  IN TPML_DIGEST_VALUES      *DigestList,
  IN OUT  TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  VOID            *HobData;
  TCG_PCR_EVENT2  *TcgPcrEvent2;
  UINT8           *DigestBuffer;

  //
  // Use GetDigestListSize (DigestList) in the GUID HOB DataLength calculation
  // to reserve enough buffer to hold TPML_DIGEST_VALUES compact binary.
  //
  HobData = BuildGuidHob (
              &gTcgEvent2EntryHobGuid,
              sizeof (TcgPcrEvent2->PCRIndex) + sizeof (TcgPcrEvent2->EventType) + GetDigestListSize (DigestList) + sizeof (TcgPcrEvent2->EventSize) + NewEventHdr->EventSize
              );
  if (HobData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TcgPcrEvent2            = HobData;
  TcgPcrEvent2->PCRIndex  = NewEventHdr->PCRIndex;
  TcgPcrEvent2->EventType = NewEventHdr->EventType;
  DigestBuffer            = (UINT8 *)&TcgPcrEvent2->Digest;

  /*
   * TF-A support one digest hash among below hash algo list:
   *     - TPM_ALG_SHA256 (default)
   *     - TPM_ALG_SHA384
   *     - TPM_ALG_SHA512
   */
  DigestBuffer = CopyDigestListToBuffer (
                   DigestBuffer,
                   DigestList,
                   TPM_ALG_SHA256 | TPM_ALG_SHA384 | TPM_ALG_SHA512
                   );
  CopyMem (DigestBuffer, &NewEventHdr->EventSize, sizeof (TcgPcrEvent2->EventSize));
  DigestBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);
  CopyMem (DigestBuffer, NewEventData, NewEventHdr->EventSize);

  return EFI_SUCCESS;
}

/**
  Do a hash operation on a data buffer, extend a specific TPM PCR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      HashData      If BIT0 of Flags is 0, it is physical address of the
                                start of the data buffer to be hashed, extended, and logged.
                                If BIT0 of Flags is 1, it is physical address of the
                                start of the pre-hash data buffer to be extended, and logged.
                                The pre-hash data format is TPML_DIGEST_VALUES.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.
  @retval EFI_UNSUPPORTED       TPM device is disabled.

**/
STATIC
EFI_STATUS
EFIAPI
HashLogExtendEvent (
  IN      UINT8              *HashData,
  IN      UINTN              HashDataLen,
  IN      TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  EFI_STATUS          Status;
  TPML_DIGEST_VALUES  DigestList;

  if (GetFirstGuidHob (&gTpmErrorHobGuid) != NULL) {
    return EFI_UNSUPPORTED;
  }

  Status = HashAndExtend (
             NewEventHdr->PCRIndex,
             HashData,
             HashDataLen,
             &DigestList
             );
  if (!EFI_ERROR (Status)) {
    Status = LogHashEvent (&DigestList, NewEventHdr, NewEventData);
  } else {
    DEBUG ((DEBUG_ERROR, "%a: %r. Disable TPM.\n", __func__, Status));
    BuildGuidHob (&gTpmErrorHobGuid, 0);
  }

  return Status;
}

/**
  Measure FV image.
  Add it into the measured FV list after the FV is measured successfully.

  @param[in]  FvBase            Base address of FV image.
  @param[in]  FvLength          Length of FV image.

  @retval EFI_SUCCESS           Fv image is measured successfully
                                or it has been already measured.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureFvImage (
  IN EFI_PHYSICAL_ADDRESS  FvBase,
  IN UINT64                FvLength
  )
{
  EFI_STATUS                      Status;
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;
  TCG_PCR_EVENT_HDR               TcgEventHdr;
  EFI_PLATFORM_FIRMWARE_BLOB      FvBlob;
  FV_HANDOFF_TABLE_POINTERS2      FvBlob2;
  VOID                            *EventData;
  VOID                            *FvName;

  FvName = NULL;

  if ((FvBase >= MAX_ADDRESS) ||
      (FvLength >= MAX_ADDRESS - FvBase) ||
      (FvLength < sizeof (EFI_FIRMWARE_VOLUME_HEADER)))
  {
    return EFI_INVALID_PARAMETER;
  }

  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvBase;
  if (FvHeader->ExtHeaderOffset >= sizeof (EFI_FIRMWARE_VOLUME_HEADER)) {
    if (FvHeader->ExtHeaderOffset + sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER) > FvLength) {
      return EFI_INVALID_PARAMETER;
    }

    FvExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(UINTN)(FvBase + FvHeader->ExtHeaderOffset);
    FvName      = &FvExtHeader->FvName;
  }

  //
  // Init the log event for FV measurement
  //
  if (PcdGet32 (PcdTcgPfpMeasurementRevision) >= TCG_EfiSpecIDEventStruct_SPEC_ERRATA_TPM2_REV_105) {
    FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
    CopyMem (FvBlob2.BlobDescription, FV_HANDOFF_TABLE_DESC, sizeof (FvBlob2.BlobDescription));
    if (FvName != NULL) {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
    }

    FvBlob2.BlobBase      = FvBase;
    FvBlob2.BlobLength    = FvLength;
    TcgEventHdr.PCRIndex  = 0;
    TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB2;
    TcgEventHdr.EventSize = sizeof (FvBlob2);
    EventData             = &FvBlob2;
  } else {
    FvBlob.BlobBase       = FvBase;
    FvBlob.BlobLength     = FvLength;
    TcgEventHdr.PCRIndex  = 0;
    TcgEventHdr.EventType = EV_EFI_PLATFORM_FIRMWARE_BLOB;
    TcgEventHdr.EventSize = sizeof (FvBlob);
    EventData             = &FvBlob;
  }

  Status = HashLogExtendEvent (
             (UINT8 *)(UINTN)FvBase,
             (UINTN)FvLength,
             &TcgEventHdr,
             EventData
             );
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "%a: The FV which failed to be measured starts at: 0x%lx, Status: %r\n", __func__, FvBase, Status));
    } else {
      Status = EFI_SUCCESS;
    }
  }

  return Status;
}

/**
  Measure CRTM version.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureCRTMVersion (
  VOID
  )
{
  TCG_PCR_EVENT_HDR  TcgEventHdr;
  EFI_STATUS         Status;

  //
  // Use FirmwareVersion string to represent CRTM version.
  // OEMs should get real CRTM version string and measure it.
  //
  TcgEventHdr.PCRIndex  = 0;
  TcgEventHdr.EventType = EV_S_CRTM_VERSION;
  TcgEventHdr.EventSize = (UINT32)StrSize ((CHAR16 *)PcdGetPtr (PcdFirmwareVersionString));

  Status =  HashLogExtendEvent (
              (UINT8 *)PcdGetPtr (PcdFirmwareVersionString),
              TcgEventHdr.EventSize,
              &TcgEventHdr,
              (UINT8 *)PcdGetPtr (PcdFirmwareVersionString)
              );
  if (Status == EFI_UNSUPPORTED) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Measure main Firmware Volume.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid firmware volume information.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
STATIC
EFI_STATUS
MeasureMainFv (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS  FvHob;

  /*
   * First firmware volume should be "main firmware volume".
   */
  FvHob.Raw = GetNextHob (EFI_HOB_TYPE_FV, GetHobList ());
  ASSERT (FvHob.FirmwareVolume != NULL);

  return MeasureFvImage (
           FvHob.FirmwareVolume->BaseAddress,
           FvHob.FirmwareVolume->Length
           );
}

/**
  Measurement for PeilessSec.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid firmware volume information.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasurePeilessSec (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = MeasureCRTMVersion ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MeasureMainFv ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}
