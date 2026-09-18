/** @file
  UART Serial Port library functions.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

extern BOOLEAN  mBaseSerialPortLibHobAtRuntime;

STATIC EFI_EVENT  mBaseSerialPortLibHobExitBootServicesEvent;

/**
  Set mSerialIoUartLibAtRuntime flag as TRUE after ExitBootServices.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
STATIC
VOID
EFIAPI
BaseSerialPortLibHobExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mBaseSerialPortLibHobAtRuntime = TRUE;
}

/**
  The constructor function registers a callback for the ExitBootServices event.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The operation completed successfully.
  @retval other         Either the serial port failed to initialize or the
                        ExitBootServices event callback registration failed.
**/
EFI_STATUS
EFIAPI
DxeBaseSerialPortLibHobConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return SystemTable->BootServices->CreateEvent (
                                      EVT_SIGNAL_EXIT_BOOT_SERVICES,
                                      TPL_NOTIFY,
                                      BaseSerialPortLibHobExitBootServicesEvent,
                                      NULL,
                                      &mBaseSerialPortLibHobExitBootServicesEvent
                                      );
}

/**
  The destructor closes the ExitBootServices event.

  A driver that fails its entry point is unloaded again by the DXE core, but
  this library has already registered its ExitBootServices callback by then.
  Close the event in the destructor so that a stale notification function
  pointing into the unloaded image is not left behind.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   No event was registered, or it was closed.
  @retval other         CloseEvent () failed.
**/
EFI_STATUS
EFIAPI
DxeBaseSerialPortLibHobDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (mBaseSerialPortLibHobExitBootServicesEvent == NULL) {
    return EFI_SUCCESS;
  }

  Status = SystemTable->BootServices->CloseEvent (mBaseSerialPortLibHobExitBootServicesEvent);
  if (!EFI_ERROR (Status)) {
    mBaseSerialPortLibHobExitBootServicesEvent = NULL;
  }

  return Status;
}
