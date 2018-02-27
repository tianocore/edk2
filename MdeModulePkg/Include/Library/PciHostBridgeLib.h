/** @file
  PCI Host Bridge Library consumed by PciHostBridgeDxe driver returning
  the platform specific information about the PCI Host Bridge.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are
  licensed and made available under the terms and conditions of
  the BSD License which accompanies this distribution.  The full
  text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __PCI_HOST_BRIDGE_LIB_H__
#define __PCI_HOST_BRIDGE_LIB_H__

//
// (Base > Limit) indicates an aperture is not available.
//
typedef struct {
  //
  // Base and Limit are the device address instead of host address when
  // Translation is not zero
  //
  UINT64 Base;
  UINT64 Limit;
  //
  // According to UEFI 2.7, Device Address = Host Address + Translation,
  // so Translation = Device Address - Host Address.
  // On platforms where Translation is not zero, the subtraction is probably to
  // be performed with UINT64 wrap-around semantics, for we may translate an
  // above-4G host address into a below-4G device address for legacy PCIe device
  // compatibility.
  //
  // NOTE: The alignment of Translation is required to be larger than any BAR
  // alignment in the same root bridge, so that the same alignment can be
  // applied to both device address and host address, which simplifies the
  // situation and makes the current resource allocation code in generic PCI
  // host bridge driver still work.
  //
  UINT64 Translation;
} PCI_ROOT_BRIDGE_APERTURE;

typedef struct {
  UINT32                   Segment;               ///< Segment number.
  UINT64                   Supports;              ///< Supported attributes.
                                                  ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                  ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  UINT64                   Attributes;            ///< Initial attributes.
                                                  ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                  ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  BOOLEAN                  DmaAbove4G;            ///< DMA above 4GB memory.
                                                  ///< Set to TRUE when root bridge supports DMA above 4GB memory.
  BOOLEAN                  NoExtendedConfigSpace; ///< When FALSE, the root bridge supports
                                                  ///< Extended (4096-byte) Configuration Space.
                                                  ///< When TRUE, the root bridge supports
                                                  ///< 256-byte Configuration Space only.
  BOOLEAN                  ResourceAssigned;      ///< Resource assignment status of the root bridge.
                                                  ///< Set to TRUE if Bus/IO/MMIO resources for root bridge have been assigned.
  UINT64                   AllocationAttributes;  ///< Allocation attributes.
                                                  ///< Refer to EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM and
                                                  ///< EFI_PCI_HOST_BRIDGE_MEM64_DECODE used by GetAllocAttributes()
                                                  ///< in EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  PCI_ROOT_BRIDGE_APERTURE Bus;                   ///< Bus aperture which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE Io;                    ///< IO aperture which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE Mem;                   ///< MMIO aperture below 4GB which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE MemAbove4G;            ///< MMIO aperture above 4GB which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE PMem;                  ///< Prefetchable MMIO aperture below 4GB which can be used by the root bridge.
  PCI_ROOT_BRIDGE_APERTURE PMemAbove4G;           ///< Prefetchable MMIO aperture above 4GB which can be used by the root bridge.
  EFI_DEVICE_PATH_PROTOCOL *DevicePath;           ///< Device path.
} PCI_ROOT_BRIDGE;

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
  );

/**
  Free the root bridge instances array returned from PciHostBridgeGetRootBridges().

  @param Bridges The root bridge instances array.
  @param Count   The count of the array.
**/
VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE *Bridges,
  UINTN           Count
  );

/**
  Inform the platform that the resource conflict happens.

  @param HostBridgeHandle Handle of the Host Bridge.
  @param Configuration    Pointer to PCI I/O and PCI memory resource descriptors.
                          The Configuration contains the resources for all the
                          root bridges. The resource for each root bridge is
                          terminated with END descriptor and an additional END
                          is appended indicating the end of the entire resources.
                          The resource descriptor field values follow the description
                          in EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.SubmitResources().
**/
VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE           HostBridgeHandle,
  VOID                 *Configuration
  );

#endif
