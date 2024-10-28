/** @file
  Private header for UEFI Shell internals.

  Copyright (c) 2024, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __UEFI_SHELL_INTERNALS__
#define __UEFI_SHELL_INTERNALS__

#include <Uefi.h>

#include "ShellParametersProtocol.h"
#include "ShellProtocol.h"
#include "ShellEnvVar.h"
#include "ShellManParser.h"
#include "ConsoleWrappers.h"
#include "FileHandleWrappers.h"

#define mNoNestingEnvVarName  L"nonesting"
#define mNoNestingTrue        L"True"
#define mNoNestingFalse       L"False"

/**
  Execute tasks for each round of the loop.

**/
VOID
EFIAPI
UefiShellProtocolsLibExecuteWaitLoopTasks (
  VOID
  );

/**
  Execute tasks for each round of the loop.

**/
VOID
EFIAPI
UefiShellProtocolInteractivityLibExecuteWaitLoopTasks (
  VOID
  );

/**
  Converts the command line to its post-processed form.  this replaces variables and alias' per UEFI Shell spec.

  @param[in,out] CmdLine        pointer to the command line to update

  @retval EFI_SUCCESS           The operation was successful
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @return                       some other error occurred
**/
EFI_STATUS
ProcessCommandLineToFinal (
  IN OUT CHAR16  **CmdLine
  );

/**
  Function to update the shell variable "lasterror".

  @param[in] ErrorCode      the error code to put into lasterror
**/
EFI_STATUS
SetLastError (
  IN CONST SHELL_STATUS  ErrorCode
  );

/**
  Sets all the alias' that were registered with the ShellCommandLib library.

  @retval EFI_SUCCESS           all init commands were run successfully.
**/
EFI_STATUS
SetBuiltInAlias (
  VOID
  );

/**
  Add a buffer to the Buffer To Free List for safely returning buffers to other
  places without risking letting them modify internal shell information.

  @param Buffer   Something to pass to FreePool when the shell is exiting.
**/
VOID *
AddBufferToFreeList (
  VOID  *Buffer
  );

/**
  Add a buffer to the Command History List.

  @param Buffer[in]     The line buffer to add.
**/
VOID
AddLineToCommandHistory (
  IN CONST CHAR16  *Buffer
  );

/**
  Function will process and run a command line.

  This will determine if the command line represents an internal shell command or dispatch an external application.

  @param[in] CmdLine  the command line to parse

  @retval EFI_SUCCESS     the command was completed
  @retval EFI_ABORTED     the command's operation was aborted
**/
EFI_STATUS
RunCommand (
  IN CONST CHAR16  *CmdLine
  );

/**
  Function will process and run a command line.

  This will determine if the command line represents an internal shell
  command or dispatch an external application.

  @param[in] CmdLine      The command line to parse.
  @param[out] CommandStatus   The status from the command line.

  @retval EFI_SUCCESS     The command was completed.
  @retval EFI_ABORTED     The command's operation was aborted.
**/
EFI_STATUS
RunShellCommand (
  IN CONST CHAR16  *CmdLine,
  OUT EFI_STATUS   *CommandStatus
  );

/**
  Function to process a NSH script file via SHELL_FILE_HANDLE.

  @param[in] Handle             The handle to the already opened file.
  @param[in] Name               The name of the script file.

  @retval EFI_SUCCESS           the script completed successfully
**/
EFI_STATUS
RunScriptFileHandle (
  IN SHELL_FILE_HANDLE  Handle,
  IN CONST CHAR16       *Name
  );

/**
  Function to process a NSH script file.

  @param[in] ScriptPath         Pointer to the script file name (including file system path).
  @param[in] Handle             the handle of the script file already opened.
  @param[in] CmdLine            the command line to run.
  @param[in] ParamProtocol      the shell parameters protocol pointer

  @retval EFI_SUCCESS           the script completed successfully
**/
EFI_STATUS
RunScriptFile (
  IN CONST CHAR16                   *ScriptPath,
  IN SHELL_FILE_HANDLE              Handle OPTIONAL,
  IN CONST CHAR16                   *CmdLine,
  IN EFI_SHELL_PARAMETERS_PROTOCOL  *ParamProtocol
  );

/**
  Return the pointer to the first occurrence of any character from a list of characters.

  @param[in] String           the string to parse
  @param[in] CharacterList    the list of character to look for
  @param[in] EscapeCharacter  An escape character to skip

  @return the location of the first character in the string
  @retval CHAR_NULL no instance of any character in CharacterList was found in String
**/
CONST CHAR16 *
FindFirstCharacter (
  IN CONST CHAR16  *String,
  IN CONST CHAR16  *CharacterList,
  IN CONST CHAR16  EscapeCharacter
  );

/**
  Cleans off leading and trailing spaces and tabs.

  @param[in] String pointer to the string to trim them off.
**/
EFI_STATUS
TrimSpaces (
  IN CHAR16  **String
  );

/**

  Create a new buffer list and stores the old one to OldBufferList

  @param OldBufferList   The temporary list head used to store the nodes in BufferToFreeList.
**/
VOID
SaveBufferList (
  OUT LIST_ENTRY  *OldBufferList
  );

/**
  Restore previous nodes into BufferToFreeList .

  @param OldBufferList   The temporary list head used to store the nodes in BufferToFreeList.
**/
VOID
RestoreBufferList (
  IN OUT LIST_ENTRY  *OldBufferList
  );

#endif
