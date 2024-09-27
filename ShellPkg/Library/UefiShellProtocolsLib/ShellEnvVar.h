/** @file
  function definitions for shell environment functions.

  the following includes are required:
//#include <Guid/ShellVariableGuid.h>
//#include <Library/UefiRuntimeServicesTableLib.h>


  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_ENVIRONMENT_VARIABLE_HEADER_
#define _SHELL_ENVIRONMENT_VARIABLE_HEADER_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

typedef struct {
  LIST_ENTRY    Link;
  CHAR16        *Key;
  CHAR16        *Val;
  UINT32        Atts;
} ENV_VAR_LIST;

//
// The list is used to cache the environment variables.
//
extern ENV_VAR_LIST  gShellEnvVarList;

/**
  Creates a list of all Shell-Guid-based environment variables.

  @param[in, out] List           The pointer to pointer to LIST_ENTRY object for
                                 storing this list.

  @retval EFI_SUCCESS           the list was created successfully.
**/
EFI_STATUS
GetEnvironmentVariableList (
  IN OUT LIST_ENTRY  *List
  );

/**
  Sets a list of all Shell-Guid-based environment variables.  this will
  also eliminate all pre-existing shell environment variables (even if they
  are not on the list).

  This function will also deallocate the memory from List.

  @param[in] List               The pointer to LIST_ENTRY from
                                GetShellEnvVarList().

  @retval EFI_SUCCESS           The list was Set successfully.
**/
EFI_STATUS
SetEnvironmentVariableList (
  IN LIST_ENTRY  *List
  );

/**
  sets all Shell-Guid-based environment variables.  this will
  also eliminate all pre-existing shell environment variables (even if they
  are not on the list).

  @param[in] Environment    Points to a NULL-terminated array of environment
                            variables with the format 'x=y', where x is the
                            environment variable name and y is the value.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_INVALID_PARAMETER The parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES Out of resources.

  @sa SetEnvironmentVariableList
**/
EFI_STATUS
SetEnvironmentVariables (
  IN CONST CHAR16  **Environment
  );

/**
  free function for ENV_VAR_LIST objects.

  @param[in] List               The pointer to pointer to list.
**/
VOID
FreeEnvironmentVariableList (
  IN LIST_ENTRY  *List
  );

/**
  Find an environment variable in the gShellEnvVarList.

  @param Key        The name of the environment variable.
  @param Value      The value of the environment variable, the buffer
                    shoule be freed by the caller.
  @param ValueSize  The size in bytes of the environment variable
                    including the tailing CHAR_NULL.
  @param Atts       The attributes of the variable.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_NOT_FOUND     The environment variable is not found in
                            gShellEnvVarList.

**/
EFI_STATUS
ShellFindEnvVarInList (
  IN  CONST CHAR16  *Key,
  OUT CHAR16        **Value,
  OUT UINTN         *ValueSize,
  OUT UINT32        *Atts OPTIONAL
  );

/**
  Add an environment variable into gShellEnvVarList.

  @param Key        The name of the environment variable.
  @param Value      The value of environment variable.
  @param ValueSize  The size in bytes of the environment variable
                    including the tailing CHAR_NULL
  @param Atts       The attributes of the variable.

  @retval EFI_SUCCESS  The environment variable was added to list successfully.
  @retval others       Some errors happened.

**/
EFI_STATUS
ShellAddEnvVarToList (
  IN CONST CHAR16  *Key,
  IN CONST CHAR16  *Value,
  IN UINTN         ValueSize,
  IN UINT32        Atts
  );

/**
  Remove a specified environment variable in gShellEnvVarList.

  @param Key        The name of the environment variable.

  @retval EFI_SUCCESS       The command executed successfully.
  @retval EFI_NOT_FOUND     The environment variable is not found in
                            gShellEnvVarList.
**/
EFI_STATUS
ShellRemvoeEnvVarFromList (
  IN CONST CHAR16  *Key
  );

/**
  Initialize the gShellEnvVarList and cache all Shell-Guid-based environment
  variables.

**/
EFI_STATUS
ShellInitEnvVarList (
  VOID
  );

/**
  Destructe the gShellEnvVarList.

**/
VOID
ShellFreeEnvVarList (
  VOID
  );

#endif //_SHELL_ENVIRONMENT_VARIABLE_HEADER_
