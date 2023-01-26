/** @file
  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/HobLib.h>
#include <Library/PrintLib.h>
#include <Library/TcgEventLogRecordLib.h>
#include <Library/TpmMeasurementLib.h>
#include <WorkArea.h>

#include "PeilessStartupInternal.h"

#pragma pack(1)

#define HANDOFF_TABLE_DESC  "TdxTable"
typedef struct {
  UINT8                      TableDescriptionSize;
  UINT8                      TableDescription[sizeof (HANDOFF_TABLE_DESC)];
  UINT64                     NumberOfTables;
  EFI_CONFIGURATION_TABLE    TableEntry[1];
} TDX_HANDOFF_TABLE_POINTERS2;

#pragma pack()

#define FV_HANDOFF_TABLE_DESC  "Fv(XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX)"
typedef PLATFORM_FIRMWARE_BLOB2_STRUCT CFV_HANDOFF_TABLE_POINTERS2;

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
STATIC
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
  Build the GuidHob for tdx measurements which were done in SEC phase.
  The measurement values are stored in WorkArea.

  @retval EFI_SUCCESS  The GuidHob is built successfully
  @retval Others       Other errors as indicated
**/
EFI_STATUS
InternalBuildGuidHobForTdxMeasurement (
  VOID
  )
{
  EFI_STATUS                   Status;
  OVMF_WORK_AREA               *WorkArea;
  VOID                         *TdHobList;
  TDX_HANDOFF_TABLE_POINTERS2  HandoffTables;
  VOID                         *FvName;
  CFV_HANDOFF_TABLE_POINTERS2  FvBlob2;
  EFI_PHYSICAL_ADDRESS         FvBase;
  UINT64                       FvLength;
  UINT8                        *HashValue;

  if (!TdIsEnabled ()) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);
  if (WorkArea == NULL) {
    return EFI_ABORTED;
  }

  Status = EFI_SUCCESS;

  //
  // Build the GuidHob for TdHob measurement
  //
  TdHobList = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  if (WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.MeasurementsBitmap & TDX_MEASUREMENT_TDHOB_BITMASK) {
    HashValue                          = WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.TdHobHashValue;
    HandoffTables.TableDescriptionSize = sizeof (HandoffTables.TableDescription);
    CopyMem (HandoffTables.TableDescription, HANDOFF_TABLE_DESC, sizeof (HandoffTables.TableDescription));
    HandoffTables.NumberOfTables = 1;
    CopyGuid (&(HandoffTables.TableEntry[0].VendorGuid), &gUefiOvmfPkgTokenSpaceGuid);
    HandoffTables.TableEntry[0].VendorTable = TdHobList;

    Status = BuildTdxMeasurementGuidHob (
               0,                               // RtmrIndex
               EV_EFI_HANDOFF_TABLES2,          // EventType
               (UINT8 *)(UINTN)&HandoffTables,  // EventData
               sizeof (HandoffTables),          // EventSize
               HashValue,                       // HashValue
               SHA384_DIGEST_SIZE               // HashSize
               );
  }

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  //
  // Build the GuidHob for Cfv measurement
  //
  if (WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.MeasurementsBitmap & TDX_MEASUREMENT_CFVIMG_BITMASK) {
    HashValue                   = WorkArea->TdxWorkArea.SecTdxWorkArea.TdxMeasurementsData.CfvImgHashValue;
    FvBase                      = (UINT64)PcdGet32 (PcdOvmfFlashNvStorageVariableBase);
    FvLength                    = (UINT64)PcdGet32 (PcdCfvRawDataSize);
    FvBlob2.BlobDescriptionSize = sizeof (FvBlob2.BlobDescription);
    CopyMem (FvBlob2.BlobDescription, FV_HANDOFF_TABLE_DESC, sizeof (FvBlob2.BlobDescription));
    FvName = GetFvName (FvBase, FvLength);
    if (FvName != NULL) {
      AsciiSPrint ((CHAR8 *)FvBlob2.BlobDescription, sizeof (FvBlob2.BlobDescription), "Fv(%g)", FvName);
    }

    FvBlob2.BlobBase   = FvBase;
    FvBlob2.BlobLength = FvLength;

    Status = BuildTdxMeasurementGuidHob (
               0,                              // RtmrIndex
               EV_EFI_PLATFORM_FIRMWARE_BLOB2, // EventType
               (VOID *)&FvBlob2,               // EventData
               sizeof (FvBlob2),               // EventSize
               HashValue,                      // HashValue
               SHA384_DIGEST_SIZE              // HashSize
               );
  }

  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  return EFI_SUCCESS;
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
  EFI_STATUS                   Status;
  CFV_HANDOFF_TABLE_POINTERS2  FvBlob2;
  VOID                         *FvName;

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
