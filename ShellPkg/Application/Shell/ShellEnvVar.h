/** @file
  function definitions for shell environment functions.

  the following includes are required:
//#include <Guid/ShellVariableGuid.h>
//#include <Library/UefiRuntimeServicesTableLib.h>


  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SHELL_ENVIRONMENT_VARIABLE_HEADER_
#define _SHELL_ENVIRONMENT_VARIABLE_HEADER_

typedef struct {
  LIST_ENTRY  Link;
  CHAR16      *Key;
  CHAR16      *Val;
  UINT32      Atts;
} ENV_VAR_LIST;

/**
  Reports whether an environment variable is Volatile or Non-Volatile

  This will use the Runtime Services call GetVariable to to search for the variable.

  @param EnvVarName             The name of the environment variable in question

  @retval TRUE                  This environment variable is Volatile
  @retval FALSE                 This environment variable is NON-Volatile
**/
BOOLEAN
EFIAPI
IsVolatileEnv (
  IN CONST CHAR16 *EnvVarName
  );

/**
  Delete a Non-Violatile environment variable.

  This will use the Runtime Services call SetVariable to remove a non-violatile variable.

  @param EnvVarName             The name of the environment variable in question

  @retval EFI_SUCCESS           The variable was deleted sucessfully
  @retval other                 An error ocurred
  @sa SetVariable
**/
#define SHELL_DELETE_ENVIRONMENT_VARIABLE(EnvVarName) \
  (gRT->SetVariable((CHAR16*)EnvVarName, \
  &gShellVariableGuid,          \
  0,                            \
  0,                            \
  NULL))

/**
  Set a Non-Violatile environment variable.

  This will use the Runtime Services call SetVariable to set a non-violatile variable.

  @param EnvVarName             The name of the environment variable in question
  @param BufferSize             UINTN size of Buffer
  @param Buffer                 Pointer to value to set variable to

  @retval EFI_SUCCESS           The variable was changed sucessfully
  @retval other                 An error ocurred
  @sa SetVariable
**/
#define SHELL_SET_ENVIRONMENT_VARIABLE_NV(EnvVarName,BufferSize,Buffer)  \
  (gRT->SetVariable((CHAR16*)EnvVarName,                          \
  &gShellVariableGuid,                                            \
  EFI_VARIABLE_NON_VOLATILE|EFI_VARIABLE_BOOTSERVICE_ACCESS,      \
  BufferSize,                                                     \
  (VOID*)Buffer))

/**
  Get an environment variable.

  This will use the Runtime Services call GetVariable to get a variable.

  @param EnvVarName             The name of the environment variable in question
  @param BufferSize             Pointer to the UINTN size of Buffer
  @param Buffer                 Pointer buffer to get variable value into

  @retval EFI_SUCCESS           The variable's value was retrieved sucessfully
  @retval other                 An error ocurred
  @sa SetVariable
**/
#define SHELL_GET_ENVIRONMENT_VARIABLE(EnvVarName,BufferSize,Buffer)    \
  (gRT->GetVariable((CHAR16*)EnvVarName,                        \
  &gShellVariableGuid,                                          \
  0,                                                            \
  BufferSize,                                                   \
  Buffer))

/**
  Get an environment variable.

  This will use the Runtime Services call GetVariable to get a variable.

  @param EnvVarName             The name of the environment variable in question
  @param Atts                   Pointer to the UINT32 for attributes (or NULL)
  @param BufferSize             Pointer to the UINTN size of Buffer
  @param Buffer                 Pointer buffer to get variable value into

  @retval EFI_SUCCESS           The variable's value was retrieved sucessfully
  @retval other                 An error ocurred
  @sa SetVariable
**/
#define SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES(EnvVarName,Atts,BufferSize,Buffer)    \
  (gRT->GetVariable((CHAR16*)EnvVarName,                        \
  &gShellVariableGuid,                                          \
  Atts,                                                            \
  BufferSize,                                                   \
  Buffer))

/**
  Set a Violatile environment variable.

  This will use the Runtime Services call SetVariable to set a violatile variable.

  @param EnvVarName             The name of the environment variable in question
  @param BufferSize             UINTN size of Buffer
  @param Buffer                 Pointer to value to set variable to

  @retval EFI_SUCCESS           The variable was changed sucessfully
  @retval other                 An error ocurred
  @sa SetVariable
**/
#define SHELL_SET_ENVIRONMENT_VARIABLE_V(EnvVarName,BufferSize,Buffer) \
  (gRT->SetVariable((CHAR16*)EnvVarName,                      \
  &gShellVariableGuid,                                        \
  EFI_VARIABLE_BOOTSERVICE_ACCESS,                            \
  BufferSize,                                                 \
  (VOID*)Buffer))

/**
  Creates a list of all Shell-Guid-based environment variables.

  @param[in, out] List           The pointer to pointer to LIST_ENTRY object for
                                 storing this list.

  @retval EFI_SUCCESS           the list was created sucessfully.
**/
EFI_STATUS
EFIAPI
GetEnvironmentVariableList(
  IN OUT LIST_ENTRY *List
  );

/**
  Sets a list of all Shell-Guid-based environment variables.  this will
  also eliminate all pre-existing shell environment variables (even if they
  are not on the list).

  This function will also deallocate the memory from List.

  @param[in] List               The pointer to LIST_ENTRY from
                                GetShellEnvVarList().

  @retval EFI_SUCCESS           The list was Set sucessfully.
**/
EFI_STATUS
EFIAPI
SetEnvironmentVariableList(
  IN LIST_ENTRY *List
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
EFIAPI
SetEnvironmentVariables(
  IN CONST CHAR16 **Environment
  );

/**
  free function for ENV_VAR_LIST objects.

  @param[in] List               The pointer to pointer to list.
**/
VOID
EFIAPI
FreeEnvironmentVariableList(
  IN LIST_ENTRY *List
  );

#endif //_SHELL_ENVIRONMENT_VARIABLE_HEADER_

