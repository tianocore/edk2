/** @file MockShellLib.h
  Google Test mocks for ShellLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SHELL_LIB_H_
#define MOCK_SHELL_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/ShellLib.h>
}

struct MockShellLib {
  MOCK_INTERFACE_DECLARATION (MockShellLib);

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    FullyQualifyPath,
    (IN CONST CHAR16  *Path)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_FILE_INFO *,
    ShellGetFileInfo,
    (IN SHELL_FILE_HANDLE  FileHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellSetFileInfo,
    (IN SHELL_FILE_HANDLE  FileHandle,
     IN EFI_FILE_INFO      *FileInfo)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellOpenFileByDevicePath,
    (IN OUT EFI_DEVICE_PATH_PROTOCOL  **FilePath,
     OUT    SHELL_FILE_HANDLE         *FileHandle,
     IN     UINT64                    OpenMode,
     IN     UINT64                    Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellOpenFileByName,
    (IN CONST CHAR16             *FileName,
     OUT      SHELL_FILE_HANDLE  *FileHandle,
     IN       UINT64             OpenMode,
     IN       UINT64             Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCreateDirectory,
    (IN CONST CHAR16             *DirectoryName,
     OUT      SHELL_FILE_HANDLE  *FileHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellReadFile,
    (IN     SHELL_FILE_HANDLE  FileHandle,
     IN OUT UINTN              *ReadSize,
     OUT    VOID               *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellWriteFile,
    (IN     SHELL_FILE_HANDLE  FileHandle,
     IN OUT UINTN              *BufferSize,
     IN     VOID               *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCloseFile,
    (IN SHELL_FILE_HANDLE  *FileHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellDeleteFile,
    (IN SHELL_FILE_HANDLE  *FileHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellSetFilePosition,
    (IN SHELL_FILE_HANDLE  FileHandle,
     IN UINT64             Position)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellGetFilePosition,
    (IN  SHELL_FILE_HANDLE  FileHandle,
     OUT UINT64             *Position)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellFlushFile,
    (IN SHELL_FILE_HANDLE  FileHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellFindFirstFile,
    (IN  SHELL_FILE_HANDLE  DirHandle,
     OUT EFI_FILE_INFO      **Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellFindNextFile,
    (IN     SHELL_FILE_HANDLE  DirHandle,
     IN OUT EFI_FILE_INFO      *Buffer,
     IN OUT BOOLEAN            *NoFile)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellGetFileSize,
    (IN  SHELL_FILE_HANDLE  FileHandle,
     OUT UINT64             *Size)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellGetExecutionBreakFlag,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellGetEnvironmentVariable,
    (IN CONST CHAR16  *EnvKey)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellSetEnvironmentVariable,
    (IN CONST CHAR16   *EnvKey,
     IN CONST CHAR16   *EnvVal,
     IN       BOOLEAN  Volatile)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellExecute,
    (IN  EFI_HANDLE  *ParentHandle,
     IN  CHAR16      *CommandLine,
     IN  BOOLEAN     Output,
     IN  CHAR16      **EnvironmentVariables,
     OUT EFI_STATUS  *Status)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellGetCurrentDir,
    (IN CHAR16  *CONST DeviceName OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    ShellSetPageBreakMode,
    (IN BOOLEAN  CurrentState)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellOpenFileMetaArg,
    (IN     CHAR16               *Arg,
     IN     UINT64               OpenMode,
     IN OUT EFI_SHELL_FILE_INFO  **ListHead)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCloseFileMetaArg,
    (IN OUT EFI_SHELL_FILE_INFO  **ListHead)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ShellFindFilePath,
    (IN CONST CHAR16  *FileName)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ShellFindFilePathEx,
    (IN CONST CHAR16  *FileName,
     IN CONST CHAR16  *FileExtension)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandLineParseEx,
    (IN CONST SHELL_PARAM_ITEM  *CheckList,
     OUT      LIST_ENTRY        **CheckPackage,
     OUT      CHAR16            **ProblemParam OPTIONAL,
     IN       BOOLEAN           AutoPageBreak,
     IN       BOOLEAN           AlwaysAllowNumbers)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    ShellCommandLineFreeVarList,
    (IN LIST_ENTRY  *CheckPackage)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellCommandLineGetFlag,
    (IN CONST LIST_ENTRY  *CONST CheckPackage,
     IN CONST CHAR16      *CONST KeyString)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellCommandLineGetValue,
    (IN CONST LIST_ENTRY  *CheckPackage,
     IN       CHAR16      *KeyString)
    );

  MOCK_FUNCTION_DECLARATION (
    CONST CHAR16 *,
    ShellCommandLineGetRawValue,
    (IN CONST LIST_ENTRY  *CONST CheckPackage,
     IN       UINTN       Position)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    ShellCommandLineGetCount,
    (IN CONST LIST_ENTRY  *CheckPackage)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCommandLineCheckDuplicate,
    (IN CONST LIST_ENTRY  *CheckPackage,
     OUT      CHAR16      **Param)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellInitialize,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellIsDirectory,
    (IN CONST CHAR16  *DirName)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellIsFile,
    (IN CONST CHAR16  *Name)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellIsFileInPath,
    (IN CONST CHAR16  *Name)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    ShellStrToUintn,
    (IN CONST CHAR16  *String)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    ShellHexStrToUintn,
    (IN CONST CHAR16  *String)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    StrnCatGrow,
    (IN OUT   CHAR16  **Destination,
     IN OUT   UINTN   *CurrentSize,
     IN CONST CHAR16  *Source,
     IN       UINTN   Count)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellCopySearchAndReplace,
    (IN CONST CHAR16   *SourceString,
     IN OUT   CHAR16   *NewString,
     IN       UINTN    NewSize,
     IN CONST CHAR16   *FindTarget,
     IN CONST CHAR16   *ReplaceWith,
     IN CONST BOOLEAN  SkipPreCarrot,
     IN CONST BOOLEAN  ParameterReplacing)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellIsHexaDecimalDigitCharacter,
    (IN CHAR16  Char)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellIsDecimalDigitCharacter,
    (IN CHAR16  Char)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellPromptForResponse,
    (IN     SHELL_PROMPT_REQUEST_TYPE  Type,
     IN     CHAR16                     *Prompt OPTIONAL,
     IN OUT VOID                       **Response OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellPromptForResponseHii,
    (IN       SHELL_PROMPT_REQUEST_TYPE  Type,
     IN CONST EFI_STRING_ID              HiiFormatStringId,
     IN CONST EFI_HII_HANDLE             HiiFormatHandle,
     IN OUT   VOID                       **Response)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    ShellIsHexOrDecimalNumber,
    (IN CONST CHAR16   *String,
     IN CONST BOOLEAN  ForceHex,
     IN CONST BOOLEAN  StopAtSpace)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellConvertStringToUint64,
    (IN CONST CHAR16   *String,
     OUT      UINT64   *Value,
     IN CONST BOOLEAN  ForceHex,
     IN CONST BOOLEAN  StopAtSpace)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellFileExists,
    (IN CONST CHAR16  *Name)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ShellFileHandleReturnLine,
    (IN     SHELL_FILE_HANDLE  Handle,
     IN OUT BOOLEAN            *Ascii)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellFileHandleReadLine,
    (IN     SHELL_FILE_HANDLE  Handle,
     IN OUT CHAR16             *Buffer,
     IN OUT UINTN              *Size,
     IN     BOOLEAN            Truncate,
     IN OUT BOOLEAN            *Ascii)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellDeleteFileByName,
    (IN CONST CHAR16  *FileName)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ShellPrintHelp,
    (IN CONST CHAR16   *CommandToGetHelpOn,
     IN CONST CHAR16   *SectionToGetHelpOn,
     IN       BOOLEAN  PrintCommandText)
    );
};

#endif
