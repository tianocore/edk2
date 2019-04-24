/** @file
  Status Code Handler Driver which produces datahub handler and hook it
  onto the DXE status code router.

  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DatahubStatusCodeHandlerDxe.h"

static EFI_EVENT          mExitBootServicesEvent     = NULL;
EFI_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol       = NULL;

/**
  Unregister status code callback functions only available at boot time from
  report status code router when exiting boot services.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
UnregisterBootTimeHandlers (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  mRscHandlerProtocol->Unregister (DataHubStatusCodeReportWorker);
}

/**
  Entry point of DXE Status Code Driver.

  This function is the entry point of this DXE Status Code Driver.
  It initializes registers status code handlers, and registers event for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
DatahubStatusCodeHandlerDxeEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  if (FeaturePcdGet (PcdStatusCodeUseDataHub)) {
    Status = gBS->LocateProtocol (
                    &gEfiRscHandlerProtocolGuid,
                    NULL,
                    (VOID **) &mRscHandlerProtocol
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Dispatch initialization request to supported devices
    //
    DataHubStatusCodeInitializeWorker ();

    mRscHandlerProtocol->Register (DataHubStatusCodeReportWorker, TPL_HIGH_LEVEL);

    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    UnregisterBootTimeHandlers,
                    NULL,
                    &gEfiEventExitBootServicesGuid,
                    &mExitBootServicesEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
