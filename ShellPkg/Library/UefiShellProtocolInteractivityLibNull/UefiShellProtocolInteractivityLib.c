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
  {
    { NULL,NULL   }, NULL, NULL
  },
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
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  This will also update the system table.

  @param[in, out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine              The new command line to parse and use.
  @param[out] OldStdIn                   Pointer to old StdIn.
  @param[out] OldStdOut                  Pointer to old StdOut.
  @param[out] OldStdErr                  Pointer to old StdErr.
  @param[out] SystemTableInfo            Pointer to old system table information.

  @retval   EFI_SUCCESS                 Operation was successful, Argv and Argc are valid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
UpdateStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             *NewCommandLine,
  OUT SHELL_FILE_HANDLE                 *OldStdIn,
  OUT SHELL_FILE_HANDLE                 *OldStdOut,
  OUT SHELL_FILE_HANDLE                 *OldStdErr,
  OUT SYSTEM_TABLE_INFO                 *SystemTableInfo
  )
{
  return EFI_SUCCESS;
}

/**
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure with StdIn and StdOut.  The current values are de-allocated.

  @param[in, out] ShellParameters      Pointer to parameter structure to modify.
  @param[in] OldStdIn                  Pointer to old StdIn.
  @param[in] OldStdOut                 Pointer to old StdOut.
  @param[in] OldStdErr                 Pointer to old StdErr.
  @param[in] SystemTableInfo           Pointer to old system table information.
**/
EFI_STATUS
RestoreStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN  SHELL_FILE_HANDLE                 *OldStdIn,
  IN  SHELL_FILE_HANDLE                 *OldStdOut,
  IN  SHELL_FILE_HANDLE                 *OldStdErr,
  IN  SYSTEM_TABLE_INFO                 *SystemTableInfo
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
