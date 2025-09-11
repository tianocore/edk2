/** @file
  function definitions for internal to shell functions.

  (C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SHELL_INTERNAL_HEADER_
#define _SHELL_INTERNAL_HEADER_

#include <Uefi.h>

#include <Guid/ShellVariableGuid.h>
#include <Guid/ShellAliasGuid.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/Shell.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <Protocol/ShellParameters.h>
#include <Protocol/BlockIo.h>
#include <Protocol/HiiPackageList.h>

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
#include <Library/UefiHiiServicesLib.h>

#include "ShellParametersProtocol.h"
#include "ShellProtocol.h"
#include "ShellEnvVar.h"
#include "ConsoleLogger.h"
#include "ShellManParser.h"
#include "ConsoleWrappers.h"
#include "FileHandleWrappers.h"

extern CONST CHAR16  mNoNestingEnvVarName[];
extern CONST CHAR16  mNoNestingTrue[];
extern CONST CHAR16  mNoNestingFalse[];

typedef struct {
  LIST_ENTRY           Link;        ///< Standard linked list handler.
  SHELL_FILE_HANDLE    SplitStdOut; ///< ConsoleOut for use in the split.
  SHELL_FILE_HANDLE    SplitStdIn;  ///< ConsoleIn for use in the split.
} SPLIT_LIST;

typedef struct {
  UINT32    Startup      : 1; ///< Was "-startup"       found on command line.
  UINT32    NoStartup    : 1; ///< Was "-nostartup"     found on command line.
  UINT32    NoConsoleOut : 1; ///< Was "-noconsoleout"  found on command line.
  UINT32    NoConsoleIn  : 1; ///< Was "-noconsolein"   found on command line.
  UINT32    NoInterrupt  : 1; ///< Was "-nointerrupt"   found on command line.
  UINT32    NoMap        : 1; ///< Was "-nomap"         found on command line.
  UINT32    NoVersion    : 1; ///< Was "-noversion"     found on command line.
  UINT32    Delay        : 1; ///< Was "-delay[:n]      found on command line
  UINT32    Exit         : 1; ///< Was "-_exit"         found on command line
  UINT32    NoNest       : 1; ///< Was "-nonest"        found on command line
  UINT32    Reserved     : 7; ///< Extra bits
} SHELL_BITS;

typedef union {
  SHELL_BITS    Bits;
  UINT16        AllBits;
} SHELL_BIT_UNION;

typedef struct {
  SHELL_BIT_UNION    BitUnion;
  UINTN              Delay;        ///< Seconds of delay default:5.
  CHAR16             *FileName;    ///< Filename to run upon successful initialization.
  CHAR16             *FileOptions; ///< Options to pass to FileName.
} SHELL_INIT_SETTINGS;

typedef struct {
  BUFFER_LIST    CommandHistory;
  UINTN          VisibleRowNumber;
  UINTN          OriginalVisibleRowNumber;
  BOOLEAN        InsertMode;                        ///< Is the current typing mode insert (FALSE = overwrite).
} SHELL_VIEWING_SETTINGS;

typedef struct {
  EFI_SHELL_PARAMETERS_PROTOCOL    *NewShellParametersProtocol;
  EFI_SHELL_PROTOCOL               *NewEfiShellProtocol;
  BOOLEAN                          PageBreakEnabled;
  BOOLEAN                          RootShellInstance;
  SHELL_INIT_SETTINGS              ShellInitSettings;
  BUFFER_LIST                      BufferToFreeList;  ///< List of buffers that were returned to the user to free.
  SHELL_VIEWING_SETTINGS           ViewingSettings;
  EFI_HII_HANDLE                   HiiHandle;           ///< Handle from HiiLib.
  UINTN                            LogScreenCount;      ///< How many screens of log information to save.
  EFI_EVENT                        UserBreakTimer;      ///< Timer event for polling for CTRL-C.
  EFI_DEVICE_PATH_PROTOCOL         *ImageDevPath;       ///< DevicePath for ourselves.
  EFI_DEVICE_PATH_PROTOCOL         *FileDevPath;        ///< DevicePath for ourselves.
  CONSOLE_LOGGER_PRIVATE_DATA      *ConsoleInfo;        ///< Pointer for ConsoleInformation.
  EFI_SHELL_PARAMETERS_PROTOCOL    *OldShellParameters; ///< old shell parameters to reinstall upon exiting.
  SHELL_PROTOCOL_HANDLE_LIST       OldShellList;        ///< List of other instances to reinstall when closing.
  SPLIT_LIST                       SplitList;           ///< List of Splits in FILO stack.
  VOID                             *CtrlCNotifyHandle1; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlCNotifyHandle2; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlCNotifyHandle3; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlCNotifyHandle4; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlSNotifyHandle1; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlSNotifyHandle2; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlSNotifyHandle3; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  VOID                             *CtrlSNotifyHandle4; ///< The NotifyHandle returned from SimpleTextInputEx.RegisterKeyNotify.
  BOOLEAN                          HaltOutput;          ///< TRUE to start a CTRL-S halt.
} SHELL_INFO;

#pragma pack(1)
///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} SHELL_MAN_HII_VENDOR_DEVICE_PATH;
#pragma pack()

extern SHELL_INFO  ShellInfoObject;

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
GetDevicePathsForImageAndFile (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevPath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath
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
ProcessCommandLine (
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
DoStartupScript (
  IN EFI_DEVICE_PATH_PROTOCOL  *ImagePath,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Function to perform the shell prompt looping.  It will do a single prompt,
  dispatch the result, and then return.  It is expected that the caller will
  call this function in a loop many times.

  @retval EFI_SUCCESS
  @retval RETURN_ABORTED
**/
EFI_STATUS
DoShellPrompt (
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

#endif //_SHELL_INTERNAL_HEADER_
