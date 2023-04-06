/** @file
  Provide common utility functions to PciHostBridgeLib instances in
  ArmVirtPkg and OvmfPkg.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, Huawei Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi10.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeUtilityLib.h>
#include <Library/PciLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH        AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    EndDevicePath;
} OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

GLOBAL_REMOVE_IF_UNREFERENCED
CHAR16  *mPciHostBridgeUtilityLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

STATIC
CONST
OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH  mRootBridgeDevicePathTemplate = {
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
  Utility function to initialize a PCI_ROOT_BRIDGE structure.

  @param[in]  Supports               Supported attributes.

  @param[in]  Attributes             Initial attributes.

  @param[in]  AllocAttributes        Allocation attributes.

  @param[in]  DmaAbove4G             DMA above 4GB memory.

  @param[in]  NoExtendedConfigSpace  No Extended Config Space.

  @param[in]  RootBusNumber          The bus number to store in RootBus.

  @param[in]  MaxSubBusNumber        The inclusive maximum bus number that can
                                     be assigned to any subordinate bus found
                                     behind any PCI bridge hanging off this
                                     root bus.

                                     The caller is repsonsible for ensuring
                                     that RootBusNumber <= MaxSubBusNumber. If
                                     RootBusNumber equals MaxSubBusNumber, then
                                     the root bus has no room for subordinate
                                     buses.

  @param[in]  Io                     IO aperture.

  @param[in]  Mem                    MMIO aperture.

  @param[in]  MemAbove4G             MMIO aperture above 4G.

  @param[in]  PMem                   Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G            Prefetchable MMIO aperture above 4G.

  @param[out] RootBus                The PCI_ROOT_BRIDGE structure (allocated
                                     by the caller) that should be filled in by
                                     this function.

  @retval EFI_SUCCESS                Initialization successful. A device path
                                     consisting of an ACPI device path node,
                                     with UID = RootBusNumber, has been
                                     allocated and linked into RootBus.

  @retval EFI_OUT_OF_RESOURCES       Memory allocation failed.
**/
EFI_STATUS
EFIAPI
PciHostBridgeUtilityInitRootBridge (
  IN  UINT64                    Supports,
  IN  UINT64                    Attributes,
  IN  UINT64                    AllocAttributes,
  IN  BOOLEAN                   DmaAbove4G,
  IN  BOOLEAN                   NoExtendedConfigSpace,
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
  OVMF_PCI_ROOT_BRIDGE_DEVICE_PATH  *DevicePath;

  //
  // Be safe if other fields are added to PCI_ROOT_BRIDGE later.
  //
  ZeroMem (RootBus, sizeof *RootBus);

  RootBus->Segment = 0;

  RootBus->Supports   = Supports;
  RootBus->Attributes = Attributes;

  RootBus->DmaAbove4G = DmaAbove4G;

  RootBus->AllocationAttributes = AllocAttributes;
  RootBus->Bus.Base             = RootBusNumber;
  RootBus->Bus.Limit            = MaxSubBusNumber;
  CopyMem (&RootBus->Io, Io, sizeof (*Io));
  CopyMem (&RootBus->Mem, Mem, sizeof (*Mem));
  CopyMem (&RootBus->MemAbove4G, MemAbove4G, sizeof (*MemAbove4G));
  CopyMem (&RootBus->PMem, PMem, sizeof (*PMem));
  CopyMem (&RootBus->PMemAbove4G, PMemAbove4G, sizeof (*PMemAbove4G));

  RootBus->NoExtendedConfigSpace = NoExtendedConfigSpace;

  DevicePath = AllocateCopyPool (
                 sizeof mRootBridgeDevicePathTemplate,
                 &mRootBridgeDevicePathTemplate
                 );
  if (DevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    return EFI_OUT_OF_RESOURCES;
  }

  DevicePath->AcpiDevicePath.UID = RootBusNumber;
  RootBus->DevicePath            = (EFI_DEVICE_PATH_PROTOCOL *)DevicePath;

  DEBUG ((
    DEBUG_INFO,
    "%a: populated root bus %d, with room for %d subordinate bus(es)\n",
    __func__,
    RootBusNumber,
    MaxSubBusNumber - RootBusNumber
    ));
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
  IN PCI_ROOT_BRIDGE  *RootBus
  )
{
  FreePool (RootBus->DevicePath);
}

/**
  Utility function to scan PCI root bridges and create instances for those
  that are found not empty. Populate their resources from the default
  provided parameters and return all the root bridge instances in an array.

  @param[out] Count                  The number of root bridge instances.

  @param[in]  Attributes             Initial attributes.

  @param[in]  AllocAttributes        Allocation attributes.

  @param[in]  DmaAbove4G             DMA above 4GB memory.

  @param[in]  NoExtendedConfigSpace  No Extended Config Space.

  @param[in]  BusMin                 Minimum Bus number, inclusive.

  @param[in]  BusMax                 Maximum Bus number, inclusive.

  @param[in]  Io                     IO aperture.

  @param[in]  Mem                    MMIO aperture.

  @param[in]  MemAbove4G             MMIO aperture above 4G.

  @param[in]  PMem                   Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G            Prefetchable MMIO aperture above 4G.

  @return                            All the root bridge instances in an array.
**/
STATIC
PCI_ROOT_BRIDGE *
PciHostBridgeUtilityGetRootBridgesBusScan (
  OUT UINTN                     *Count,
  IN  UINT64                    Attributes,
  IN  UINT64                    AllocationAttributes,
  IN  BOOLEAN                   DmaAbove4G,
  IN  BOOLEAN                   NoExtendedConfigSpace,
  IN  UINTN                     BusMin,
  IN  UINTN                     BusMax,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  UINT64                ExtraRootBridges;
  PCI_ROOT_BRIDGE       *Bridges;
  UINTN                 Initialized;
  UINTN                 LastRootBridgeNumber;
  UINTN                 RootBridgeNumber;

  if ((BusMin > BusMax) || (BusMax > PCI_MAX_BUS)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid bus range with BusMin %Lu and BusMax "
      "%Lu\n",
      __func__,
      (UINT64)BusMin,
      (UINT64)BusMax
      ));
    return NULL;
  }

  //
  // QEMU provides the number of extra root buses, shortening the exhaustive
  // search below. If there is no hint, the feature is missing.
  //
  Status = QemuFwCfgFindFile ("etc/extra-pci-roots", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status) || (FwCfgSize != sizeof ExtraRootBridges)) {
    ExtraRootBridges = 0;
  } else {
    QemuFwCfgSelectItem (FwCfgItem);
    QemuFwCfgReadBytes (FwCfgSize, &ExtraRootBridges);

    //
    // Validate the number of extra root bridges. As BusMax is inclusive, the
    // max bus count is (BusMax - BusMin + 1). From that, the "main" root bus
    // is always a given, so the max count for the "extra" root bridges is one
    // less, i.e. (BusMax - BusMin). If the QEMU hint exceeds that, we have
    // invalid behavior.
    //
    if (ExtraRootBridges > BusMax - BusMin) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: invalid count of extra root buses (%Lu) "
        "reported by QEMU\n",
        __func__,
        ExtraRootBridges
        ));
      return NULL;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: %Lu extra root buses reported by QEMU\n",
      __func__,
      ExtraRootBridges
      ));
  }

  //
  // Allocate the "main" root bridge, and any extra root bridges.
  //
  Bridges = AllocatePool ((1 + (UINTN)ExtraRootBridges) * sizeof *Bridges);
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    return NULL;
  }

  Initialized = 0;

  //
  // The "main" root bus is always there.
  //
  LastRootBridgeNumber = BusMin;

  //
  // Scan all other root buses. If function 0 of any device on a bus returns a
  // VendorId register value different from all-bits-one, then that bus is
  // alive.
  //
  for (RootBridgeNumber = BusMin + 1;
       RootBridgeNumber <= BusMax && Initialized < ExtraRootBridges;
       ++RootBridgeNumber)
  {
    UINTN  Device;

    for (Device = 0; Device <= PCI_MAX_DEVICE; ++Device) {
      if (PciRead16 (
            PCI_LIB_ADDRESS (
              RootBridgeNumber,
              Device,
              0,
              PCI_VENDOR_ID_OFFSET
              )
            ) != MAX_UINT16)
      {
        break;
      }
    }

    if (Device <= PCI_MAX_DEVICE) {
      //
      // Found the next root bus. We can now install the *previous* one,
      // because now we know how big a bus number range *that* one has, for any
      // subordinate buses that might exist behind PCI bridges hanging off it.
      //
      Status = PciHostBridgeUtilityInitRootBridge (
                 Attributes,
                 Attributes,
                 AllocationAttributes,
                 DmaAbove4G,
                 NoExtendedConfigSpace,
                 (UINT8)LastRootBridgeNumber,
                 (UINT8)(RootBridgeNumber - 1),
                 Io,
                 Mem,
                 MemAbove4G,
                 PMem,
                 PMemAbove4G,
                 &Bridges[Initialized]
                 );
      if (EFI_ERROR (Status)) {
        goto FreeBridges;
      }

      ++Initialized;
      LastRootBridgeNumber = RootBridgeNumber;
    }
  }

  //
  // Install the last root bus (which might be the only, ie. main, root bus, if
  // we've found no extra root buses).
  //
  Status = PciHostBridgeUtilityInitRootBridge (
             Attributes,
             Attributes,
             AllocationAttributes,
             DmaAbove4G,
             NoExtendedConfigSpace,
             (UINT8)LastRootBridgeNumber,
             (UINT8)BusMax,
             Io,
             Mem,
             MemAbove4G,
             PMem,
             PMemAbove4G,
             &Bridges[Initialized]
             );
  if (EFI_ERROR (Status)) {
    goto FreeBridges;
  }

  ++Initialized;

  *Count = Initialized;
  return Bridges;

FreeBridges:
  while (Initialized > 0) {
    --Initialized;
    PciHostBridgeUtilityUninitRootBridge (&Bridges[Initialized]);
  }

  FreePool (Bridges);
  return NULL;
}

/**
  Utility function to read root bridges information from host-provided fw-cfg
  file and return them in an array.

  @param[out] Count   The number of root bridge instances.

  @return             All the root bridge instances in an array parsed from
                      host-provided fw-cfg file (hardware-info).
**/
STATIC
PCI_ROOT_BRIDGE *
PciHostBridgeUtilityGetRootBridgesHostProvided (
  OUT UINTN  *Count
  )
{
  EFI_STATUS                Status;
  FIRMWARE_CONFIG_ITEM      FwCfgItem;
  UINTN                     FwCfgSize;
  PCI_ROOT_BRIDGE           *Bridges;
  UINTN                     Initialized;
  UINTN                     LastRootBridgeNumber;
  UINTN                     RootBridgeNumber;
  UINTN                     PciHostBridgeCount;
  UINT8                     *HardwareInfoBlob;
  LIST_ENTRY                HwInfoList;
  LIST_ENTRY                *HwLink;
  HARDWARE_INFO             *HwInfo;
  UINT64                    Attributes;
  UINT64                    AllocationAttributes;
  BOOLEAN                   DmaAbove4G;
  BOOLEAN                   NoExtendedConfigSpace;
  BOOLEAN                   CombineMemPMem;
  PCI_ROOT_BRIDGE_APERTURE  Io;
  PCI_ROOT_BRIDGE_APERTURE  Mem;
  PCI_ROOT_BRIDGE_APERTURE  MemAbove4G;
  PCI_ROOT_BRIDGE_APERTURE  PMem;
  PCI_ROOT_BRIDGE_APERTURE  PMemAbove4G;

  //
  // Initialize the Hardware Info list head to start with an empty but valid
  // list head.
  //
  InitializeListHead (&HwInfoList);
  HardwareInfoBlob   =  NULL;
  Initialized        = 0;
  Bridges            = NULL;
  PciHostBridgeCount = 0;

  //
  // Hypervisor can provide the specifications (resources) for one or more
  // PCI host bridges. Such information comes through fw-cfg as part of
  // the hardware-info file.
  //
  Status = QemuFwCfgFindFile ("etc/hardware-info", &FwCfgItem, &FwCfgSize);

  if (EFI_ERROR (Status)) {
    return NULL;
  }

  HardwareInfoBlob = AllocatePool (FwCfgSize);

  if (HardwareInfoBlob == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to allocate memory for hardware resources info\n",
      __func__
      ));
    return NULL;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, HardwareInfoBlob);

  //
  // Create the list of hardware info devices filtering for PCI host
  // bridges
  //
  Status = CreateHardwareInfoList (
             HardwareInfoBlob,
             FwCfgSize,
             HardwareInfoTypeHostBridge,
             &HwInfoList
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create hardware info list to retrieve host "
      "bridges information from fw-cfg\n",
      __func__
      ));

    goto FreeBridges;
  }

  PciHostBridgeCount = GetHardwareInfoCountByType (
                         &HwInfoList,
                         HardwareInfoTypeHostBridge,
                         sizeof (HOST_BRIDGE_INFO)
                         );

  if (PciHostBridgeCount == 0) {
    goto FreeBridges;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Host provided description for %Lu root bridges\n",
    __func__,
    PciHostBridgeCount
    ));

  //
  // Allocate the root bridges
  //
  Bridges = AllocatePool (((UINTN)PciHostBridgeCount) * sizeof *Bridges);
  if (Bridges == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %r\n", __func__, EFI_OUT_OF_RESOURCES));
    goto FreeBridges;
  }

  //
  // If Host Bridges' specification was obtained from fw-cfg, the list
  // contains information to populate all root bridges in the system
  // including resources and attributes.
  //
  HwLink = GetFirstHardwareInfoByType (
             &HwInfoList,
             HardwareInfoTypeHostBridge,
             sizeof (HOST_BRIDGE_INFO)
             );

  while (!EndOfHardwareInfoList (&HwInfoList, HwLink)) {
    HwInfo = HARDWARE_INFO_FROM_LINK (HwLink);

    Status = HardwareInfoPciHostBridgeGet (
               HwInfo->Data.PciHostBridge,
               (UINTN)HwInfo->Header.Size,
               &RootBridgeNumber,
               &LastRootBridgeNumber,
               &Attributes,
               &DmaAbove4G,
               &NoExtendedConfigSpace,
               &CombineMemPMem,
               &Io,
               &Mem,
               &MemAbove4G,
               &PMem,
               &PMemAbove4G,
               NULL
               );

    if (EFI_ERROR (Status)) {
      goto FreeBridges;
    }

    if ((RootBridgeNumber > LastRootBridgeNumber) || (LastRootBridgeNumber > PCI_MAX_BUS)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: invalid bus range with BusMin %Lu and BusMax "
        "%Lu\n",
        __func__,
        (UINT64)RootBridgeNumber,
        (UINT64)LastRootBridgeNumber
        ));
      goto FreeBridges;
    }

    AllocationAttributes = 0;
    if (CombineMemPMem) {
      AllocationAttributes |= EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM;
    }

    if ((MemAbove4G.Limit > MemAbove4G.Base) ||
        (PMemAbove4G.Limit > PMemAbove4G.Base))
    {
      AllocationAttributes |= EFI_PCI_HOST_BRIDGE_MEM64_DECODE;
    }

    Status = PciHostBridgeUtilityInitRootBridge (
               Attributes,
               Attributes,
               AllocationAttributes,
               DmaAbove4G,
               NoExtendedConfigSpace,
               (UINT8)RootBridgeNumber,
               (UINT8)LastRootBridgeNumber,
               &Io,
               &Mem,
               &MemAbove4G,
               &PMem,
               &PMemAbove4G,
               &Bridges[Initialized]
               );

    if (EFI_ERROR (Status)) {
      goto FreeBridges;
    }

    ++Initialized;

    HwLink = GetNextHardwareInfoByType (
               &HwInfoList,
               HwLink,
               HardwareInfoTypeHostBridge,
               sizeof (HOST_BRIDGE_INFO)
               );
  }

  *Count = Initialized;

  //
  // If resources were allocated for host bridges info, release them
  //
  if (HardwareInfoBlob) {
    FreePool (HardwareInfoBlob);
  }

  FreeHardwareInfoList (&HwInfoList);
  return Bridges;

FreeBridges:
  while (Initialized > 0) {
    --Initialized;
    PciHostBridgeUtilityUninitRootBridge (&Bridges[Initialized]);
  }

  if (Bridges) {
    FreePool (Bridges);
  }

  if (HardwareInfoBlob) {
    FreePool (HardwareInfoBlob);
  }

  FreeHardwareInfoList (&HwInfoList);
  return NULL;
}

/**
  Utility function to return all the root bridge instances in an array.

  @param[out] Count                  The number of root bridge instances.

  @param[in]  Attributes             Initial attributes.

  @param[in]  AllocAttributes        Allocation attributes.

  @param[in]  DmaAbove4G             DMA above 4GB memory.

  @param[in]  NoExtendedConfigSpace  No Extended Config Space.

  @param[in]  BusMin                 Minimum Bus number, inclusive.

  @param[in]  BusMax                 Maximum Bus number, inclusive.

  @param[in]  Io                     IO aperture.

  @param[in]  Mem                    MMIO aperture.

  @param[in]  MemAbove4G             MMIO aperture above 4G.

  @param[in]  PMem                   Prefetchable MMIO aperture.

  @param[in]  PMemAbove4G            Prefetchable MMIO aperture above 4G.

  @return                            All the root bridge instances in an array.
**/
PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeUtilityGetRootBridges (
  OUT UINTN                     *Count,
  IN  UINT64                    Attributes,
  IN  UINT64                    AllocationAttributes,
  IN  BOOLEAN                   DmaAbove4G,
  IN  BOOLEAN                   NoExtendedConfigSpace,
  IN  UINTN                     BusMin,
  IN  UINTN                     BusMax,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Io,
  IN  PCI_ROOT_BRIDGE_APERTURE  *Mem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *MemAbove4G,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMem,
  IN  PCI_ROOT_BRIDGE_APERTURE  *PMemAbove4G
  )
{
  PCI_ROOT_BRIDGE  *Bridges;

  *Count = 0;

  //
  // First attempt to get the host provided descriptions of the Root Bridges
  // if available.
  //
  Bridges = PciHostBridgeUtilityGetRootBridgesHostProvided (Count);

  //
  // If host did not provide Root Bridge information, scan the buses and
  // auto populate them with default resources.
  //
  if (Bridges == NULL) {
    Bridges = PciHostBridgeUtilityGetRootBridgesBusScan (
                Count,
                Attributes,
                AllocationAttributes,
                DmaAbove4G,
                NoExtendedConfigSpace,
                BusMin,
                BusMax,
                Io,
                Mem,
                MemAbove4G,
                PMem,
                PMemAbove4G
                );
  }

  return Bridges;
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
  IN PCI_ROOT_BRIDGE  *Bridges,
  IN UINTN            Count
  )
{
  if ((Bridges == NULL) && (Count == 0)) {
    return;
  }

  ASSERT (Bridges != NULL && Count > 0);

  do {
    --Count;
    PciHostBridgeUtilityUninitRootBridge (&Bridges[Count]);
  } while (Count > 0);

  FreePool (Bridges);
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
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  UINTN                              RootBridgeIndex;

  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict happens!\n"));

  RootBridgeIndex = 0;
  Descriptor      = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for ( ; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (
        Descriptor->ResType <
        ARRAY_SIZE (mPciHostBridgeUtilityLibAcpiAddressSpaceTypeStr)
        );
      DEBUG ((
        DEBUG_ERROR,
        " %s: Length/Alignment = 0x%lx / 0x%lx\n",
        mPciHostBridgeUtilityLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
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
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                                                       (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                                                       );
  }
}
