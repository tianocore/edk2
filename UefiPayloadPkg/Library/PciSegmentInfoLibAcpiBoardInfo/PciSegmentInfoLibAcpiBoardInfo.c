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
  UINTN  Size;
  UINT8  Index;

  if (PciRootBridgeInfo == NULL) {
    mPciSegments = NULL;
    return;
  }

  *NumberOfRootBridges = PciRootBridgeInfo->Count;

  Size         = PciRootBridgeInfo->Count * sizeof (PCI_SEGMENT_INFO);
  mPciSegments = (PCI_SEGMENT_INFO *)AllocatePool (Size);
  ASSERT (mPciSegments != NULL);
  ZeroMem (mPciSegments, PciRootBridgeInfo->Count * sizeof (PCI_SEGMENT_INFO));

  //
  // Create all root bridges with PciRootBridgeInfoHob
  //
  for (Index = 0; Index < PciRootBridgeInfo->Count; Index++) {
    if (UplSegmentInfo->SegmentInfo[Index].SegmentNumber == (UINT16)(PciRootBridgeInfo->RootBridge[Index].Segment)) {
      mPciSegments[Index].BaseAddress = UplSegmentInfo->SegmentInfo[Index].BaseAddress;
    }

    mPciSegments[Index].SegmentNumber  = (UINT16)(PciRootBridgeInfo->RootBridge[Index].Segment);
    mPciSegments[Index].StartBusNumber = (UINT8)PciRootBridgeInfo->RootBridge[Index].Bus.Base;
    mPciSegments[Index].EndBusNumber   = (UINT8)PciRootBridgeInfo->RootBridge[Index].Bus.Limit;
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
