/** @file
  Library constructor for FMP Authentication PKCS7 at runtime.

  Copyright (c) 2025, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "FmpAuthenticationCommon.h"

STATIC EFI_EVENT  mFmpAuthenticateLibExitBootServiceEvent;

/**
  Notify function for event of exit boot service.

  @param[in]  Event    The Event that is being processed.
  @param[in]  Context  The Event Context.

**/
STATIC
VOID
EFIAPI
FmpAuthenticationLibBootServiceEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mFmpAuthenticationExitBootServiceSignalled = TRUE;
}

/**
  The constructor function for the file of FmpAuthenticationRuntimeLibPkcs7.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully .

**/
EFI_STATUS
EFIAPI
FmpAuthenticationRuntimeLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Register notify function to indicate the event is signaled at ExitBootService.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  FmpAuthenticationLibBootServiceEventNotify,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mFmpAuthenticateLibExitBootServiceEvent
                  );

  return Status;
}
