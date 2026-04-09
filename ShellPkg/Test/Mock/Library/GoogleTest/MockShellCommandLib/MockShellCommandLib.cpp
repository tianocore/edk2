/** @file MockShellCommandLib.cpp
  Google Test mocks for ShellCommandLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockShellCommandLib.h>

MOCK_INTERFACE_DEFINITION (MockShellCommandLib);

MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandRegisterCommandName, 8, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandRunCommandHandler, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetManFileNameHandler, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetCommandList, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandRegisterAlias, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetInitAliasList, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandIsOnAliasList, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandIsCommandOnList, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetCommandHelp, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, CommandInit, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetEchoState, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandSetEchoState, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandRegisterExit, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetExitCode, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetExit, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetScriptExit, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetCurrentScriptFile, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandSetNewScript, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, DeleteScriptFileStruct, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandGetProfileList, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandCreateNewMappingName, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandConsistMappingInitialize, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandConsistMappingUnInitialize, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandConsistMappingGenMappingName, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandFindMapItem, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandAddMapItemAndUpdatePath, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandCreateInitialMappingsAndPaths, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellCommandUpdateMapping, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ConvertShellHandleToEfiFileProtocol, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellFileHandleRemove, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ConvertEfiFileProtocolToShellHandle, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellFileHandleGetPath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellFileHandleEof, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, FreeBufferList, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, DumpHex, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, CatSDumpHex, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockShellCommandLib, ShellSortFileList, 3, EFIAPI);
