/** @file
  function definitions for internal to shell functions.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SHELL_INTERNAL_HEADER_
#define _SHELL_INTERNAL_HEADER_

#include <Uefi.h>
#include <ShellBase.h>

#include <Guid/ShellVariableGuid.h>
#include <Guid/ShellAliasGuid.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/EfiShell.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/EfiShellParameters.h>
#include <Protocol/BlockIo.h>

#include <Library/BaseLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/ShellLib.h>
#include <Library/SortLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/FileHandleLib.h>

#include "ShellParametersProtocol.h"
#include "ShellProtocol.h"
#include "ShellEnvVar.h"
#include "ConsoleLogger.h"
#include "ShellManParser.h"
#include "ConsoleWrappers.h"
#include "FileHandleWrappers.h"

typedef struct {
  LIST_ENTRY        Link;           ///< Standard linked list handler.
  SHELL_FILE_HANDLE *SplitStdOut;   ///< ConsoleOut for use in the split.
  SHELL_FILE_HANDLE *SplitStdIn;    ///< ConsoleIn for use in the split.
} SPLIT_LIST;

typedef struct {
  UINT32  Startup:1;      ///< Was "-startup"       found on command line.
  UINT32  NoStartup:1;    ///< Was "-nostartup"     found on command line.
  UINT32  NoConsoleOut:1; ///< Was "-noconsoleout"  found on command line.
  UINT32  NoConsoleIn:1;  ///< Was "-noconsolein"   found on command line.
  UINT32  NoInterrupt:1;  ///< Was "-nointerrupt"   found on command line.
  UINT32  NoMap:1;        ///< Was "-nomap"         found on command line.
  UINT32  NoVersion:1;    ///< Was "-noversion"     found on command line.
  UINT32  Delay:1;        ///< Was "-delay[:n]      found on command line
  UINT32  Exit:1;         ///< Was "-_exit"          found on command line
  UINT32  Reserved:7;     ///< Extra bits
} SHELL_BITS;

typedef union {
  SHELL_BITS  Bits;
  UINT16      AllBits;
} SHELL_BIT_UNION;

typedef struct {
  SHELL_BIT_UNION BitUnion;
  UINTN           Delay;          ///< Seconds of delay default:5.
  CHAR16          *FileName;      ///< Filename to run upon successful initialization.
  CHAR16          *FileOptions;   ///< Options to pass to FileName.
} SHELL_INIT_SETTINGS;

typedef struct {
  BUFFER_LIST                 CommandHistory;
  UINTN                       VisibleRowNumber;
  UINTN                       OriginalVisibleRowNumber;
  BOOLEAN                     InsertMode;           ///< Is the current typing mode insert (FALSE = overwrite).
} SHELL_VIEWING_SETTINGS;

typedef struct {
  EFI_SHELL_PARAMETERS_PROTOCOL *NewShellParametersProtocol;
  EFI_SHELL_PROTOCOL            *NewEfiShellProtocol;
  BOOLEAN                       PageBreakEnabled;
  BOOLEAN                       RootShellInstance;
  SHELL_INIT_SETTINGS           ShellInitSettings;
  BUFFER_LIST                   BufferToFreeList;     ///< List of buffers that were returned to the user to free.
  SHELL_VIEWING_SETTINGS        ViewingSettings;
  EFI_HII_HANDLE                HiiHandle;            ///< Handle from HiiLib.
  UINTN                         LogScreenCount;       ///< How many screens of log information to save.
  EFI_EVENT                     UserBreakTimer;       ///< Timer event for polling for CTRL-C.
  EFI_DEVICE_PATH_PROTOCOL      *ImageDevPath;        ///< DevicePath for ourselves.
  EFI_DEVICE_PATH_PROTOCOL      *FileDevPath;         ///< DevicePath for ourselves.
  CONSOLE_LOGGER_PRIVATE_DATA   *ConsoleInfo;         ///< Pointer for ConsoleInformation.
  EFI_SHELL_PARAMETERS_PROTOCOL *OldShellParameters;  ///< old shell parameters to reinstall upon exiting.
  SHELL_PROTOCOL_HANDLE_LIST    OldShellList;         ///< List of other instances to reinstall when closing.
  SPLIT_LIST                    SplitList;            ///< List of Splits in FILO stack.
  VOID                          *CtrlCNotifyHandle1;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlCNotifyHandle2;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlCNotifyHandle3;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlCNotifyHandle4;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlSNotifyHandle1;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlSNotifyHandle2;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlSNotifyHandle3;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                          *CtrlSNotifyHandle4;  ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  BOOLEAN                       HaltOutput;           ///< TRUE to start a CTRL-S halt.
} SHELL_INFO;

extern SHELL_INFO ShellInfoObject;

typedef enum {
  Internal_Command,
  Script_File_Name,
  Efi_Application,
  File_Sys_Change,
  Unknown_Invalid
} SHELL_OPERATION_TYPES;

/**
  Converts the command line to it's post-processed form.  this replaces variables and alias' per UEFI Shell spec.

  @param[in,out] CmdLine        pointer to the command line to update

  @retval EFI_SUCCESS           The operation was successful
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @return                       some other error occurred
**/
EFI_STATUS
EFIAPI
ProcessCommandLineToFinal(
  IN OUT CHAR16 **CmdLine
  );

/**
  Function to update the shell variable "lasterror".

  @param[in] ErrorCode      the error code to put into lasterror
**/
EFI_STATUS
EFIAPI
SetLastError(
  IN CONST SHELL_STATUS   ErrorCode
  );

/**
  Sets all the alias' that were registered with the ShellCommandLib library.

  @retval EFI_SUCCESS           all init commands were run successfully.
**/
EFI_STATUS
EFIAPI
SetBuiltInAlias(
  VOID
  );

/**
  This function will populate the 2 device path protocol parameters based on the
  global gImageHandle.  the DevPath will point to the device path for the handle that has
  loaded image protocol installed on it.  the FilePath will point to the device path
  for the file that was loaded.

  @param[in, out] DevPath       on a successful return the device path to the loaded image
  @param[in, out] FilePath      on a successful return the device path to the file

  @retval EFI_SUCCESS           the 2 device paths were successfully returned.
  @return other                 a error from gBS->HandleProtocol

  @sa HandleProtocol
**/
EFI_STATUS
EFIAPI
GetDevicePathsForImageAndFile (
  IN OUT EFI_DEVICE_PATH_PROTOCOL **DevPath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL **FilePath
  );

/**
  Process all Uefi Shell 2.0 command line options.

  see Uefi Shell 2.0 section 3.2 for full details.

  the command line should resemble the following:

  shell.efi [ShellOpt-options] [options] [file-name [file-name-options]]

  ShellOpt options  Options which control the initialization behavior of the shell.
                    These options are read from the EFI global variable "ShellOpt"
                    and are processed before options or file-name.

  options           Options which control the initialization behavior of the shell.

  file-name         The name of a UEFI shell application or script to be executed
                    after initialization is complete. By default, if file-name is
                    specified, then -nostartup is implied. Scripts are not supported
                    by level 0.

  file-nameoptions  The command-line options that are passed to file-name when it
                    is invoked.

  This will initialize the ShellInitSettings global variable.

  @retval EFI_SUCCESS           the variable is initialized.
**/
EFI_STATUS
EFIAPI
ProcessCommandLine(
  VOID
  );

/**
  Handles all interaction with the default startup script.

  this will check that the correct command line parameters were passed, handle the delay, and then start running the script.

  @param[in] ImagePath          The path to the image for shell.  The first place to look for the startup script.
  @param[in] FilePath           The path to the file for shell.  The second place to look for the startup script.

  @retval EFI_SUCCESS           The variable is initialized.
**/
EFI_STATUS
EFIAPI
DoStartupScript(
  IN EFI_DEVICE_PATH_PROTOCOL *ImagePath,
  IN EFI_DEVICE_PATH_PROTOCOL *FilePath
  );

/**
  Function to perform the shell prompt looping.  It will do a single prompt,
  dispatch the result, and then return.  It is expected that the caller will
  call this function in a loop many times.

  @retval EFI_SUCCESS
  @retval RETURN_ABORTED
**/
EFI_STATUS
EFIAPI
DoShellPrompt (
  VOID
  );

/**
  Add a buffer to the Buffer To Free List for safely returning buffers to other
  places without risking letting them modify internal shell information.

  @param Buffer   Something to pass to FreePool when the shell is exiting.
**/
VOID*
EFIAPI
AddBufferToFreeList(
  VOID *Buffer
  );

/**
  Add a buffer to the Command History List.

  @param Buffer[in]     The line buffer to add.
**/
VOID
EFIAPI
AddLineToCommandHistory(
  IN CONST CHAR16 *Buffer
  );

/**
  Function will process and run a command line.

  This will determine if the command line represents an internal shell command or dispatch an external application.

  @param[in] CmdLine  the command line to parse

  @retval EFI_SUCCESS     the command was completed
  @retval EFI_ABORTED     the command's operation was aborted
**/
EFI_STATUS
EFIAPI
RunCommand(
  IN CONST CHAR16   *CmdLine
  );

/**
  Function determines if the CommandName COULD be a valid command.  It does not determine whether
  this is a valid command.  It only checks for invalid characters.

  @param[in] CommandName    The name to check

  @retval TRUE              CommandName could be a command name
  @retval FALSE             CommandName could not be a valid command name
**/
BOOLEAN
EFIAPI
IsValidCommandName(
  IN CONST CHAR16     *CommandName
  );

/**
  Function to process a NSH script file via SHELL_FILE_HANDLE.

  @param[in] Handle             The handle to the already opened file.
  @param[in] Name               The name of the script file.

  @retval EFI_SUCCESS           the script completed successfully
**/
EFI_STATUS
EFIAPI
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
EFIAPI
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
CONST CHAR16*
EFIAPI
FindFirstCharacter(
  IN CONST CHAR16 *String,
  IN CONST CHAR16 *CharacterList,
  IN CONST CHAR16 EscapeCharacter
  );

#endif //_SHELL_INTERNAL_HEADER_

