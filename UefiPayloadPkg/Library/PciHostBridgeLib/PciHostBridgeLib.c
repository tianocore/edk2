/** @file
  Library instance of PciHostBridgeLib library class for coreboot.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016 - 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciLib.h>
#include <Library/HobLib.h>

#include "PciHostBridge.h"

STATIC
CONST
CB_PCI_ROOT_BRIDGE_DEVICE_PATH  mRootBridgeDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID (0x0A03), // HID
    0                     // UID
  },

  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

/**
  Initialize a PCI_ROOT_BRIDGE structure.

  @param[in]  Supports         Supported attributes.

  @param[in]  Attributes       Initial attributes.

  @param[in]  AllocAttributes  Allocation attributes.

  @param[in]  RootBusNumber    The bus number to store in RootBus.

  @param[in]  MaxSubBusNumber  The inclusive maximum bus number that can be
                               assigned to any subordinate bus found behind any
                               PCI bridge hanging off this root bus.

                               The caller is responsible for ensuring that
                               RootBusNumber <= MaxSubBusNumber. If
                               RootBusNumber equals MaxSubBusNumber, then the
                               root bus has no room for subordinate buses.

  @param[in]  Io               IO aperture.

  @param[in]  Mem              MMIO aperture.

  @param[in]  MemAbove4G       MMIO aperture above 4G.

  @param[in]  PMem             Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G      Prefetchable MMIO aperture above 4G.

  @param[out] RootBus          The PCI_ROOT_BRIDGE structure (allocated by the
                               caller) that should be filled in by this
                               function.

  @retval EFI_SUCCESS           Initialization successful. A device path
                                consisting of an ACPI device path node, with
                                UID = RootBusNumber, has been allocated and
                                linked into RootBus.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
InitRootBridge (
  IN  UINT64                    Supports,
  IN  UINT64                    Attributes,
  IN  UINT64                    AllocAttributes,
  IN  UINT8                     RootBusNumber,
  IN  UINT8                     MaxSubBusNumber,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G,
  OUT PCI_ROOT_BRIDGE           *RootBus
  )
{
  CB_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;

  //
  // Be safe if other fields are added to PCI_ROOT_BRIDGE later.
  //
  ZeroMem (RootBus, sizeof *RootBus);

  RootBus->Segment = 0;

  RootBus->Supports   = Supports;
  RootBus->Attributes = Attributes;

  RootBus->DmaAbove4G = FALSE;

  RootBus->AllocationAttributes = AllocAttributes;
  RootBus->Bus.Base             = RootBusNumber;
  RootBus->Bus.Limit            = MaxSubBusNumber;
  CopyMem (&RootBus->Io, Io, sizeof (*Io));
  CopyMem (&RootBus->Mem, Mem, sizeof (*Mem));
  CopyMem (&RootBus->MemAbove4G, MemAbove4G, sizeof (*MemAbove4G));
  CopyMem (&RootBus->PMem, PMem, sizeof (*PMem));
  CopyMem (&RootBus->PMemAbove4G, PMemAbove4G, sizeof (*PMemAbove4G));

  RootBus->NoExtendedConfigSpace = FALSE;

  DevicePath = AllocateCopyPool (
                 sizeof (mRootBridgeDevicePathTemplate),
                 &mRootBridgeDevicePathTemplate
                 );
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  DevicePath->AcpiDevicePath.UID = RootBusNumber;
  RootBus->DevicePath            = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;

  DEBUG ((
    DEBUG_INFO,
    "%a: populated root bus %d, with room for %d subordinate bus(es)\n",
    __FUNCTION__,
    RootBusNumber,
    MaxSubBusNumber - RootBusNumber
    ));
  return EFI_SUCCESS;
}

/**
  Initialize DevicePath for a PCI_ROOT_BRIDGE.
  @param[in] HID               HID for device path
  @param[in] UID               UID for device path

  @retval A pointer to the new created device patch.
**/
EFI_DEVICE_PATH_PROTOCOL *
CreateRootBridgeDevicePath (
  IN     UINT32  HID,
  IN     UINT32  UID
  )
{
  CB_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;

  DevicePath = AllocateCopyPool (
                 sizeof (mRootBridgeDevicePathTemplate),
                 &mRootBridgeDevicePathTemplate
                 );
  ASSERT (DevicePath != NULL);
  DevicePath->AcpiDevicePath.HID = HID;
  DevicePath->AcpiDevicePath.UID = UID;
  return (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;
}

/**
  Return all the root bridge instances in an array.

  @param Count  Return the count of root bridge instances.

  @return All the root bridge instances in an array.
          The array should be passed into PciHostBridgeFreeRootBridges()
          when it's not used.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo;
  EFI_HOB_GUID_TYPE                   *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    *GenericHeader;

  //
  // Find Universal Payload PCI Root Bridge Info hob
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadPciRootBridgeInfoGuid);
  if (GuidHob != NULL) {
    GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) <= GET_GUID_HOB_DATA_SIZE (GuidHob)) && (GenericHeader->Length <= GET_GUID_HOB_DATA_SIZE (GuidHob))) {
      if ((GenericHeader->Revision == UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION) && (GenericHeader->Length >= sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES))) {
        //
        // UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES structure is used when Revision equals to UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION
        //
        PciRootBridgeInfo = (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES *)GET_GUID_HOB_DATA (GuidHob);
        if (PciRootBridgeInfo->Count <= (GET_GUID_HOB_DATA_SIZE (GuidHob) - sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES)) / sizeof (UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE)) {
          return RetrieveRootBridgeInfoFromHob (PciRootBridgeInfo, Count);
        }
      }
    }
  }

  return ScanForRootBridges (Count);
}

/**
  Free the root bridge instances array returned from
  PciHostBridgeGetRootBridges().

  @param  Bridges    The root bridge instances array.
  @param  Count      The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE  *Bridges,
  UINTN            Count
  )
{
  if ((Bridges == NULL) && (Count == 0)) {
    return;
  }

  ASSERT (Bridges != NULL && Count > 0);

  do {
    --Count;
    FreePool (Bridges[Count].DevicePath);
  } while (Count > 0);

  FreePool (Bridges);
}

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource
                          descriptors. The Configuration contains the resources
                          for all the root bridges. The resource for each root
                          bridge is terminated with END descriptor and an
                          additional END is appended indicating the end of the
                          entire resources. The resource descriptor field
                          values follow the description in
                          EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                          .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE  HostBridgeHandle,
  VOID        *Configuration
  )
{
  //
  // coreboot UEFI Payload does not do PCI enumeration and should not call this
  // library interface.
  //
  ASSERT (FALSE);
}
