/** @file
  Provide common utility functions to PciHostBridgeLib instances in
  ArmVirtPkg and OvmfPkg.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, Huawei Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PCI_HOST_BRIDGE_UTILITY_LIB_H__
#define __PCI_HOST_BRIDGE_UTILITY_LIB_H__

#include <Library/PciHostBridgeLib.h>

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
  );

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
  );

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
  );

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
  );

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
  );

#endif // __PCI_HOST_BRIDGE_UTILITY_LIB_H__
