/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

/**
  Notification function for keystrokes.

  @param[in] KeyData    The key that was pressed.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
NotificationFunction (
  IN EFI_KEY_DATA  *KeyData
  )
{
  if (((KeyData->Key.UnicodeChar == L'c') &&
       ((KeyData->KeyState.KeyShiftState == (EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED)) || (KeyData->KeyState.KeyShiftState  == (EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED)))) ||
      (KeyData->Key.UnicodeChar == 3)
      )
  {
    if (ShellProtocolsInfoObject.NewEfiShellProtocol->ExecutionBreak == NULL) {
      return (EFI_UNSUPPORTED);
    }

    return (gBS->SignalEvent (ShellProtocolsInfoObject.NewEfiShellProtocol->ExecutionBreak));
  } else if ((KeyData->Key.UnicodeChar == L's') &&
             ((KeyData->KeyState.KeyShiftState  == (EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED)) || (KeyData->KeyState.KeyShiftState  == (EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED)))
             )
  {
    ShellProtocolInteractivityInfoObject.HaltOutput = TRUE;
  }

  return (EFI_SUCCESS);
}

/**
  Function to start monitoring for CTRL-C using SimpleTextInputEx.  This
  feature's enabled state was not known when the shell initially launched.

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available.
**/
EFI_STATUS
InernalEfiShellStartMonitor (
  VOID
  )
{
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleEx;
  EFI_KEY_DATA                       KeyData;
  EFI_STATUS                         Status;

  Status = gBS->OpenProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleEx,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_SHELL_NO_IN_EX),
      ShellInfoObject.HiiHandle
      );
    return (EFI_SUCCESS);
  }

  if (ShellProtocolsInfoObject.NewEfiShellProtocol->ExecutionBreak == NULL) {
    return (EFI_UNSUPPORTED);
  }

  KeyData.KeyState.KeyToggleState = 0;
  KeyData.Key.ScanCode            = 0;
  KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar         = L'c';

  Status = SimpleEx->RegisterKeyNotify (
                       SimpleEx,
                       &KeyData,
                       NotificationFunction,
                       &ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle1
                       );

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle2
                         );
  }

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
  KeyData.Key.UnicodeChar        = 3;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle3
                         );
  }

  KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->RegisterKeyNotify (
                         SimpleEx,
                         &KeyData,
                         NotificationFunction,
                         &ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle4
                         );
  }

  return (Status);
}
