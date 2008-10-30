/** @file
  UEFI Runtime Library implementation for non IPF processor types.

Copyright (c) 2006 - 2008 Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <RuntimeLibInternal.h>

///
/// Driver Lib Module Globals
///

EFI_EVENT              mEfiVirtualNotifyEvent;
EFI_EVENT              mEfiExitBootServicesEvent;
BOOLEAN                mEfiGoneVirtual         = FALSE;
BOOLEAN                mEfiAtRuntime           = FALSE;
EFI_RUNTIME_SERVICES          *mRT;

/**
  Set AtRuntime flag as TRUE after ExitBootServices.

  @param[in]  Event   The Event that is being processed
  @param[in]  Context Event Context
**/
VOID
EFIAPI
RuntimeLibExitBootServicesEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  //
  // Clear out BootService globals
  //
  gBS             = NULL;

  mEfiAtRuntime = TRUE;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
RuntimeLibVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  //
  // Update global for Runtime Services Table and IO
  //
  EfiConvertPointer (0, (VOID **) &mRT);

  mEfiGoneVirtual = TRUE;
}

/**
  Intialize runtime Driver Lib if it has not yet been initialized.
  It will ASSERT() if gRT is NULL or gBS is NULL.
  It will ASSERT() if that operation fails.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @return     EFI_STATUS    always returns EFI_SUCCESS except EFI_ALREADY_STARTED if already started.
**/
EFI_STATUS
EFIAPI
RuntimeDriverLibConstruct (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS  Status;

  ASSERT (gRT != NULL);
  ASSERT (gBS != NULL);

  mRT = gRT;
  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  RuntimeLibVirtualNotifyEvent,
                  NULL,
                  &mEfiVirtualNotifyEvent
                  );

  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  RuntimeLibExitBootServicesEvent,
                  NULL,
                  &mEfiExitBootServicesEvent
                  );

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  If a runtime driver exits with an error, it must call this routine 
  to free the allocated resource before the exiting.
  It will ASSERT() if gBS is NULL.
  It will ASSERT() if that operation fails.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval     EFI_SUCCESS       Shutdown the Runtime Driver Lib successfully
  @retval     EFI_UNSUPPORTED   Runtime Driver lib was not initialized at all
**/
EFI_STATUS
EFIAPI
RuntimeDriverLibDeconstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close SetVirtualAddressMap () notify function
  //
  ASSERT (gBS != NULL);
  Status = gBS->CloseEvent (mEfiVirtualNotifyEvent);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CloseEvent (mEfiExitBootServicesEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Return TRUE if ExitBootServices () has been called.

  @retval TRUE If ExitBootServices () has been called
**/
BOOLEAN
EFIAPI
EfiAtRuntime (
  VOID
  )
{
  return mEfiAtRuntime;
}

/**
  Return TRUE if SetVirtualAddressMap () has been called.

  @retval TRUE  If SetVirtualAddressMap () has been called
**/
BOOLEAN
EFIAPI
EfiGoneVirtual (
  VOID
  )
{
  return mEfiGoneVirtual;
}

