/** @file
  Main file for NULL named library for level 1 shell command functions.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <ShellBase.h>

#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/DevicePathToText.h>

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

extern        EFI_HANDLE                        gShellLevel1HiiHandle;
extern CONST  EFI_GUID                          gShellLevel1HiiGuid;

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
  Function to move to a spacified tag in a script file structure.

  @param[in] Function       The pointer to the function to move with.
  @param[in] DecrementerTag The pointer to a string to decrement upon finding.
  @param[in] IncrementerTag The pointer to a string to increment upon finding.
  @param[in] Label          A Label to look for.
  @param[in] ScriptFile     The script file structure to look in.
  @param[in] MovePast       TRUE to go to the element just after the found one.  FALSE otherwise.
  @param[in] FindOnly       FALSE to change the execution point in the script file structure.   TRUE otherwise.
  @param[in] WrapAroundScript  TRUE to go to begining when end is hit, or vise versa.  FALSE otherwise.
**/
BOOLEAN
EFIAPI
MoveToTag (
  IN CONST LIST_MANIP_FUNC      Function,
  IN CONST CHAR16               *DecrementerTag,
  IN CONST CHAR16               *IncrementerTag,
  IN CONST CHAR16               *Label OPTIONAL,
  IN SCRIPT_FILE                *ScriptFile,
  IN CONST BOOLEAN              MovePast,
  IN CONST BOOLEAN              FindOnly,
  IN CONST BOOLEAN              WrapAroundScript
  );

