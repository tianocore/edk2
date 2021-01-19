/** @file
  Provide common utility functions to PciHostBridgeLib instances in
  ArmVirtPkg and OvmfPkg.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, Huawei Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciHostBridgeUtilityLib.h>


#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()


GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeUtilityLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};


STATIC
CONST
OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH mRootBridgeDevicePathTemplate = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof(ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof(ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID(0x0A03), // HID
    0                    // UID
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
  Utility function to initialize a PCI_ROOT_BRIDGE structure.

  @param[in]  Supports         Supported attributes.

  @param[in]  Attributes       Initial attributes.

  @param[in]  AllocAttributes  Allocation attributes.

  @param[in]  RootBusNumber    The bus number to store in RootBus.

  @param[in]  MaxSubBusNumber  The inclusive maximum bus number that can be
                               assigned to any subordinate bus found behind any
                               PCI bridge hanging off this root bus.

                               The caller is repsonsible for ensuring that
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
EFIAPI
PciHostBridgeUtilityInitRootBridge (
  IN  UINT64                   Supports,
  IN  UINT64                   Attributes,
  IN  UINT64                   AllocAttributes,
  IN  UINT8                    RootBusNumber,
  IN  UINT8                    MaxSubBusNumber,
  IN  PCI_ROOT_BRIDGE_APERTURE *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE *PMemAbove4G,
  OUT PCI_ROOT_BRIDGE          *RootBus
  )
{
  OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH *DevicePath;

  //
  // Be safe if other fields are added to PCI_ROOT_BRIDGE later.
  //
  ZeroMem (RootBus, sizeof *RootBus);

  RootBus->Segment = 0;

  RootBus->Supports   = Supports;
  RootBus->Attributes = Attributes;

  RootBus->DmaAbove4G = FALSE;

  RootBus->AllocationAttributes = AllocAttributes;
  RootBus->Bus.Base  = RootBusNumber;
  RootBus->Bus.Limit = MaxSubBusNumber;
  CopyMem (&RootBus->Io, Io, sizeof (*Io));
  CopyMem (&RootBus->Mem, Mem, sizeof (*Mem));
  CopyMem (&RootBus->MemAbove4G, MemAbove4G, sizeof (*MemAbove4G));
  CopyMem (&RootBus->PMem, PMem, sizeof (*PMem));
  CopyMem (&RootBus->PMemAbove4G, PMemAbove4G, sizeof (*PMemAbove4G));

  RootBus->NoExtendedConfigSpace = (PcdGet16 (PcdOvmfHostBridgePciDevId) !=
                                    INTEL_Q35_MCH_DEVICE_ID);

  DevicePath = AllocateCopyPool (sizeof mRootBridgeDevicePathTemplate,
                 &mRootBridgeDevicePathTemplate);
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __FUNCTION__, EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }
  DevicePath->AcpiDevicePath.UID = RootBusNumber;
  RootBus->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;

  DEBUG ((DEBUG_INFO,
    "%a: populated root bus %d, with room for %d subordinate bus(es)\n",
    __FUNCTION__, RootBusNumber, MaxSubBusNumber - RootBusNumber));
  return EFI_SUCCESS;
}


/**
  Utility function to uninitialize a PCI_ROOT_BRIDGE structure set up with
  PciHostBridgeUtilityInitRootBridge().

  @param[in] RootBus  The PCI_ROOT_BRIDGE structure, allocated by the caller and
                      initialized with PciHostBridgeUtilityInitRootBridge(),
                      that should be uninitialized. This function doesn't free
                      RootBus.
**/
VOID
EFIAPI
PciHostBridgeUtilityUninitRootBridge (
  IN PCI_ROOT_BRIDGE *RootBus
  )
{
  FreePool (RootBus->DevicePath);
}


/**
  Utility function to inform the platform that the resource conflict happens.

  @param[in] Configuration  Pointer to PCI I/O and PCI memory resource
                            descriptors. The Configuration contains the
                            resources for all the root bridges. The resource
                            for each root bridge is terminated with END
                            descriptor and an additional END is appended
                            indicating the end of the entire resources. The
                            resource descriptor field values follow the
                            description in
                            EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL
                            .SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeUtilityResourceConflict (
  IN VOID  *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *Descriptor;
  UINTN                             RootBridgeIndex;
  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              ARRAY_SIZE (mPciHostBridgeUtilityLibAcpiAddressSpaceTypeStr)
              );
      DEBUG ((DEBUG_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeUtilityLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax
              ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((DEBUG_ERROR, "     Granularity/SpecificFlag = %ld / %02x%s\n",
                Descriptor->AddrSpaceGranularity, Descriptor->SpecificFlag,
                ((Descriptor->SpecificFlag &
                  EFI_ACPI_MEMORY_RESOURCE_SPECIFIC_FLAG_CACHEABLE_PREFETCHABLE
                  ) != 0) ? L" (Prefetchable)" : L""
                ));
      }
    }
    //
    // Skip the END descriptor for root bridge
    //
    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}

