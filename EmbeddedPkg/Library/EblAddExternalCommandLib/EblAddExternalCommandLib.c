/** @file
  Add external EblCmd Lib

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/EblAddExternalCommandLib.h>
#include <Protocol/EblAddCommand.h>

STATIC BOOLEAN   gInstalledCommand = FALSE;
STATIC EFI_EVENT mEblCommandRegistration = NULL;

STATIC const EBL_COMMAND_TABLE *mAddExternalCmdLibTemplate = NULL;
STATIC UINTN                   mAddExternalCmdLibTemplateSize = 0;
EBL_ADD_COMMAND_PROTOCOL      *gEblExternalCommand = NULL;


/**
  Return a keypress or optionally timeout if a timeout value was passed in.
  An optional callback function is called every second when waiting for a
  timeout.

  @param  Key           EFI Key information returned
  @param  TimeoutInSec  Number of seconds to wait to timeout
  @param  CallBack      Callback called every second during the timeout wait

  @return EFI_SUCCESS  Key was returned
  @return EFI_TIMEOUT  If the TimoutInSec expired

**/
EFI_STATUS
EFIAPI
EblGetCharKey (
  IN OUT EFI_INPUT_KEY            *Key,
  IN     UINTN                    TimeoutInSec,
  IN     EBL_GET_CHAR_CALL_BACK   CallBack   OPTIONAL
  )
{
  if (gEblExternalCommand != NULL) {
    return gEblExternalCommand->EblGetCharKey (Key, TimeoutInSec, CallBack);
  }
  return EFI_TIMEOUT;
}


/**
  This routine is used prevent command output data from scrolling off the end
  of the screen. The global gPageBreak is used to turn on or off this feature.
  If the CurrentRow is near the end of the screen pause and print out a prompt
  If the use hits Q to quit return TRUE else for any other key return FALSE.
  PrefixNewline is used to figure out if a newline is needed before the prompt
  string. This depends on the last print done before calling this function.
  CurrentRow is updated by one on a call or set back to zero if a prompt is
  needed.

  @param  CurrentRow  Used to figure out if its the end of the page and updated
  @param  PrefixNewline  Did previous print issue a newline

  @return TRUE if Q was hit to quit, FALSE in all other cases.

**/
BOOLEAN
EFIAPI
EblAnyKeyToContinueQtoQuit (
  IN  UINTN   *CurrentRow,
  IN  BOOLEAN PrefixNewline
  )
{
  if (gEblExternalCommand != NULL) {
    return gEblExternalCommand->EblAnyKeyToContinueQtoQuit (CurrentRow, PrefixNewline);
  }
  return FALSE;
}



/**
  Update mFvbEntry. Add new entry, or update existing entry if Fvb protocol is
  reinstalled.

  @param Event      The Event that is being processed
  @param Context    Event Context

**/
VOID
EFIAPI
EblAddCommandNotificationEvent (
  IN  EFI_EVENT       Event,
  IN  VOID            *Context
  )
{
  EFI_STATUS                  Status;

  if (!gInstalledCommand) {
    Status = gBS->LocateProtocol (&gEfiEblAddCommandProtocolGuid, NULL, (VOID **)&gEblExternalCommand);
    if (!EFI_ERROR (Status)) {
      gEblExternalCommand->AddCommands (mAddExternalCmdLibTemplate, mAddExternalCmdLibTemplateSize);
      gInstalledCommand = TRUE;
    }
  }
}



/**
  The user Entry Point for the driver. The user code starts with this function
  as the real entry point for the image goes into a library that calls this
  function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
EblAddExternalCommands (
  IN const EBL_COMMAND_TABLE   *EntryArray,
  IN UINTN                     ArrayCount
  )
{
  if (mAddExternalCmdLibTemplate != NULL) {
    return EFI_ALREADY_STARTED;
  }

  mAddExternalCmdLibTemplate     = EntryArray;
  mAddExternalCmdLibTemplateSize = ArrayCount;

  EfiCreateProtocolNotifyEvent (
    &gEfiEblAddCommandProtocolGuid,
    TPL_CALLBACK,
    EblAddCommandNotificationEvent,
    NULL,
    &mEblCommandRegistration
    );

  return EFI_SUCCESS;
}

