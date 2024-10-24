/** @file
  function definitions for shell environment functions.

  the following includes are required:
//#include <Guid/ShellVariableGuid.h>
//#include <Library/UefiRuntimeServicesTableLib.h>


  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_SHELL_INTERNALS_SHELL_ENV_VAR__
#define __UEFI_SHELL_INTERNALS_SHELL_ENV_VAR__

#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/ShellVariableGuid.h>

/**
  Reports whether an environment variable is Volatile or Non-Volatile.

  @param EnvVarName             The name of the environment variable in question
  @param Volatile               Return TRUE if the environment variable is volatile

  @retval EFI_SUCCESS           The volatile attribute is returned successfully
  @retval others                Some errors happened.
**/
EFI_STATUS
IsVolatileEnv (
  IN CONST CHAR16  *EnvVarName,
  OUT BOOLEAN      *Volatile
  );

/**
  Delete a Non-Volatile environment variable.

  This will use the Runtime Services call SetVariable to remove a non-volatile variable.

  @param EnvVarName             The name of the environment variable in question

  @retval EFI_SUCCESS           The variable was deleted successfully
  @retval other                 An error occurred
  @sa SetVariable
**/
#define SHELL_DELETE_ENVIRONMENT_VARIABLE(EnvVarName) \
  (gRT->SetVariable((CHAR16*)EnvVarName, \
  &gShellVariableGuid,          \
  0,                            \
  0,                            \
  NULL))

/**
  Set a Non-Volatile environment variable.

  This will use the Runtime Services call SetVariable to set a non-volatile variable.

  @param EnvVarName             The name of the environment variable in question
  @param BufferSize             UINTN size of Buffer
  @param Buffer                 Pointer to value to set variable to

  @retval EFI_SUCCESS           The variable was changed successfully
  @retval other                 An error occurred
  @sa SetVariable
**/
#define SHELL_SET_ENVIRONMENT_VARIABLE_NV(EnvVarName, BufferSize, Buffer)  \
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

  @retval EFI_SUCCESS           The variable's value was retrieved successfully
  @retval other                 An error occurred
  @sa SetVariable
**/
#define SHELL_GET_ENVIRONMENT_VARIABLE(EnvVarName, BufferSize, Buffer)    \
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

  @retval EFI_SUCCESS           The variable's value was retrieved successfully
  @retval other                 An error occurred
  @sa SetVariable
**/
#define SHELL_GET_ENVIRONMENT_VARIABLE_AND_ATTRIBUTES(EnvVarName, Atts, BufferSize, Buffer)    \
  (gRT->GetVariable((CHAR16*)EnvVarName,                        \
  &gShellVariableGuid,                                          \
  Atts,                                                            \
  BufferSize,                                                   \
  Buffer))

/**
  Set a Volatile environment variable.

  This will use the Runtime Services call SetVariable to set a volatile variable.

  @param EnvVarName             The name of the environment variable in question
  @param BufferSize             UINTN size of Buffer
  @param Buffer                 Pointer to value to set variable to

  @retval EFI_SUCCESS           The variable was changed successfully
  @retval other                 An error occurred
  @sa SetVariable
**/
#define SHELL_SET_ENVIRONMENT_VARIABLE_V(EnvVarName, BufferSize, Buffer) \
  (gRT->SetVariable((CHAR16*)EnvVarName,                      \
  &gShellVariableGuid,                                        \
  EFI_VARIABLE_BOOTSERVICE_ACCESS,                            \
  BufferSize,                                                 \
  (VOID*)Buffer))

#endif //__UEFI_SHELL_INTERNALS_SHELL_ENV_VAR__
