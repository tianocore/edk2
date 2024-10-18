/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellProtocolsLib.h>
#include <Library/ShellProtocolInteractivityLib.h>
#include <Library/SortLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <ShellInternals.h>

/**
  Disables the page break output mode.
**/
VOID
EFIAPI
EfiShellDisablePageBreak (
  VOID
  )
{
  ShellProtocolInteractivityInfoObject.PageBreakEnabled = FALSE;
}

/**
  Enables the page break output mode.
**/
VOID
EFIAPI
EfiShellEnablePageBreak (
  VOID
  )
{
  ShellProtocolInteractivityInfoObject.PageBreakEnabled = TRUE;
}

/**
  Gets the enable status of the page break output mode.

  User can use this function to determine current page break mode.

  @retval TRUE                  The page break output mode is enabled.
  @retval FALSE                 The page break output mode is disabled.
**/
BOOLEAN
EFIAPI
EfiShellGetPageBreak (
  VOID
  )
{
  return (ShellProtocolInteractivityInfoObject.PageBreakEnabled);
}

/**
  Determine if the UEFI Shell is currently running with nesting enabled or disabled.

  @retval FALSE   nesting is required
  @retval other   nesting is enabled
**/
BOOLEAN
EFIAPI
NestingEnabled (
  VOID
  )
{
  EFI_STATUS  Status;
  CHAR16      *Temp;
  CHAR16      *Temp2;
  UINTN       TempSize;
  BOOLEAN     RetVal;

  RetVal = TRUE;
  Temp   = NULL;
  Temp2  = NULL;

  if (ShellProtocolsInfoObject.ShellProtocolBitUnion.Bits.NoNest) {
    TempSize = 0;
    Temp     = NULL;
    Status   = SHELL_GET_ENVIRONMENT_VARIABLE (mNoNestingEnvVarName, &TempSize, Temp);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Temp = AllocateZeroPool (TempSize + sizeof (CHAR16));
      if (Temp != NULL) {
        Status = SHELL_GET_ENVIRONMENT_VARIABLE (mNoNestingEnvVarName, &TempSize, Temp);
      }
    }

    Temp2 = StrnCatGrow (&Temp2, NULL, mNoNestingTrue, 0);
    if ((Temp != NULL) && (Temp2 != NULL) && (StringNoCaseCompare (&Temp, &Temp2) == 0)) {
      //
      // Use the no nesting method.
      //
      RetVal = FALSE;
    }
  }

  SHELL_FREE_NON_NULL (Temp);
  SHELL_FREE_NON_NULL (Temp2);
  return (RetVal);
}

/**
  Cleanup the interactive shell environment.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
CleanUpInteractiveShellEnvironment (
  VOID
  )
{
  EFI_STATUS                         Status;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *SimpleEx;

  Status = gBS->OpenProtocol (
                  gST->ConsoleInHandle,
                  &gEfiSimpleTextInputExProtocolGuid,
                  (VOID **)&SimpleEx,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle1);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle2);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle3);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlCNotifyHandle4);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle1);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle2);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle3);
    Status = SimpleEx->UnregisterKeyNotify (SimpleEx, ShellProtocolInteractivityInfoObject.CtrlSNotifyHandle4);
  }

  return (Status);
}
