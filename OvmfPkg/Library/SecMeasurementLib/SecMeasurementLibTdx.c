/** @file
*
*  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>
#include <Guid/CcEventHob.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HashLib.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <IndustryStandard/Tpm20.h>
#include <Protocol/CcMeasurement.h>
#include <Library/SecMeasurementLib.h>

#pragma pack(1)

typedef struct {
  UINT32           count;
  TPMI_ALG_HASH    hashAlg;
  BYTE             sha384[SHA384_DIGEST_SIZE];
} TDX_DIGEST_VALUE;

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

#define INVALID_PCR2MR_INDEX  0xFF

/**
    RTMR[0]  => PCR[1,7]
    RTMR[1]  => PCR[2,3,4,5]
    RTMR[2]  => PCR[8~15]
    RTMR[3]  => NA
  Note:
    PCR[0] is mapped to MRTD and should not appear here.
    PCR[6] is reserved for OEM. It is not used.
**/
UINT8
GetMappedRtmrIndex (
  UINT32  PCRIndex
  )
{
  UINT8  RtmrIndex;

  if ((PCRIndex == 6) || (PCRIndex == 0) || (PCRIndex > 15)) {
    DEBUG ((DEBUG_ERROR, "Invalid PCRIndex(%d) map to MR Index.\n", PCRIndex));
    ASSERT (FALSE);
    return INVALID_PCR2MR_INDEX;
  }

  RtmrIndex = 0;
  if ((PCRIndex == 1) || (PCRIndex == 7)) {
    RtmrIndex = 0;
  } else if ((PCRIndex >= 2) && (PCRIndex < 6)) {
    RtmrIndex = 1;
  } else if ((PCRIndex >= 8) && (PCRIndex <= 15)) {
    RtmrIndex = 2;
  }

  return RtmrIndex;
}

/**
  Tpm measure and log data, and extend the measurement result into a specific PCR.

  @param[in]  PcrIndex         PCR Index.
  @param[in]  EventType        Event type.
  @param[in]  EventLog         Measurement event log.
  @param[in]  LogLen           Event log length in bytes.
  @param[in]  HashData         The start of the data buffer to be hashed, extended.
  @param[in]  HashDataLen      The length, in bytes, of the buffer referenced by HashData
  @retval EFI_SUCCESS               Operation completed successfully.
  @retval EFI_UNSUPPORTED       TPM device not available.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.
**/
EFI_STATUS
EFIAPI
TdxMeasureAndLogData (
  IN UINT32  PcrIndex,
  IN UINT32  EventType,
  IN VOID    *EventLog,
  IN UINT32  LogLen,
  IN VOID    *HashData,
  IN UINT64  HashDataLen
  )
{
  EFI_STATUS          Status;
  UINT32              RtmrIndex;
  VOID                *EventHobData;
  TCG_PCR_EVENT2      *TcgPcrEvent2;
  UINT8               *DigestBuffer;
  TDX_DIGEST_VALUE    *TdxDigest;
  TPML_DIGEST_VALUES  DigestList;
  UINT8               *Ptr;

  RtmrIndex = GetMappedRtmrIndex (PcrIndex);
  if (RtmrIndex == INVALID_PCR2MR_INDEX) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "Creating TdTcg2PcrEvent PCR[%d]/RTMR[%d] EventType 0x%x\n", PcrIndex, RtmrIndex, EventType));

  Status = HashAndExtend (
             RtmrIndex,
             (VOID *)HashData,
             HashDataLen,
             &DigestList
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "Failed to HashAndExtend. %r\n", Status));
    return Status;
  }

  //
  // Use TDX_DIGEST_VALUE in the GUID HOB DataLength calculation
  // to reserve enough buffer to hold TPML_DIGEST_VALUES compact binary
  // which is limited to a SHA384 digest list
  //
  EventHobData = BuildGuidHob (
                   &gCcEventEntryHobGuid,
                   sizeof (TcgPcrEvent2->PCRIndex) + sizeof (TcgPcrEvent2->EventType) +
                   sizeof (TDX_DIGEST_VALUE) +
                   sizeof (TcgPcrEvent2->EventSize) + LogLen
                   );

  if (EventHobData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ptr = (UINT8 *)EventHobData;
  //
  // Initialize PcrEvent data now
  //
  RtmrIndex++;
  CopyMem (Ptr, &RtmrIndex, sizeof (UINT32));
  Ptr += sizeof (UINT32);
  CopyMem (Ptr, &EventType, sizeof (TCG_EVENTTYPE));
  Ptr += sizeof (TCG_EVENTTYPE);

  DigestBuffer = Ptr;

  TdxDigest          = (TDX_DIGEST_VALUE *)DigestBuffer;
  TdxDigest->count   = 1;
  TdxDigest->hashAlg = TPM_ALG_SHA384;
  CopyMem (
    TdxDigest->sha384,
    DigestList.digests[0].digest.sha384,
    SHA384_DIGEST_SIZE
    );

  Ptr += sizeof (TDX_DIGEST_VALUE);

  CopyMem (Ptr, &LogLen, sizeof (UINT32));
  Ptr += sizeof (UINT32);
  CopyMem (Ptr, EventLog, LogLen);
  Ptr += LogLen;

  Status = EFI_SUCCESS;
  return Status;
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

  Status = TdxMeasureAndLogData (
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

  Status = TdxMeasureAndLogData (
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
