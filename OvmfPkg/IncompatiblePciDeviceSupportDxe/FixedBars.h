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
  Read the "etc/fixed-bars" fw_cfg blob (if present) and reserve the fixed BAR
  address ranges it describes.

  @param[out] HasFixedBarsDevices  TRUE if the blob describes at least one
                                   fixed BAR device.

  @retval EFI_SUCCESS           Blob absent, empty, or parsed.
  @retval EFI_UNSUPPORTED       Blob malformed or of an unsupported version.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failure.
**/
EFI_STATUS
FixedBarsInit (
  OUT BOOLEAN  *HasFixedBarsDevices
  );

/**
  Return the fixed BAR descriptors for one PCI function, if the blob has an
  entry for it.

  @param[in]  VendorId         PCI Vendor ID.
  @param[in]  DeviceId         PCI Device ID.
  @param[out] Descriptors      Array of ACPI address space descriptors, one per
                               fixed BAR, or NULL if the device has none.
  @param[out] DescriptorsSize  Size in bytes of Descriptors.

  @retval EFI_SUCCESS           Descriptors are returned, or none are needed.
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
