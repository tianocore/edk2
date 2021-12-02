/** @file
  This file defines the structure for the PCI Root Bridges.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.75 (https://universalpayload.github.io/documentation/)
**/

#ifndef UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_H_
#define UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_H_

#include <UniversalPayload/UniversalPayload.h>

#pragma pack(1)

//
// (Base > Limit) indicates an aperture is not available.
//
typedef struct {
  //
  // Base and Limit are the device address instead of host address when
  // Translation is not zero
  //
  UINT64    Base;
  UINT64    Limit;
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
  UINT64    Translation;
} UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE;

///
/// Payload PCI Root Bridge Information HOB
///
typedef struct {
  UINT32                                        Segment;               ///< Segment number.
  UINT64                                        Supports;              ///< Supported attributes.
                                                                       ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                                       ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  UINT64                                        Attributes;            ///< Initial attributes.
                                                                       ///< Refer to EFI_PCI_ATTRIBUTE_xxx used by GetAttributes()
                                                                       ///< and SetAttributes() in EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL.
  BOOLEAN                                       DmaAbove4G;            ///< DMA above 4GB memory.
                                                                       ///< Set to TRUE when root bridge supports DMA above 4GB memory.
  BOOLEAN                                       NoExtendedConfigSpace; ///< When FALSE, the root bridge supports
                                                                       ///< Extended (4096-byte) Configuration Space.
                                                                       ///< When TRUE, the root bridge supports
                                                                       ///< 256-byte Configuration Space only.
  UINT64                                        AllocationAttributes;  ///< Allocation attributes.
                                                                       ///< Refer to EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM and
                                                                       ///< EFI_PCI_HOST_BRIDGE_MEM64_DECODE used by GetAllocAttributes()
                                                                       ///< in EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL.
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE    Bus;                   ///< Bus aperture which can be used by the root bridge.
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE    Io;                    ///< IO aperture which can be used by the root bridge.
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE    Mem;                   ///< MMIO aperture below 4GB which can be used by the root bridge.
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE    MemAbove4G;            ///< MMIO aperture above 4GB which can be used by the root bridge.
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE    PMem;                  ///< Prefetchable MMIO aperture below 4GB which can be used by the root bridge.
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE_APERTURE    PMemAbove4G;           ///< Prefetchable MMIO aperture above 4GB which can be used by the root bridge.
  UINT32                                        HID;                   ///< PnP hardware ID of the root bridge. This value must match the corresponding
                                                                       ///< _HID in the ACPI name space.
  UINT32                                        UID;                   ///< Unique ID that is required by ACPI if two devices have the same _HID.
                                                                       ///< This value must also match the corresponding _UID/_HID pair in the ACPI name space.
} UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE;

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER     Header;
  BOOLEAN                              ResourceAssigned;
  UINT8                                Count;
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE    RootBridge[0];
} UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES;

#pragma pack()

#define UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_REVISION  1

extern GUID  gUniversalPayloadPciRootBridgeInfoGuid;

#endif // UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGES_H_
