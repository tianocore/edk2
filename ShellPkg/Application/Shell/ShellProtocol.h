/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_PROTOCOL_HEADER_
#define _SHELL_PROTOCOL_HEADER_

#include <Uefi.h>
#include <Protocol/Shell.h>

/**
  Function to start monitoring for CTRL-C using SimpleTextInputEx.  This
  feature's enabled state was not known when the shell initially launched.

  @retval EFI_SUCCESS           The feature is enabled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available.
**/
EFI_STATUS
InernalEfiShellStartMonitor (
  VOID
  );

/**
  Notification function for keystrokes.

  @param[in] KeyData    The key that was pressed.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
NotificationFunction (
  IN EFI_KEY_DATA  *KeyData
  );

#endif //_SHELL_PROTOCOL_HEADER_
