/** @file
  OVMF's instance of the PCI Host Bridge Library, for Bhyve and Xen guests.

  Copyright (C) 2016-2021, Red Hat, Inc.
  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/PciHostBridgeLib.h>                 // PCI_ROOT_BRIDGE
#include <Library/PciHostBridgeUtilityLib.h>          // PciHostBridgeUtilit...

#include "PciHostBridge.h"

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
  UINTN *Count
  )
{
  return ScanForRootBridges (Count);
}


/**
  Free the root bridge instances array returned from
  PciHostBridgeGetRootBridges().

  @param  The root bridge instances array.
  @param  The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE *Bridges,
  UINTN           Count
  )
{
  PciHostBridgeUtilityFreeRootBridges (Bridges, Count);
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
  EFI_HANDLE                        HostBridgeHandle,
  VOID                              *Configuration
  )
{
  PciHostBridgeUtilityResourceConflict (Configuration);
}
