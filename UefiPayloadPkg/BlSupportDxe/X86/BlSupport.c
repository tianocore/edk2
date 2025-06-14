/** @file
  This file will report some MMIO/IO resources to dxe core.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright 2025 Google LLC
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BlSupportDxe.h"

/**
  Reserve MMIO/IO resource in GCD

  @param  IsMMIO        Flag of whether it is mmio resource or io resource.
  @param  GcdType       Type of the space.
  @param  BaseAddress   Base address of the space.
  @param  Length        Length of the space.
  @param  Alignment     Align with 2^Alignment
  @param  ImageHandle   Handle for the image of this driver.

  @retval EFI_SUCCESS   Reserve successful
**/
EFI_STATUS
ReserveResourceInGcd (
  IN BOOLEAN               IsMMIO,
  IN UINTN                 GcdType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINTN                 Alignment,
  IN EFI_HANDLE            ImageHandle
  )
{
  EFI_STATUS  Status;

  if (IsMMIO) {
    Status = gDS->AddMemorySpace (
                    GcdType,
                    BaseAddress,
                    Length,
                    EFI_MEMORY_UC
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "Failed to add memory space :0x%lx 0x%lx\n",
        BaseAddress,
        Length
        ));
    }

    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
  } else {
    Status = gDS->AddIoSpace (
                    GcdType,
                    BaseAddress,
                    Length
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_WARN,
        "Failed to add IO space :0x%lx 0x%lx\n",
        BaseAddress,
        Length
        ));
    }

    Status = gDS->AllocateIoSpace (
                    EfiGcdAllocateAddress,
                    GcdType,
                    Alignment,
                    Length,
                    &BaseAddress,
                    ImageHandle,
                    NULL
                    );
  }

  return Status;
}

/**
  Architecture level additional operation which needs to be performed before
  launching payload.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BlArchAdditionalOps (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Report MMIO/IO Resources
  //
  ReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFEC00000, SIZE_4KB, 0, ImageHandle); // IOAPIC

  ReserveResourceInGcd (TRUE, EfiGcdMemoryTypeMemoryMappedIo, 0xFED00000, SIZE_1KB, 0, ImageHandle); // HPET

  return EFI_SUCCESS;
}
