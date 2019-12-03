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

typedef struct {
  LIST_ENTRY  Link;
  CHAR16      *Key;
  CHAR16      *Val;
  UINT32      Atts;
} ENV_VAR_LIST;

//
// The list is used to cache the environment variables.
//
extern ENV_VAR_LIST    gShellEnvVarList;


/**
  Reports whether an environment variable is Volatile or Non-Volatile.

  @param EnvVarName             The name of the environment variable in question
  @param Volatile               Return TRUE if the environment variable is volatile

  @retval EFI_SUCCESS           The volatile attribute is returned successfully
  @retval others                Some errors happened.
**/
EFI_STATUS
IsVolatileEnv (
  IN CONST CHAR16 *EnvVarName,
  OUT BOOLEAN     *Volatile
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
SetEnvironmentVariables(
  IN CONST CHAR16 **Environment
  );

/**
  free function for ENV_VAR_LIST objects.

  @param[in] List               The pointer to pointer to list.
**/
VOID
FreeEnvironmentVariableList(
  IN LIST_ENTRY *List
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
  IN  CONST CHAR16    *Key,
  OUT CHAR16          **Value,
  OUT UINTN           *ValueSize,
  OUT UINT32          *Atts OPTIONAL
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
  IN CONST CHAR16     *Key,
  IN CONST CHAR16     *Value,
  IN UINTN            ValueSize,
  IN UINT32           Atts
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
  IN CONST CHAR16           *Key
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

