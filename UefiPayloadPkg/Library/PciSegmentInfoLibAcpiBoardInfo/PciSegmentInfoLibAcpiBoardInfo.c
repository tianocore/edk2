/** @file
  PCI Segment Information Library that returns one segment whose
  segment base address is retrieved from AcpiBoardInfo HOB.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2024, Rivos Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/AcpiBoardInfoGuid.h>

#include <Library/HobLib.h>
#include <Library/PciSegmentInfoLib.h>
#include <Library/DebugLib.h>
#include <UniversalPayload/PciRootBridges.h>
#include <Library/PciLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/PciSegmentInfoGuid.h>

static PCI_SEGMENT_INFO  *mPciSegments;
static UINTN             mCount;

/**
  Find segment info from all root bridges

  @param[in]  PciRootBridgeInfo    Pointer of Universal Payload PCI Root Bridge Info Hob
  @param[in]  UplSegmentInfo       Pointer of Universal UPL Segment Info

  @param[out] NumberOfRootBridges  Number of root bridges detected

**/
VOID
RetrieveMultiSegmentInfoFromHob (
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo,
  IN  UPL_PCI_SEGMENT_INFO_HOB            *UplSegmentInfo,
  OUT UINTN                               *NumberOfRootBridges
  )
{
  UINTN             Size;
  UINT8             IndexForExistingSegments;
  UINT8             NumberOfExistingSegments;
  UINT8             IndexForRb;
  PCI_SEGMENT_INFO  *pPciSegments;

  if (PciRootBridgeInfo == NULL) {
    mPciSegments = NULL;
    return;
  }

  IndexForExistingSegments = 0;
  NumberOfExistingSegments = 0;
  IndexForRb               = 0;
  Size                     = PciRootBridgeInfo->Count * sizeof (PCI_SEGMENT_INFO);

  *NumberOfRootBridges = PciRootBridgeInfo->Count;

  pPciSegments = (PCI_SEGMENT_INFO *)AllocatePool (Size);
  if (pPciSegments == NULL) {
    ASSERT (pPciSegments != NULL);
    return;
  }

  ZeroMem (pPciSegments, PciRootBridgeInfo->Count * sizeof (PCI_SEGMENT_INFO));

  //
  // RBs may share the same Segment number, but mPciSegments should not have duplicate segment data.
  // 1. if RB Segment is same with existing one, update StartBus and EndBus of existing one
  // 2. if RB Segment is different from all existing ones, add it to the existing Segments data buffer.
  // pPciSegments is local temporary data buffer that will be freed later (size depends on numbers of RB)
  // mPciSegments is global data that will be updated with the pPciSegments for caller to utilize. (size depends on numbers of different PciSegments)
  //
  NumberOfExistingSegments       = 1;
  pPciSegments[0].BaseAddress    = UplSegmentInfo->SegmentInfo[0].BaseAddress;
  pPciSegments[0].SegmentNumber  = (UINT16)(PciRootBridgeInfo->RootBridge[0].Segment);
  pPciSegments[0].StartBusNumber = (UINT8)PciRootBridgeInfo->RootBridge[0].Bus.Base;
  pPciSegments[0].EndBusNumber   = (UINT8)PciRootBridgeInfo->RootBridge[0].Bus.Limit;
  for (IndexForRb = 1; IndexForRb < PciRootBridgeInfo->Count; IndexForRb++) {
    for (IndexForExistingSegments = 0; IndexForExistingSegments < NumberOfExistingSegments; IndexForExistingSegments++) {
      if (pPciSegments[IndexForExistingSegments].SegmentNumber == PciRootBridgeInfo->RootBridge[IndexForRb].Segment) {
        if (pPciSegments[IndexForExistingSegments].StartBusNumber > PciRootBridgeInfo->RootBridge[IndexForRb].Bus.Base) {
          pPciSegments[IndexForExistingSegments].StartBusNumber = (UINT8)PciRootBridgeInfo->RootBridge[IndexForRb].Bus.Base;
        }

        if (pPciSegments[IndexForExistingSegments].EndBusNumber < PciRootBridgeInfo->RootBridge[IndexForRb].Bus.Limit) {
          pPciSegments[IndexForExistingSegments].EndBusNumber = (UINT8)PciRootBridgeInfo->RootBridge[IndexForRb].Bus.Limit;
        }

        break;  // breaking after a match found
      }
    }

    if (IndexForExistingSegments >= NumberOfExistingSegments) {
      //
      // No match found, add it to segments data buffer
      //
      pPciSegments[NumberOfExistingSegments].BaseAddress    = UplSegmentInfo->SegmentInfo[IndexForRb].BaseAddress;
      pPciSegments[NumberOfExistingSegments].SegmentNumber  = (UINT16)(PciRootBridgeInfo->RootBridge[IndexForRb].Segment);
      pPciSegments[NumberOfExistingSegments].StartBusNumber = (UINT8)PciRootBridgeInfo->RootBridge[IndexForRb].Bus.Base;
      pPciSegments[NumberOfExistingSegments].EndBusNumber   = (UINT8)PciRootBridgeInfo->RootBridge[IndexForRb].Bus.Limit;
      NumberOfExistingSegments++;
    }
  }

  //
  // Prepare data for returning to caller
  //
  *NumberOfRootBridges = NumberOfExistingSegments;
  Size                 = NumberOfExistingSegments * sizeof (PCI_SEGMENT_INFO);
  mPciSegments         = (PCI_SEGMENT_INFO *)AllocatePool (Size);
  if (mPciSegments == NULL) {
    ASSERT (FALSE);
    return;
  }

  CopyMem (&mPciSegments[0], &pPciSegments[0], Size);
  if (pPciSegments != NULL) {
    FreePool (pPciSegments);
  }

  return;
}

/**
  Find segment info from all root bridges for legacy systems

  @param[in]  PciRootBridgeInfo    Pointer of Universal Payload PCI Root Bridge Info Hob
  @param[out] NumberOfRootBridges  Number of root bridges detected

**/
VOID
RetrieveSegmentInfoFromHob (
  OUT UINTN  *NumberOfRootBridges
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  ACPI_BOARD_INFO    *AcpiBoardInfo;

  *NumberOfRootBridges = 1;
  // old model relies on gUefiAcpiBoardInfoGuid and hardcoded values for single segment only.
  // This is only for backward compatibility, new platforms should adopt new model even in single segment cases.
  //
  mPciSegments = (PCI_SEGMENT_INFO *)AllocatePool (sizeof (PCI_SEGMENT_INFO));
  ASSERT (mPciSegments != NULL);
  GuidHob = GetFirstGuidHob (&gUefiAcpiBoardInfoGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob != NULL) {
    AcpiBoardInfo                = (ACPI_BOARD_INFO *)GET_GUID_HOB_DATA (GuidHob);
    mPciSegments->SegmentNumber  = 0;
    mPciSegments->BaseAddress    = AcpiBoardInfo->PcieBaseAddress;
    mPciSegments->StartBusNumber = 0;
    mPciSegments->EndBusNumber   = 0xFF;
  }
}

/**
  Return info for all root bridges

  @return All the root bridge info instances in an array.
**/
UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *
Get_RBInfo (
  VOID
  )
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  EFI_HOB_GUID_TYPE                   *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    *GenericHeader;

  //
  // Find Universal Payload PCI Root Bridge Info hob
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid);
  if ((GuidHob == NULL) || (sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) > GET_GUID_HOB_DATA_SIZE (GuidHob))) {
    return NULL;
  }

  GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
  if (GenericHeader->Length > GET_GUID_HOB_DATA_SIZE (GuidHob)) {
    return NULL;
  }

  if ((GenericHeader->Revision != UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION) || (GenericHeader->Length < sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES))) {
    return NULL;
  }

  //
  // UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES structure is used when Revision equals to UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION
  //
  PciRootBridgeInfo = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *)GET_GUID_HOB_DATA (GuidHob);
  if (PciRootBridgeInfo->Count <= (GET_GUID_HOB_DATA_SIZE (GuidHob) - sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES)) / sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE)) {
    return PciRootBridgeInfo;
  }

  return NULL;
}

/**
  Return info for all root bridge segments

  @return All the segment info instances in an array.
**/
UPL_PCI_SEGMENT_INFO_HOB *
Get_UPLSegInfo (
  VOID
  )
{
  UPL_PCI_SEGMENT_INFO_HOB          *UplSegmentInfo;
  EFI_HOB_GUID_TYPE                 *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER  *GenericHeader;

  //
  // Find Universal Payload Segment Info hob
  //
  GuidHob = GetFirstGuidHob (&gUplPciSegmentInfoHobGuid);
  if ((GuidHob == NULL) || (sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) > GET_GUID_HOB_DATA_SIZE (GuidHob))) {
    return NULL;
  }

  GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
  if (GenericHeader->Length > GET_GUID_HOB_DATA_SIZE (GuidHob)) {
    return NULL;
  }

  if ((GenericHeader->Revision != UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION) || (GenericHeader->Length < sizeof (UPL_PCI_SEGMENT_INFO_HOB))) {
    return NULL;
  }

  //
  // UPL_PCI_SEGMENT_INFO_HOB structure is used when Revision equals to UPL_PCI_SEGMENT_INFO_HOB_REVISION
  //
  UplSegmentInfo = (UPL_PCI_SEGMENT_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
  if (UplSegmentInfo->Count <= (GET_GUID_HOB_DATA_SIZE (GuidHob) - sizeof (UPL_PCI_SEGMENT_INFO_HOB)) / sizeof (UPL_SEGMENT_INFO)) {
    return UplSegmentInfo;
  }

  return NULL;
}

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_SEGMENT_INFO *
EFIAPI
GetPciSegmentInfo (
  UINTN  *Count
  )
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  UPL_PCI_SEGMENT_INFO_HOB            *UplSegmentInfo;

  if (mPciSegments != NULL) {
    *Count = mCount;
    return mPciSegments;
  }

  UplSegmentInfo = Get_UPLSegInfo ();

  if (UplSegmentInfo == NULL) {
    RetrieveSegmentInfoFromHob (Count);
  } else {
    PciRootBridgeInfo = Get_RBInfo ();
    if (PciRootBridgeInfo == NULL) {
      return 0;
    }

    RetrieveMultiSegmentInfoFromHob (PciRootBridgeInfo, UplSegmentInfo, Count);
  }

  mCount = *Count;
  return mPciSegments;
}
