/** @file

  Copyright (c) 2004  - 2016, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



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

