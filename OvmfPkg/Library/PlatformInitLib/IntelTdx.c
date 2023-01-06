/** @file
  Initialize Intel TDX support.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PlatformInitLib.h>
#include <Library/PrintLib.h>
#include <IndustryStandard/Tdx.h>
#include <IndustryStandard/IntelTdx.h>
#include <Library/PeiServicesLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Pi/PrePiHob.h>
#include <WorkArea.h>
#include <ConfidentialComputingGuestAttr.h>

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
 * Build ResourceDescriptorHob for the unaccepted memory region.
 * This memory region may be splitted into 2 parts because of lazy accept.
 *
 * @param Hob     Point to the EFI_HOB_RESOURCE_DESCRIPTOR
 * @return VOID
 */
VOID
BuildResourceDescriptorHobForUnacceptedMemory (
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *Hob
  )
{
  EFI_PHYSICAL_ADDRESS         PhysicalStart;
  EFI_PHYSICAL_ADDRESS         PhysicalEnd;
  UINT64                       ResourceLength;
  EFI_RESOURCE_TYPE            ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  UINT64                       MaxAcceptedMemoryAddress;

  ASSERT (Hob->ResourceType == BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED);

  ResourceType      = BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED;
  ResourceAttribute = Hob->ResourceAttribute;
  PhysicalStart     = Hob->PhysicalStart;
  ResourceLength    = Hob->ResourceLength;
  PhysicalEnd       = PhysicalStart + ResourceLength;

  //
  // In the first stage of lazy-accept, all the memory under 4G will be accepted.
  // The memory above 4G will not be accepted.
  //
  MaxAcceptedMemoryAddress = BASE_4GB;

  if (PhysicalEnd <= MaxAcceptedMemoryAddress) {
    //
    // This memory region has been accepted.
    //
    ResourceType       = EFI_RESOURCE_SYSTEM_MEMORY;
    ResourceAttribute |= (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED);
  } else if (PhysicalStart >= MaxAcceptedMemoryAddress) {
    //
    // This memory region hasn't been accepted.
    // So keep the ResourceType and ResourceAttribute unchange.
    //
  }

  BuildResourceDescriptorHob (
    ResourceType,
    ResourceAttribute,
    PhysicalStart,
    ResourceLength
    );
}

/**
  Transfer the incoming HobList for the TD to the final HobList for Dxe.
  The Hobs transferred in this function are ResourceDescriptor hob and
  MemoryAllocation hob.

  @param[in] VmmHobList    The Hoblist pass the firmware

**/
VOID
EFIAPI
TransferTdxHobList (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_RESOURCE_TYPE            ResourceType;
  EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute;
  VOID                         *GuidedData;

  //
  // PcdOvmfSecGhcbBase is used as the TD_HOB in Tdx guest.
  //
  Hob.Raw = (UINT8 *)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase);
  while (!END_OF_HOB_LIST (Hob)) {
    switch (Hob.Header->HobType) {
      case EFI_HOB_TYPE_RESOURCE_DESCRIPTOR:
        ResourceType      = Hob.ResourceDescriptor->ResourceType;
        ResourceAttribute = Hob.ResourceDescriptor->ResourceAttribute;

        if (ResourceType == BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED) {
          BuildResourceDescriptorHobForUnacceptedMemory (Hob.ResourceDescriptor);
        } else {
          BuildResourceDescriptorHob (
            ResourceType,
            ResourceAttribute,
            Hob.ResourceDescriptor->PhysicalStart,
            Hob.ResourceDescriptor->ResourceLength
            );
        }

        break;
      case EFI_HOB_TYPE_MEMORY_ALLOCATION:
        BuildMemoryAllocationHob (
          Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
          Hob.MemoryAllocation->AllocDescriptor.MemoryLength,
          Hob.MemoryAllocation->AllocDescriptor.MemoryType
          );
        break;
      case EFI_HOB_TYPE_GUID_EXTENSION:
        GuidedData = (VOID *)(&Hob.Guid->Name + 1);
        BuildGuidDataHob (&Hob.Guid->Name, GuidedData, Hob.Guid->Header.HobLength - sizeof (EFI_HOB_GUID_TYPE));
        break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }
}

/**
  In Tdx guest, the system memory is passed in TdHob by host VMM. So
  the major task of PlatformTdxPublishRamRegions is to walk thru the
  TdHob list and transfer the ResourceDescriptorHob and MemoryAllocationHob
  to the hobs in DXE phase.

  MemoryAllocationHob should also be created for Mailbox and Ovmf work area.
**/
VOID
EFIAPI
PlatformTdxPublishRamRegions (
  VOID
  )
{
  if (!TdIsEnabled ()) {
    return;
  }

  TransferTdxHobList ();

  //
  // The memory region defined by PcdOvmfSecGhcbBackupBase is pre-allocated by
  // host VMM and used as the td mailbox at the beginning of system boot.
  //
  BuildMemoryAllocationHob (
    FixedPcdGet32 (PcdOvmfSecGhcbBackupBase),
    FixedPcdGet32 (PcdOvmfSecGhcbBackupSize),
    EfiACPIMemoryNVS
    );

  if (FixedPcdGet32 (PcdOvmfWorkAreaSize) != 0) {
    //
    // Reserve the work area.
    //
    // Since this memory range will be used by the Reset Vector on S3
    // resume, it must be reserved as ACPI NVS.
    //
    // If S3 is unsupported, then various drivers might still write to the
    // work area. We ought to prevent DXE from serving allocation requests
    // such that they would overlap the work area.
    //
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase),
      (UINT64)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaSize),
      EfiBootServicesData
      );
  }
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
  // Add comments: why RtmrIndex is increased by 1
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
EFIAPI
PlatformBuildGuidHobForTdxMeasurement (
  VOID
  )
{
  EFI_STATUS                   Status;
  OVMF_WORK_AREA               *WorkArea;
  VOID                         *TdHobList;
  TDX_HANDOFF_TABLE_POINTERS2  HandoffTables;
  VOID                         *FvName;
  FV_HANDOFF_TABLE_POINTERS2   FvBlob2;
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
  if (WorkArea->TdxWorkArea.SecTdxWorkArea.TdHobMeasurement.Signature == SIGNATURE_32 ('T', 'D', 'H', 'B')) {
    HashValue                          = WorkArea->TdxWorkArea.SecTdxWorkArea.TdHobMeasurement.HashValue;
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
  if (WorkArea->TdxWorkArea.SecTdxWorkArea.CfvMeasurement.Signature == SIGNATURE_32 ('T', 'C', 'F', 'V')) {
    HashValue                   = WorkArea->TdxWorkArea.SecTdxWorkArea.CfvMeasurement.HashValue;
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
