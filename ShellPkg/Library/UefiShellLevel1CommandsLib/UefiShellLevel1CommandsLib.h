/** @file
  Main file for NULL named library for level 1 shell command functions.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UEFI_SHELL_LEVEL1_COMMANDS_LIB_H_
#define _UEFI_SHELL_LEVEL1_COMMANDS_LIB_H_

#include <Uefi.h>

#include <Guid/ShellLibHiiGuid.h>

#include <Protocol/Shell.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/FileHandleLib.h>

extern        EFI_HII_HANDLE  gShellLevel1HiiHandle;

/**
  Function for 'stall' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunStall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'exit' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunExit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'endif' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEndIf (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'for' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunFor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'endfor' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEndFor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'if' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunIf (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'goto' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunGoto (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'shift' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunShift (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'else' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunElse (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

///
/// Function prototype for BOTH GetNextNode and GetPreviousNode...
/// This is used to control the MoveToTag function direction...
///
typedef
LIST_ENTRY *
(EFIAPI *LIST_MANIP_FUNC)(
  IN CONST LIST_ENTRY *List,
  IN CONST LIST_ENTRY *Node
  );

/**
  Move the script pointer from 1 tag (line) to another.

  It functions so that count starts at 1 and it increases or decreases when it
  hits the specified tags.  when it hits zero the location has been found.

  DecrementerTag and IncrementerTag are used to get around for/endfor and
  similar paired types where the entire middle should be ignored.

  If label is used it will be used instead of the count.

  @param[in] Function          The function to use to enumerate through the
                               list.  Normally GetNextNode or GetPreviousNode.
  @param[in] DecrementerTag    The tag to decrement the count at.
  @param[in] IncrementerTag    The tag to increment the count at.
  @param[in] Label             A label to look for.
  @param[in, out] ScriptFile   The pointer to the current script file structure.
  @param[in] MovePast          TRUE makes function return 1 past the found
                               location.
  @param[in] FindOnly          TRUE to not change the ScriptFile.
  @param[in] WrapAroundScript  TRUE to wrap end-to-beginning or vise versa in
                               searching.
**/
BOOLEAN
MoveToTag (
  IN CONST LIST_MANIP_FUNC  Function,
  IN CONST CHAR16           *DecrementerTag,
  IN CONST CHAR16           *IncrementerTag,
  IN CONST CHAR16           *Label OPTIONAL,
  IN OUT SCRIPT_FILE        *ScriptFile,
  IN CONST BOOLEAN          MovePast,
  IN CONST BOOLEAN          FindOnly,
  IN CONST BOOLEAN          WrapAroundScript
  );

#endif
