/** @file

  Copyright (c) 2004  - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent



**/

#include <PiDxe.h>

#include <Library/FlashDeviceLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include "SpiChipDefinitions.h"

extern UINTN FlashDeviceBase;

extern EFI_SPI_PROTOCOL *mSpiProtocol;

/**
  The library constructuor.

  The function does the necessary initialization work for this library
  instance. Please put all initialization works in it.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       The function always return EFI_SUCCESS for now.
                                It will ASSERT on error for debug version.
  @retval     EFI_ERROR         Please reference LocateProtocol for error code details.

**/
EFI_STATUS
EFIAPI
LibFvbFlashDeviceSupportInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                  (VOID **)&mSpiProtocol
                  );
  ASSERT_EFI_ERROR (Status);
  // There is no need to call Init, because Runtime or SMM FVB already does that.
  DEBUG((EFI_D_ERROR, "LibFvbFlashDeviceSupportInit - no init\n"));
  return EFI_SUCCESS;
}

