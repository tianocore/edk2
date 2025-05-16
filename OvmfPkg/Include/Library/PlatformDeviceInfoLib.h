/** @file
  Platform Device Info Library

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_DEVICE_INFO_LIB_H_
#define PLATFORM_DEVICE_INFO_LIB_H_

#include <Base.h>
#include <Uefi/UefiBaseType.h>

/* Maximum number of platform devices that can be discovered from a FDT.
*/
#define MAX_PLAT_DEVICE_COUNT  FixedPcdGet64 (PcdMaxPlatDeviceCount)

/** A DEVICE_INFO structure that contains a brief description of the
    device and the Base address and range.
**/
typedef struct DeviceInfo {
  /// Brief description of the device.
  CHAR8     Desc[16];
  /// The MMIO Base address.
  UINT64    BaseAddress;
  /// The base address range.
  UINT64    Length;
} DEVICE_INFO;

/** A structure that contains the information about the devices available
    on a platform.
**/
typedef struct PlatDevInfo {
  /// An array containing the information about the platform devices.
  DEVICE_INFO    Dev[MAX_PLAT_DEVICE_COUNT];
  /// The max count of devices.
  UINTN          MaxDevices;
} PLATFORM_DEVICE_INFO;

/** Parse the platform FDT and populate the platform device info.

  @param [in]  FdtBase     Pointer to the platform FDT.
  @param [in]  PlatInfo    Pointer to the platform device info to populate.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           Parser did not find any device information
                                  in the FDT.
**/
EFI_STATUS
EFIAPI
ParsePlatformDeviceFdt (
  IN  VOID                  *FdtBase,
  IN  PLATFORM_DEVICE_INFO  *PlatInfo
  );

#endif // PLATFORM_DEVICE_INFO_LIB_H_
