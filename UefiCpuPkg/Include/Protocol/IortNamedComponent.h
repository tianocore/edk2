/** @file IortNamedComponent.h

  IORT Named Component protocol.

  Identifies the IORT Named Component node that describes the device behind
  an EFI_HANDLE. Installed by a DMA-capable driver on the same EFI_HANDLE it
  passes to EDKII_IOMMU_PROTOCOL.SetAttribute() when that handle cannot be
  resolved through PCI enumeration (e.g. NonDiscoverable devices or
  firmware-internal DMA agents). An IORT-aware IOMMU driver calls
  GetDeviceObjectName() and walks the IORT for the matching node to recover
  the device's StreamID(s) and owning SMMUv3.

  See <https://developer.arm.com/documentation/den0049/latest/> for IORT spec.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#define IORT_NAMED_COMPONENT_PROTOCOL_GUID \
  { 0x4c3c50a3, 0xfaf3, 0x403e, { 0x94, 0x70, 0x75, 0x66, 0x54, 0x30, 0x58, 0x4c } }

#define IORT_NAMED_COMPONENT_PROTOCOL_REVISION  0x00010000

typedef struct _IORT_NAMED_COMPONENT_PROTOCOL IORT_NAMED_COMPONENT_PROTOCOL;

/**
  Return the Device Object Name of the IORT Named Component node that
  describes the device owning this protocol instance.

  @param[in]   This        Protocol instance.
  @param[out]  ObjectName  Receives a NUL-terminated ACPI namespace path
                           (e.g. "\\_SB_.USB0") matching the Device Object Name
                           of an IORT Named Component node. The string is owned
                           by the producer and must stay valid for the lifetime
                           of the protocol instance.

  @retval EFI_SUCCESS            ObjectName returned.
  @retval EFI_INVALID_PARAMETER  This or ObjectName is NULL.
  @retval EFI_NOT_FOUND          No IORT Named Component describes this device.
**/
typedef
EFI_STATUS
(EFIAPI *IORT_NAMED_COMPONENT_GET_DEVICE_OBJECT_NAME)(
  IN  IORT_NAMED_COMPONENT_PROTOCOL  *This,
  OUT CONST CHAR8                    **ObjectName
  );

struct _IORT_NAMED_COMPONENT_PROTOCOL {
  UINT64                                         Revision;
  IORT_NAMED_COMPONENT_GET_DEVICE_OBJECT_NAME    GetDeviceObjectName;
};

extern EFI_GUID  gIortNamedComponentProtocolGuid;
