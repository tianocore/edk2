/** @file
  Public interface for FixedBars.c -- fixed PCI BAR placement using
  metadata provided by QEMU through the "etc/fixed-bars" fw_cfg blob.

  Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FIXED_BARS_H_
#define FIXED_BARS_H_

#include <Uefi.h>

/**
  Read and parse the "etc/fixed-bars" fw_cfg blob (if present), and reserve
  the fixed BAR address ranges it describes in the GCD memory space map.

  Must be called once, from the owning driver's entry point, before that
  driver installs EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_PROTOCOL.

  @param[out] HasFixedBarsDevices  TRUE if the blob was present and
                                    described at least one fixed-BAR device;
                                    FALSE otherwise (nothing else in this
                                    file has any effect in that case).

  @retval EFI_SUCCESS           Blob absent, empty, or successfully parsed.
  @retval EFI_UNSUPPORTED       Blob present but malformed/wrong version.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failure.
**/
EFI_STATUS
FixedBarsInit (
  OUT BOOLEAN  *HasFixedBarsDevices
  );

/**
  Return the fixed-BAR ACPI address-space descriptors for one PCI function,
  if it has a matching entry in the "etc/fixed-bars" blob. Returns
  *Descriptors == NULL, with *DescriptorsSize == 0, when the device is not
  a fixed-BAR device.

  @param[in]  VendorId        Device's PCI Vendor ID.
  @param[in]  DeviceId        Device's PCI Device ID.
  @param[out] Descriptors     Heap-allocated array of
                              EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR, one per
                              fixed BAR; caller takes ownership. NULL if no
                              match.
  @param[out] DescriptorsSize Size in bytes of *Descriptors.

  @retval EFI_SUCCESS           Always, on non-allocation-failure paths
                                (matches EFI_INCOMPATIBLE_PCI_DEVICE_SUPPORT_
                                PROTOCOL.CheckDevice()'s own contract).
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failure.
**/
EFI_STATUS
FixedBarsCheckDevice (
  IN  UINTN  VendorId,
  IN  UINTN  DeviceId,
  OUT VOID   **Descriptors,
  OUT UINTN  *DescriptorsSize
  );

#endif
