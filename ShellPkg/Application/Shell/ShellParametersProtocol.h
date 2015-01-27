/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SHELL_PARAMETERS_PROTOCOL_PROVIDER_HEADER_
#define _SHELL_PARAMETERS_PROTOCOL_PROVIDER_HEADER_

#include "Shell.h"

/**
  creates a new EFI_SHELL_PARAMETERS_PROTOCOL instance and populates it and then
  installs it on our handle and if there is an existing version of the protocol
  that one is cached for removal later.

  @param[in, out] NewShellParameters on a successful return, a pointer to pointer
                                     to the newly installed interface.
  @param[in, out] RootShellInstance  on a successful return, pointer to boolean.
                                     TRUE if this is the root shell instance.

  @retval EFI_SUCCESS               the operation completed successfully.
  @return other                     the operation failed.
  @sa ReinstallProtocolInterface
  @sa InstallProtocolInterface
  @sa ParseCommandLineToArgs
**/
EFI_STATUS
EFIAPI
CreatePopulateInstallShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  **NewShellParameters,
  IN OUT BOOLEAN                        *RootShellInstance
  );

/**
  frees all memory used by createion and installation of shell parameters protocol
  and if there was an old version installed it will restore that one.

  @param NewShellParameters the interface of EFI_SHELL_PARAMETERS_PROTOCOL that is
  being cleaned up.

  @retval EFI_SUCCESS     the cleanup was successful
  @return other           the cleanup failed
  @sa ReinstallProtocolInterface
  @sa UninstallProtocolInterface
**/
EFI_STATUS
EFIAPI
CleanUpShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *NewShellParameters
  );

/**
  Funcion will replace the current Argc and Argv in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  @param[in, out] ShellParameters       pointer to parameter structure to modify
  @param[in] NewCommandLine             the new command line to parse and use
  @param[out] OldArgv                   pointer to old list of parameters
  @param[out] OldArgc                   pointer to old number of items in Argv list

  @retval   EFI_SUCCESS                 operation was sucessful, Argv and Argc are valid
  @retval   EFI_OUT_OF_RESOURCES        a memory allocation failed.
**/
EFI_STATUS
EFIAPI
UpdateArgcArgv(
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CONST CHAR16                       *NewCommandLine,
  OUT CHAR16                            ***OldArgv,
  OUT UINTN                             *OldArgc
  );

/**
  Funcion will replace the current Argc and Argv in the ShellParameters protocol
  structure with Argv and Argc.  The current values are de-allocated and the
  OldArgv must not be deallocated by the caller.

  @param[in, out] ShellParameters       pointer to parameter structure to modify
  @param[in] OldArgv                    pointer to old list of parameters
  @param[in] OldArgc                    pointer to old number of items in Argv list
**/
VOID
EFIAPI
RestoreArgcArgv(
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             ***OldArgv,
  IN UINTN                              *OldArgc
  );

typedef struct {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL        *ConIn;
  EFI_HANDLE                            ConInHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *ConOut;
  EFI_HANDLE                            ConOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *ErrOut;
  EFI_HANDLE                            ErrOutHandle;
} SYSTEM_TABLE_INFO;

/**
  Funcion will replace the current StdIn and StdOut in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  This will also update the system table.

  @param[in, out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine              The new command line to parse and use.
  @param[out] OldStdIn                   Pointer to old StdIn.
  @param[out] OldStdOut                  Pointer to old StdOut.
  @param[out] OldStdErr                  Pointer to old StdErr.
  @param[out] SystemTableInfo            Pointer to old system table information.

  @retval   EFI_SUCCESS                 Operation was sucessful, Argv and Argc are valid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
EFIAPI
UpdateStdInStdOutStdErr(
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             *NewCommandLine,
  OUT SHELL_FILE_HANDLE                 *OldStdIn,
  OUT SHELL_FILE_HANDLE                 *OldStdOut,
  OUT SHELL_FILE_HANDLE                 *OldStdErr,
  OUT SYSTEM_TABLE_INFO                 *SystemTableInfo
  );

/**
  Funcion will replace the current StdIn and StdOut in the ShellParameters protocol
  structure with StdIn and StdOut.  The current values are de-allocated.

  @param[in, out] ShellParameters      Pointer to parameter structure to modify.
  @param[in] OldStdIn                  Pointer to old StdIn.
  @param[in] OldStdOut                 Pointer to old StdOut.
  @param[in] OldStdErr                 Pointer to old StdErr.
  @param[in] SystemTableInfo           Pointer to old system table information.
**/
EFI_STATUS
EFIAPI
RestoreStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN  SHELL_FILE_HANDLE                 *OldStdIn,
  IN  SHELL_FILE_HANDLE                 *OldStdOut,
  IN  SHELL_FILE_HANDLE                 *OldStdErr,
  IN  SYSTEM_TABLE_INFO                 *SystemTableInfo
  );

/**
  function to populate Argc and Argv.

  This function parses the CommandLine and divides it into standard C style Argc/Argv
  parameters for inclusion in EFI_SHELL_PARAMETERS_PROTOCOL.  this supports space
  delimited and quote surrounded parameter definition.

  @param[in] CommandLine         String of command line to parse
  @param[in, out] Argv           pointer to array of strings; one for each parameter
  @param[in, out] Argc           pointer to number of strings in Argv array

  @return EFI_SUCCESS           the operation was sucessful
  @return EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
EFIAPI
ParseCommandLineToArgs(
  IN CONST CHAR16 *CommandLine,
  IN OUT CHAR16 ***Argv,
  IN OUT UINTN *Argc
  );

/**
  return the next parameter from a command line string;

  This function moves the next parameter from Walker into TempParameter and moves
  Walker up past that parameter for recursive calling.  When the final parameter
  is moved *Walker will be set to NULL;

  Temp Parameter must be large enough to hold the parameter before calling this
  function.

  @param[in, out] Walker        pointer to string of command line.  Adjusted to
                                reminaing command line on return
  @param[in, out] TempParameter pointer to string of command line item extracted.
  @param[in]      Length        Length of (*TempParameter) in bytes

  @return   EFI_INALID_PARAMETER  A required parameter was NULL or pointed to a NULL or empty string.
  @return   EFI_NOT_FOUND         A closing " could not be found on the specified string
**/
EFI_STATUS
EFIAPI
GetNextParameter(
  IN OUT CHAR16   **Walker,
  IN OUT CHAR16   **TempParameter,
  IN CONST UINTN  Length
  );

#endif //_SHELL_PARAMETERS_PROTOCOL_PROVIDER_HEADER_

