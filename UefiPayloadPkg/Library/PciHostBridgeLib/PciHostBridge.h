/** @file
  Header file of PciHostBridgeLib.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2016 - 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCI_HOST_BRIDGE_H_
#define PCI_HOST_BRIDGE_H_

#include <UniversalPayload/PciRootBridges.h>

typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} CB_PCI_ROOT_BRIDGE_DEVICE_PATH;

/**
  Scan for all root bridges in platform.

  @param[out] NumberOfRootBridges  Number of root bridges detected

  @retval     Pointer to the allocated PCI_ROOT_BRIDGE structure array.
**/
PCI_ROOT_BRIDGE *
ScanForRootBridges (
  OUT UINTN      *NumberOfRootBridges
);

/**
  Scan for all root bridges from Universal Payload PciRootBridgeInfoHob

  @param[in]  PciRootBridgeInfo    Pointer of Universal Payload PCI Root Bridge Info Hob
  @param[out] NumberOfRootBridges  Number of root bridges detected

  @retval     Pointer to the allocated PCI_ROOT_BRIDGE structure array.

**/
PCI_ROOT_BRIDGE *
RetrieveRootBridgeInfoFromHob (
  IN  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES  *PciRootBridgeInfo,
  OUT UINTN                               *NumberOfRootBridges
);

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
);

/**
  Initialize DevicePath for a PCI_ROOT_BRIDGE.
  @param[in] HID               HID for device path
  @param[in] UID               UID for device path

  @retval A pointer to the new created device patch.
**/
EFI_DEVICE_PATH_PROTOCOL *
CreateRootBridgeDevicePath (
  IN     UINT32                   HID,
  IN     UINT32                   UID
);
#endif
