/** @file
  PCI Host Bridge utility functions for ArmVirt.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/PciHostBridgeUtilityLib.h>
#include <Library/PciLib.h>

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeAcpiAddressSpaceTypeStr[] = {
  L"Mem",
  L"I/O",
  L"Bus"
};

STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID (0x0A03), // HID
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


GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16 *mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

STATIC PCI_ROOT_BRIDGE mRootBridge;

/**
  Utility function to return all the root bridge instances in an array.

  @param [out] Count                  The number of root bridge instances.
  @param [in]  Attributes             Initial attributes.
  @param [in]  AllocationAttributes   Allocation attributes.
  @param [in]  DmaAbove4G             DMA above 4GB memory.
  @param [in]  NoExtendedConfigSpace  No Extended Config Space.
  @param [in]  BusMin                 Minimum Bus number, inclusive.
  @param [in]  BusMax                 Maximum Bus number, inclusive.
  @param [in]  Io                     IO aperture.
  @param [in]  Mem                    MMIO aperture.
  @param [in]  MemAbove4G             MMIO aperture above 4G.
  @param [in]  PMem                   Prefetchable MMIO aperture.
  @param [in]  PMemAbove4G            Prefetchable MMIO aperture above 4G.

  @return                            All the root bridge instances in an array.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeUtilityGetRootBridges (
  OUT UINTN                    *Count,
  IN  UINT64                   Attributes,
  IN  UINT64                   AllocationAttributes,
  IN  BOOLEAN                  DmaAbove4G,
  IN  BOOLEAN                  NoExtendedConfigSpace,
  IN  UINTN                    BusMin,
  IN  UINTN                    BusMax,
  IN  PCI_ROOT_BRIDGE_APERTURE *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE *PMemAbove4G
  )
{
  if ((Count == NULL)       ||
      (Io == NULL)          ||
      (Mem == NULL)         ||
      (MemAbove4G == NULL)  ||
      (PMem == NULL)        ||
      (PMemAbove4G == NULL)) {
    return NULL;
  }


  *Count = 1;

  mRootBridge.Segment               = 0;
  mRootBridge.Supports              = Attributes;
  mRootBridge.Attributes            = Attributes;

  mRootBridge.DmaAbove4G            = DmaAbove4G;
  mRootBridge.NoExtendedConfigSpace = NoExtendedConfigSpace;
  mRootBridge.ResourceAssigned      = FALSE;

  mRootBridge.AllocationAttributes  = AllocationAttributes;

  mRootBridge.Bus.Base              = BusMin;
  mRootBridge.Bus.Limit             = BusMax;
  mRootBridge.Io.Base               = Io->Base;
  mRootBridge.Io.Limit              = Io->Limit;
  mRootBridge.Mem.Base              = Mem->Base;
  mRootBridge.Mem.Limit             = Mem->Limit;
  mRootBridge.MemAbove4G.Base       = MemAbove4G->Base;
  mRootBridge.MemAbove4G.Limit      = MemAbove4G->Limit;
  mRootBridge.PMem.Base             = PMem->Base;
  mRootBridge.PMem.Limit            = PMem->Limit;
  mRootBridge.PMemAbove4G.Base      = PMemAbove4G->Base;
  mRootBridge.PMemAbove4G.Limit     = PMemAbove4G->Limit;

  mRootBridge.DevicePath =
    (EFI_DEVICE_PATH_PROTOCOL*)&mEfiPciRootBridgeDevicePath;

  return &mRootBridge;
}

/**
  Utility function to free root bridge instances array from
  PciHostBridgeUtilityGetRootBridges().

  @param[in] Bridges  The root bridge instances array.
  @param[in] Count    The count of the array.
**/
VOID
EFIAPI
PciHostBridgeUtilityFreeRootBridges (
  IN PCI_ROOT_BRIDGE *Bridges,
  IN UINTN           Count
  )
{
  // Nothing to do here.
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
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR*)Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              ARRAY_SIZE (mPciHostBridgeAcpiAddressSpaceTypeStr)
              );
      DEBUG ((
        DEBUG_ERROR,
        " %s: Length/Alignment = 0x%lx / 0x%lx\n",
        mPciHostBridgeAcpiAddressSpaceTypeStr[Descriptor->ResType],
        Descriptor->AddrLen,
        Descriptor->AddrRangeMax
        ));
      if (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) {
        DEBUG ((
          DEBUG_ERROR,
          "     Granularity/SpecificFlag = %ld / %02x%s\n",
          Descriptor->AddrSpaceGranularity,
          Descriptor->SpecificFlag,
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
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR*)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR*)Descriptor + 1
                   );
  }
}
