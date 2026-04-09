/** @file MockShellLib.cpp
  Google Test mocks for ShellLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockShellLib.h>

//
// Global Variables that are not const
//
EFI_SHELL_PARAMETERS_PROTOCOL  *gEfiShellParametersProtocol;
EFI_SHELL_PROTOCOL             *gEfiShellProtocol;

MOCK_INTERFACE_DEFINITION (MockShellLib);

MOCK_FUNCTION_DEFINITION (MockShellLib, FullyQualifyPath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellGetFileInfo, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellSetFileInfo, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellOpenFileByDevicePath, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellOpenFileByName, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCreateDirectory, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellReadFile, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellWriteFile, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCloseFile, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellDeleteFile, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellSetFilePosition, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellGetFilePosition, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFlushFile, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFindFirstFile, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFindNextFile, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellGetFileSize, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellGetExecutionBreakFlag, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellGetEnvironmentVariable, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellSetEnvironmentVariable, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellExecute, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellGetCurrentDir, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellSetPageBreakMode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellOpenFileMetaArg, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCloseFileMetaArg, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFindFilePath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFindFilePathEx, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineParseEx, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineFreeVarList, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineGetFlag, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineGetValue, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineGetRawValue, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineGetCount, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCommandLineCheckDuplicate, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellInitialize, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellIsDirectory, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellIsFile, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellIsFileInPath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellStrToUintn, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellHexStrToUintn, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, StrnCatGrow, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellCopySearchAndReplace, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellIsHexaDecimalDigitCharacter, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellIsDecimalDigitCharacter, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellPromptForResponse, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellPromptForResponseHii, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellIsHexOrDecimalNumber, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellConvertStringToUint64, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFileExists, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFileHandleReturnLine, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellFileHandleReadLine, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellDeleteFileByName, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellLib, ShellPrintHelp, 3, EFIAPI);
