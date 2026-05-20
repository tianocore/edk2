/** @file
  This module implements EDKII CC Measurement PPI.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/CcMeasurement.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/TdxLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/HobLib.h>
#include <Library/TdxMeasurementLib.h>

/**
  Do a hash operation on a data buffer, extend a specific RTMR with the hash result,
  and build a GUIDed HOB recording the event.

  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData
  @param[in]      EventType     Type of the Event.
  @param[in]      MrIndex       CC Mr Index.
  @param[in]      EventData     Physical address of the start of the data buffer.
  @param[in]      EventSize     The length, in bytes, of the buffer referenced by EventData.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval Others                The operation failed with an unexpected error.

**/
EFI_STATUS
TdxPeiHashLogExtendEvent (
  IN      UINT64  Flags,
  IN      UINT8   *HashData,
  IN      UINT64  HashDataLen,
  IN      UINT32  EventType,
  IN      UINT32  MrIndex,
  IN      UINT8   *EventData,
  IN      UINT32  EventSize
  )
{
  EFI_STATUS  Status;
  UINT8       Digest[SHA384_DIGEST_SIZE];

  Status = TdxMeasurementHashAndExtendToRtmr (
             MrIndex - 1,
             HashData,
             (UINTN)HashDataLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TdxMeasurementHashAndExtendToRtmr failed with %r\n", __func__, Status));
    return Status;
  }

  Status = TdxMeasurementBuildGuidHob (
             MrIndex - 1,
             EventType,
             EventData,
             EventSize,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: TdxMeasurementBuildGuidHob failed with %r\n", __func__, Status));
  }

  return Status;
}

/**
  Do a hash operation on a data buffer, extend a specific RTMR with the hash result,
  and build a GUIDed HOB recording the event which will be passed to the DXE phase and
  added into the Event Log.

  @param[in]      This          Indicates the calling context
  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer to be hashed.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData.
  @param[in]      NewEventHdr   Pointer to a CC_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval Others                The operation failed with an unexpected error.

**/
EFI_STATUS
EFIAPI
TdHashLogExtendEvent (
  IN EDKII_CC_PPI          *This,
  IN UINT64                Flags,
  IN EFI_PHYSICAL_ADDRESS  DataToHash,
  IN UINTN                 DataToHashLen,
  IN CC_EVENT_HDR          *NewEventHdr,
  IN UINT8                 *NewEventData
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_VERBOSE, "TdHashLogExtendEvent ...\n"));

  if ((This == NULL) || (NewEventHdr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Do not check hash data size for EV_NO_ACTION event.
  //
  if ((NewEventHdr->EventType != EV_NO_ACTION) && (DataToHash == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (NewEventHdr->MrIndex == CC_MR_INDEX_0_MRTD) {
    DEBUG ((DEBUG_ERROR, "%a: MRTD cannot be extended in TDVF.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (NewEventHdr->MrIndex >= CC_MR_INDEX_INVALID) {
    DEBUG ((DEBUG_ERROR, "%a: MrIndex is invalid. (%d)\n", __func__, NewEventHdr->MrIndex));
    return EFI_INVALID_PARAMETER;
  }

  Status = TdxPeiHashLogExtendEvent (
             Flags,
             (UINT8 *)(UINTN)DataToHash,
             DataToHashLen,
             NewEventHdr->EventType,
             NewEventHdr->MrIndex,
             NewEventData,
             NewEventHdr->EventSize
             );

  DEBUG ((DEBUG_VERBOSE, "TdHashLogExtendEvent - %r\n", Status));
  return Status;
}

EFI_STATUS
EFIAPI
TdMapPcrToMrIndex (
  IN  EDKII_CC_PPI  *This,
  IN  UINT32        PCRIndex,
  OUT UINT32        *MrIndex
  )
{
  *MrIndex = TdxMeasurementMapPcrToMrIndex (PCRIndex);

  return EFI_SUCCESS;
}

EDKII_CC_PPI  mEdkiiCcPpi = {
  TdHashLogExtendEvent,
  TdMapPcrToMrIndex
};

EFI_PEI_PPI_DESCRIPTOR  mCcPpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiCcPpiGuid,
  &mEdkiiCcPpi
};

/**
  Entry point of this module.

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Describes the list of possible PEI Services.

  @return Status.

**/
EFI_STATUS
EFIAPI
PeimEntryMA (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  if (!TdIsEnabled ()) {
    return EFI_UNSUPPORTED;
  }

  Status = PeiServicesInstallPpi (&mCcPpiList);

  DEBUG ((DEBUG_INFO, "%a::CC Measurement PPI install Status is %r.\n", __func__, Status));

  return Status;
}
