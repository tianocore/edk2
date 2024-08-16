/** @file
  Platform Device Info Library

  Copyright (c) 2024 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Base.h>
#include <Uefi/UefiBaseType.h>

/** A structure that contains a brief description of the
    device, the Base address and range.
**/
typedef struct PlatformDeviceInfo {
  /// Brief description of the device.
  CHAR8     Desc[16];
  /// The MMIO Base address.
  UINT64    BaseAddress;
  /// The base address range.
  UINT64    Length;
} PLATFORM_DEVICE_INFO;

/** Parse the platform FDT and populate the platform device info.

  @param [in]       FdtBase     Pointer to the platform FDT.
  @param [in]       PlatDevInfo Pointer to the platform device info array to
                                populate.
  @param [in, out]  MaxDevices  Size of the platform device info array on input,
                                and maximum devices populated in the platform
                                device info array on output.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Parser did not find any device information
                                  in the FDT.
**/
EFI_STATUS
EFIAPI
ParsePlatformDeviceFdt (
  IN       VOID                  *FdtBase,
  IN       PLATFORM_DEVICE_INFO  *PlatDevInfo,
  IN OUT   UINTN                 *MaxDevices
  );
