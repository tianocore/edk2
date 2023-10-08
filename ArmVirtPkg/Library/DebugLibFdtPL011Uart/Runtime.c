/** @file
  Permanently disable the library instance in DXE_RUNTIME_DRIVER modules when
  exiting boot services.

  Copyright (C) Red Hat
  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiSpec.h>

#include "Ram.h"

STATIC EFI_EVENT  mExitBootServicesEvent;

/**
  Notification function that is triggered when the boot service
  ExitBootServices() is called.

  @param[in] Event    Event whose notification function is being invoked. Here,
                      unused.

  @param[in] Context  The pointer to the notification function's context, which
                      is implementation-dependent. Here, unused.
**/
STATIC
VOID
EFIAPI
ExitBootServicesNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mDebugLibFdtPL011UartAddress         = 0;
  mDebugLibFdtPL011UartPermanentStatus = RETURN_ABORTED;
}

/**
  Library instance constructor, registering ExitBootServicesNotify().

  @param[in] ImageHandle  The firmware-allocated handle for the EFI image.

  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The operation completed successfully.

  @return              Error codes propagated from CreateEvent(); the
                       registration of ExitBootServicesNotify() failed.
**/
EFI_STATUS
EFIAPI
DxeRuntimeDebugLibFdtPL011UartConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return SystemTable->BootServices->CreateEvent (
                                      EVT_SIGNAL_EXIT_BOOT_SERVICES,
                                      TPL_CALLBACK,
                                      ExitBootServicesNotify,
                                      NULL /* NotifyContext */,
                                      &mExitBootServicesEvent
                                      );
}

/**
  Library instance destructor, deregistering ExitBootServicesNotify().

  @param[in] ImageHandle  The firmware-allocated handle for the EFI image.

  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  Library instance tear-down complete.

  @return              Error codes propagated from CloseEvent(); the
                       deregistration of ExitBootServicesNotify() failed.
**/
EFI_STATUS
EFIAPI
DxeRuntimeDebugLibFdtPL011UartDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return SystemTable->BootServices->CloseEvent (mExitBootServicesEvent);
}
