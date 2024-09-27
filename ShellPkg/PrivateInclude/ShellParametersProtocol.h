/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_SHELL_INTERNALS_SHELL_PARAMETERS_PROTOCOL__
#define __UEFI_SHELL_INTERNALS_SHELL_PARAMETERS_PROTOCOL__

#include <Uefi.h>
#include <Protocol/ShellParameters.h>

typedef enum {
  Internal_Command,
  Script_File_Name,
  Efi_Application,
  File_Sys_Change,
  Unknown_Invalid
} SHELL_OPERATION_TYPES;

/**
  Function will replace the current Argc and Argv in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  @param[in, out] ShellParameters       pointer to parameter structure to modify
  @param[in] NewCommandLine             the new command line to parse and use
  @param[in] Type                       the type of operation.
  @param[out] OldArgv                   pointer to old list of parameters
  @param[out] OldArgc                   pointer to old number of items in Argv list

  @retval   EFI_SUCCESS                 operation was successful, Argv and Argc are valid
  @return   EFI_INVALID_PARAMETER       some parameters are invalid
  @retval   EFI_OUT_OF_RESOURCES        a memory allocation failed.
**/
EFI_STATUS
UpdateArgcArgv (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CONST CHAR16                       *NewCommandLine,
  IN SHELL_OPERATION_TYPES              Type,
  OUT CHAR16                            ***OldArgv,
  OUT UINTN                             *OldArgc
  );

/**
  Function will replace the current Argc and Argv in the ShellParameters protocol
  structure with Argv and Argc.  The current values are de-allocated and the
  OldArgv must not be deallocated by the caller.

  @param[in, out] ShellParameters       pointer to parameter structure to modify
  @param[in] OldArgv                    pointer to old list of parameters
  @param[in] OldArgc                    pointer to old number of items in Argv list
**/
VOID
RestoreArgcArgv (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             ***OldArgv,
  IN UINTN                              *OldArgc
  );

typedef struct {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     *ConIn;
  EFI_HANDLE                         ConInHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *ConOut;
  EFI_HANDLE                         ConOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *ErrOut;
  EFI_HANDLE                         ErrOutHandle;
} SYSTEM_TABLE_INFO;

/**
  return the next parameter from a command line string;

  This function moves the next parameter from Walker into TempParameter and moves
  Walker up past that parameter for recursive calling.  When the final parameter
  is moved *Walker will be set to NULL;

  Temp Parameter must be large enough to hold the parameter before calling this
  function.

  @param[in, out] Walker          pointer to string of command line.  Adjusted to
                                  remaining command line on return
  @param[in, out] TempParameter   pointer to string of command line item extracted.
  @param[in]      Length          Length of (*TempParameter) in bytes
  @param[in]      StripQuotation  if TRUE then strip the quotation marks surrounding
                                  the parameters.

  @return   EFI_INVALID_PARAMETER A required parameter was NULL or pointed to a NULL or empty string.
  @return   EFI_NOT_FOUND         A closing " could not be found on the specified string
**/
EFI_STATUS
GetNextParameter (
  IN OUT CHAR16   **Walker,
  IN OUT CHAR16   **TempParameter,
  IN CONST UINTN  Length,
  IN BOOLEAN      StripQuotation
  );

#endif //__UEFI_SHELL_INTERNALS_SHELL_PARAMETERS_PROTOCOL__
