/** @file
  Member functions of EFI_SHELL_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PROTOCOL.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ShellProtocolInteractivityLib.h>

//
// Initialize the global structure
//
SHELL_PROTOCOL_INTERACTIVITY_INFO  ShellProtocolInteractivityInfoObject = {
  FALSE,
  {
    {
      { NULL,NULL   }, NULL
    },
    0,
    0,
    TRUE
  },
  NULL,
  0,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  FALSE
};

/**
  Displays previously logged output back to the screen.

  This will scroll the screen forwards and backwards through the log of previous
  output.  If Rows is 0 then the size of 1/2 the screen will be scrolled.  If Rows
  is (UINTN)(-1) then the size of the screen will be scrolled.

  @param[in] Forward      If TRUE then the log will be displayed forwards (scroll to newer).
                          If FALSE then the log will be displayed backwards (scroll to older).
  @param[in] Rows         Determines how many rows the log should scroll.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.
**/
EFI_STATUS
ConsoleLoggerDisplayHistory (
  IN CONST BOOLEAN                Forward,
  IN CONST UINTN                  Rows,
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  return EFI_SUCCESS;
}

/**
  Function to return to normal output whent he scrolling is complete.
  @param[in] ConsoleInfo  The pointer to the instance of the console logger information.

  @retval EFI_SUCCESS   The operation was successful.
  @return other         The operation failed.  See UpdateDisplayFromHistory.

  @sa UpdateDisplayFromHistory
**/
EFI_STATUS
ConsoleLoggerStopHistory (
  IN CONSOLE_LOGGER_PRIVATE_DATA  *ConsoleInfo
  )
{
  return EFI_SUCCESS;
}

/**
  Disables the page break output mode.
**/
VOID
EFIAPI
EfiShellDisablePageBreak (
  VOID
  )
{
  return;
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
  return;
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
  return FALSE;
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
  return FALSE;
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
  return EFI_SUCCESS;
}

/**
  Execute tasks for each round of the loop.

**/
VOID
EFIAPI
UefiShellProtocolInteractivityLibExecuteWaitLoopTasks (
  VOID
  )
{
  return;
}

/**
  Add a buffer to the Line History List

  @param Buffer     The line buffer to add.
**/
VOID
AddLineToCommandHistory (
  IN CONST CHAR16  *Buffer
  )
{
  return;
}

/**
  Prints out each previously typed command in the command list history log.

  When each screen is full it will pause for a key before continuing.

  @param[in] TotalCols    How many columns are on the screen
  @param[in] TotalRows    How many rows are on the screen
  @param[in] StartColumn  which column to start at
**/
VOID
PrintCommandHistory (
  IN CONST UINTN  TotalCols,
  IN CONST UINTN  TotalRows,
  IN CONST UINTN  StartColumn
  )
{
  return;
}
