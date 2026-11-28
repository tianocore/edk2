/** @file
  Common wrapper for prepared device-tree resources.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>

typedef struct _UFDT_RESOURCE_PROTOCOL UFDT_RESOURCE_PROTOCOL;

/** Map the selected device's MMIO regions before hardware access.

  Apply the memory attributes required by this device's consumers. Repeated
  calls after success return EFI_SUCCESS without remapping. The producer records
  success in the snapshot's UFDT_COMMON header. A failure leaves MmioMapped FALSE
  and may leave some regions mapped; the caller
  must not access the device until a subsequent call succeeds.
  Call only before ExitBootServices().

  @param[in] This  Published resource interface containing the device snapshot.

  @retval EFI_SUCCESS  Success.
  @retval Other        MMIO mapping or resource validation errors.
**/
typedef
EFI_STATUS
(EFIAPI *UFDT_MAP_MMIO)(
  IN UFDT_RESOURCE_PROTOCOL *This
  );

/** Interface installed under a device-specific resource protocol GUID.

  The protocol GUID determines the type of Info. Cast Info, not the interface.
  The producer owns both allocations; consumers must not modify or free them.
  Both remain valid until ExitBootServices(). Runtime consumers must copy any
  required values into their own runtime storage.

  Publication signals prepared resources and any required legacy PCD setup.
  Consumers must call MapMmio, when present, before accessing the hardware.
  The callback and interface are valid only until ExitBootServices().
**/
struct _UFDT_RESOURCE_PROTOCOL {
  /// Device-specific snapshot, including producer-owned mapping state.
  VOID             *Info;
  /// Optional boot-services callback; NULL when no MMIO mapping is required.
  UFDT_MAP_MMIO    MapMmio;
};
