/** @file
  This module implements EFI CC Measurement PPI.

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

#define   CC_MR_INDEX_0_MRTD   0
#define   CC_MR_INDEX_1_RTMR0  1
#define   CC_MR_INDEX_2_RTMR1  2
#define   CC_MR_INDEX_3_RTMR2  3
#define   CC_MR_INDEX_INVALID  4

/**
  According to UEFI Spec 2.10 Section 38.4.1:
    The following table shows the TPM PCR index mapping and CC event log measurement
  register index interpretation for Intel TDX, where MRTD means Trust Domain Measurement
   Register and RTMR means Runtime Measurement Register

    // TPM PCR Index | CC Measurement Register Index | TDX-measurement register
    //  ------------------------------------------------------------------------
    // 0             |   0                           |   MRTD
    // 1, 7          |   1                           |   RTMR[0]
    // 2~6           |   2                           |   RTMR[1]
    // 8~15          |   3                           |   RTMR[2]

  @param[in] PCRIndex Index of the TPM PCR

  @retval    UINT32               Index of the CC Event Log Measurement Register Index
  @retval    CC_MR_INDEX_INVALID  Invalid MR Index
**/
UINT32
EFIAPI
MapPcrToMrIndex (
  IN  UINT32  PCRIndex
  )
{
  UINT32  MrIndex;

  if (PCRIndex > 15) {
    ASSERT (FALSE);
    return CC_MR_INDEX_INVALID;
  }

  MrIndex = 0;
  if (PCRIndex == 0) {
    MrIndex = CC_MR_INDEX_0_MRTD;
  } else if ((PCRIndex == 1) || (PCRIndex == 7)) {
    MrIndex = CC_MR_INDEX_1_RTMR0;
  } else if ((PCRIndex >= 2) && (PCRIndex <= 6)) {
    MrIndex = CC_MR_INDEX_2_RTMR1;
  } else if ((PCRIndex >= 8) && (PCRIndex <= 15)) {
    MrIndex = CC_MR_INDEX_3_RTMR2;
  }

  return MrIndex;
}

/**
 * Calculate the sha384 of input Data and extend it to RTMR register.
 *
 * @param RtmrIndex       Index of the RTMR register
 * @param DataToHash      Data to be hashed
 * @param DataToHashLen   Length of the data
 * @param Digest          Hash value of the input data
 * @param DigestLen       Length of the hash value
 *
 * @retval EFI_SUCCESS    Successfully hash and extend to RTMR
 * @retval Others         Other errors as indicated
 */
STATIC
EFI_STATUS
HashAndExtendToRtmr (
  IN UINT32  RtmrIndex,
  IN VOID    *DataToHash,
  IN UINTN   DataToHashLen,
  OUT UINT8  *Digest,
  IN  UINTN  DigestLen
  )
{
  EFI_STATUS  Status;

  if ((DataToHash == NULL) || (DataToHashLen == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Digest == NULL) || (DigestLen != SHA384_DIGEST_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the sha384 of the data
  //
  if (!Sha384HashAll (DataToHash, DataToHashLen, Digest)) {
    return EFI_ABORTED;
  }

  //
  // Extend to RTMR
  //
  Status = TdExtendRtmr (
             (UINT32 *)Digest,
             SHA384_DIGEST_SIZE,
             (UINT8)RtmrIndex
             );

  ASSERT (!EFI_ERROR (Status));
  return Status;
}

/**
 * Build GuidHob for Tdx measurement.
 *
 * Tdx measurement includes the measurement of TdHob and CFV. They're measured
 * and extended to RTMR registers in SEC phase. Because at that moment the Hob
 * service are not available. So the values of the measurement are saved in
 * workarea and will be built into GuidHob after the Hob service is ready.
 *
 * @param RtmrIndex     RTMR index
 * @param EventType     Event type
 * @param EventData     Event data
 * @param EventSize     Size of event data
 * @param HashValue     Hash value
 * @param HashSize      Size of hash
 *
 * @retval EFI_SUCCESS  Successfully build the GuidHobs
 * @retval Others       Other error as indicated
 */
EFI_STATUS
BuildTdxMeasurementGuidHob (
  UINT32  RtmrIndex,
  UINT32  EventType,
  UINT8   *EventData,
  UINT32  EventSize,
  UINT8   *HashValue,
  UINT32  HashSize
  )
{
  VOID                *EventHobData;
  UINT8               *Ptr;
  TPML_DIGEST_VALUES  *TdxDigest;

  if (HashSize != SHA384_DIGEST_SIZE) {
    return EFI_INVALID_PARAMETER;
  }

  #define TDX_DIGEST_VALUE_LEN  (sizeof (UINT32) + sizeof (TPMI_ALG_HASH) + SHA384_DIGEST_SIZE)

  EventHobData = BuildGuidHob (
                   &gCcEventEntryHobGuid,
                   sizeof (TCG_PCRINDEX) + sizeof (TCG_EVENTTYPE) +
                   TDX_DIGEST_VALUE_LEN +
                   sizeof (UINT32) + EventSize
                   );

  if (EventHobData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = (UINT8 *)EventHobData;

  //
  // There are 2 types of measurement registers in TDX: MRTD and RTMR[0-3].
  // According to UEFI Spec 2.10 Section 38.4.1, RTMR[0-3] is mapped to MrIndex[1-4].
  // So RtmrIndex must be increased by 1 before the event log is created.
  //
  RtmrIndex++;
  CopyMem (Ptr, &RtmrIndex, sizeof (UINT32));
  Ptr += sizeof (UINT32);

  CopyMem (Ptr, &EventType, sizeof (TCG_EVENTTYPE));
  Ptr += sizeof (TCG_EVENTTYPE);

  TdxDigest                     = (TPML_DIGEST_VALUES *)Ptr;
  TdxDigest->count              = 1;
  TdxDigest->digests[0].hashAlg = TPM_ALG_SHA384;
  CopyMem (
    TdxDigest->digests[0].digest.sha384,
    HashValue,
    SHA384_DIGEST_SIZE
    );
  Ptr += TDX_DIGEST_VALUE_LEN;

  CopyMem (Ptr, &EventSize, sizeof (UINT32));
  Ptr += sizeof (UINT32);

  CopyMem (Ptr, (VOID *)EventData, EventSize);
  Ptr += EventSize;

  return EFI_SUCCESS;
}

/**
  Do a hash operation on a data buffer, extend a specific RTMR with the hash result,
  and add an entry to the Event Log.

  @param[in]      Flags         Bitmap providing additional information.
  @param[in]      HashData      Physical address of the start of the data buffer
                                to be hashed, extended, and logged.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced by HashData
  @param[in]      EventType     Type of the Event.
  @param[in]      MrIndex       CC Mr Index.
  @param[in]      EventData     Physical address of the start of the data buffer.
  @param[in]      EventSize     The length, in bytes, of the buffer referenced by EventData.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

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

  Status = HashAndExtendToRtmr (
             MrIndex - 1,
             HashData,
             (UINTN)HashDataLen,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: HashAndExtendToRtmr failed with %r\n", __func__, Status));
    return Status;
  }

  Status = BuildTdxMeasurementGuidHob (
             MrIndex - 1,
             EventType,
             EventData,
             EventSize,
             Digest,
             SHA384_DIGEST_SIZE
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: BuildTdxMeasurementGuidHob failed with %r\n", __func__, Status));
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

  DEBUG ((DEBUG_VERBOSE, "Pei::TdHashLogExtendEvent ...\n"));

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
  *MrIndex = MapPcrToMrIndex (PCRIndex);

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
