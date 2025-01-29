/** @file MockShellCommandLib.h
  Google Test mocks for ShellCommandLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SHELL_COMMAND_LIB_H_
#define MOCK_SHELL_COMMAND_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/ShellCommandLib.h>
}

struct MockShellCommandLib {
  MOCK_INTERFACE_DECLARATION (MockShellCommandLib);

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    ShellCommandRegisterCommandName,
    (IN CONST CHAR16                  *CommandString,
     IN       SHELL_RUN_COMMAND       CommandHandler,
     IN       SHELL_GET_MAN_FILENAME  GetManFileName,
     IN       UINT32                  ShellMinSupportLevel,
     IN CONST CHAR16                  *ProfileName,
     IN CONST BOOLEAN                 CanAffectLE,
     IN CONST EFI_HII_HANDLE          HiiHandle,
     IN CONST EFI_STRING_ID           ManFormatHelp)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    ShellCommandRunCommandHandler,
    (IN CONST CHAR16        *CommandString,
     IN OUT   SHELL_STATUS  *RetVal,
     IN OUT   BOOLEAN       *CanAffectLE OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellCommandGetManFileNameHandler,
    (IN CONST CHAR16  *CommandString)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST COMMAND_LIST *,
    ShellCommandGetCommandList,
    (IN CONST BOOLEAN  Sort)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    ShellCommandRegisterAlias,
    (IN CONST CHAR16  *Command,
     IN CONST CHAR16  *Alias)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST ALIAS_LIST *,
    ShellCommandGetInitAliasList,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellCommandIsOnAliasList,
    (IN CONST CHAR16  *Alias)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellCommandIsCommandOnList,
    (IN CONST CHAR16  *CommandString)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ShellCommandGetCommandHelp,
    (IN CONST CHAR16  *CommandString)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    CommandInit,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellCommandGetEchoState,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    ShellCommandSetEchoState,
    (IN BOOLEAN  State)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    ShellCommandRegisterExit,
    (IN       BOOLEAN  ScriptOnly,
     IN CONST UINT64   ErrorCode)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT64,
    ShellCommandGetExitCode,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellCommandGetExit,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellCommandGetScriptExit,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    SCRIPT_FILE *,
    ShellCommandGetCurrentScriptFile,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    SCRIPT_FILE *,
    ShellCommandSetNewScript,
    (IN SCRIPT_FILE  *Script OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    DeleteScriptFileStruct,
    (IN SCRIPT_FILE  *Script)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellCommandGetProfileList,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ShellCommandCreateNewMappingName,
    (IN CONST SHELL_MAPPING_TYPE  Type)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandConsistMappingInitialize,
    (EFI_DEVICE_PATH_PROTOCOL  ***Table)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandConsistMappingUnInitialize,
    (EFI_DEVICE_PATH_PROTOCOL  **Table)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ShellCommandConsistMappingGenMappingName,
    (IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
     IN EFI_DEVICE_PATH_PROTOCOL  **Table)
    );

  MOCK_FUNCTION_DECLARATION (
    SHELL_MAP_LIST *,
    ShellCommandFindMapItem,
    (IN CONST CHAR16  *MapKey)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandAddMapItemAndUpdatePath,
    (IN CONST CHAR16                    *Name,
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
     IN CONST UINT64                    Flags,
     IN CONST BOOLEAN                   Path)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandCreateInitialMappingsAndPaths,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandUpdateMapping,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_FILE_PROTOCOL *,
    ConvertShellHandleToEfiFileProtocol,
    (IN CONST SHELL_FILE_HANDLE  Handle)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellFileHandleRemove,
    (IN CONST SHELL_FILE_HANDLE  Handle)
    );

  MOCK_FUNCTION_DECLARATION (
    SHELL_FILE_HANDLE,
    ConvertEfiFileProtocolToShellHandle,
    (IN CONST EFI_FILE_PROTOCOL  *Handle,
     IN CONST CHAR16             *Path)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellFileHandleGetPath,
    (IN CONST SHELL_FILE_HANDLE  Handle)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellFileHandleEof,
    (IN SHELL_FILE_HANDLE  Handle)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    FreeBufferList,
    (IN BUFFER_LIST  *List)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    DumpHex,
    (IN UINTN  Indent,
     IN UINTN  Offset,
     IN UINTN  DataSize,
     IN VOID   *UserData)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    CatSDumpHex,
    (IN CHAR16  *Buffer,
     IN UINTN   Indent,
     IN UINTN   Offset,
     IN UINTN   DataSize,
     IN VOID    *UserData)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellSortFileList,
    (IN OUT EFI_SHELL_FILE_INFO   **FileList,
     OUT    EFI_SHELL_FILE_INFO   **Duplicates OPTIONAL,
     IN     SHELL_SORT_FILE_LIST  Order)
    );
};

#endif
