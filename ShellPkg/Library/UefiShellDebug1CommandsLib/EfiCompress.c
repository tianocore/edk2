/** @file
  Main file for EfiCompress shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include "Compress.h"

/**
  Compress the full contents of an input file and write the compressed payload
  to an output file handle.

  @param[in] InShellFileHandle   Source file to read and compress.
  @param[in] OutShellFileHandle  Destination file to receive compressed data.
  @param[in] OutFileName         Name of the output file for error reporting.

  @retval SHELL_SUCCESS            Compression and write completed successfully.
  @retval SHELL_OUT_OF_RESOURCES   A required buffer allocation failed.
  @retval SHELL_DEVICE_ERROR       Read, compress, or write failed.
**/
STATIC
SHELL_STATUS
CompressFile (
  IN SHELL_FILE_HANDLE  InShellFileHandle,
  IN SHELL_FILE_HANDLE  OutShellFileHandle,
  IN CONST CHAR16       *OutFileName
  )
{
  EFI_STATUS    Status;
  UINT64        OutSize;
  UINTN         OutSize2;
  VOID          *OutBuffer;
  UINT64        InSize;
  UINTN         InSize2;
  VOID          *InBuffer;
  SHELL_STATUS  ShellStatus;

  OutSize     = 0;
  OutBuffer   = NULL;
  InBuffer    = NULL;
  ShellStatus = SHELL_SUCCESS;

  Status = gEfiShellProtocol->GetFileSize (InShellFileHandle, &InSize);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, Status);
    return SHELL_DEVICE_ERROR;
  }

  InBuffer = AllocateZeroPool ((UINTN)InSize);
  if (InBuffer == NULL) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, EFI_OUT_OF_RESOURCES);
    return SHELL_OUT_OF_RESOURCES;
  }

  InSize2 = (UINTN)InSize;
  Status  = gEfiShellProtocol->ReadFile (InShellFileHandle, &InSize2, InBuffer);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, Status);
    ShellStatus = SHELL_DEVICE_ERROR;
    goto Exit;
  }

  InSize = InSize2;
  Status = Compress (InBuffer, InSize, OutBuffer, &OutSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    OutBuffer = AllocateZeroPool ((UINTN)OutSize);
    if (OutBuffer == NULL) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, EFI_OUT_OF_RESOURCES);
      ShellStatus = SHELL_OUT_OF_RESOURCES;
      goto Exit;
    }

    Status = Compress (InBuffer, InSize, OutBuffer, &OutSize);
  }

  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, Status);
    ShellStatus = SHELL_DEVICE_ERROR;
    goto Exit;
  }

  OutSize2 = (UINTN)OutSize;
  Status   = gEfiShellProtocol->WriteFile (OutShellFileHandle, &OutSize2, OutBuffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_WRITE_FAIL), gShellDebug1HiiHandle, L"eficompress", OutFileName);
    ShellStatus = SHELL_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  SHELL_FREE_NON_NULL (InBuffer);
  SHELL_FREE_NON_NULL (OutBuffer);

  return ShellStatus;
}

/**
  Validate that a path is not a directory and open it with the requested mode.

  @param[in]  FileName         Path to open.
  @param[in]  OpenMode         Mode passed to ShellOpenFileByName().
  @param[out] ShellFileHandle  Opened shell file handle.

  @retval SHELL_SUCCESS            The file was opened successfully.
  @retval SHELL_INVALID_PARAMETER  FileName names a directory.
  @retval SHELL_NOT_FOUND          The file could not be opened.
**/
STATIC
SHELL_STATUS
OpenFileHelper (
  IN  CONST CHAR16       *FileName,
  IN  UINT64             OpenMode,
  OUT SHELL_FILE_HANDLE  *ShellFileHandle
  )
{
  EFI_STATUS  Status;

  ASSERT (ShellFileHandle != NULL);

  if (ShellIsDirectory (FileName) == EFI_SUCCESS) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, L"eficompress", FileName);
    return SHELL_INVALID_PARAMETER;
  }

  Status = ShellOpenFileByName (FileName, ShellFileHandle, OpenMode, 0);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"eficompress", FileName);
    return SHELL_NOT_FOUND;
  }

  return SHELL_SUCCESS;
}

/** Main function of the 'EfiCompress' command.

  @param[in] Package    List of input parameter for the command.
**/
STATIC
SHELL_STATUS
MainCmdEfiCompress (
  LIST_ENTRY  *Package
  )
{
  SHELL_STATUS       ShellStatus;
  SHELL_FILE_HANDLE  InShellFileHandle;
  SHELL_FILE_HANDLE  OutShellFileHandle;
  CHAR16             *InFileName;
  CONST CHAR16       *OutFileName;
  CONST CHAR16       *TempParam;

  InFileName         = NULL;
  OutFileName        = NULL;
  ShellStatus        = SHELL_SUCCESS;
  InShellFileHandle  = NULL;
  OutShellFileHandle = NULL;

  if (ShellCommandLineGetCount (Package) > 3) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"eficompress");
    return SHELL_INVALID_PARAMETER;
  } else if (ShellCommandLineGetCount (Package) < 3) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"eficompress");
    return SHELL_INVALID_PARAMETER;
  }

  TempParam = ShellCommandLineGetRawValue (Package, 1);
  if (TempParam == NULL) {
    ASSERT (TempParam != NULL);
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"eficompress");
    return SHELL_INVALID_PARAMETER;
  }

  InFileName  = ShellFindFilePath (TempParam);
  OutFileName = (CHAR16 *)ShellCommandLineGetRawValue (Package, 2);
  if ((InFileName == NULL) || (OutFileName == NULL)) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, L"eficompress", TempParam);
    ShellStatus = SHELL_NOT_FOUND;
    goto Exit;
  }

  ShellStatus = OpenFileHelper (InFileName, EFI_FILE_MODE_READ, &InShellFileHandle);
  if (ShellStatus != SHELL_SUCCESS) {
    goto Exit;
  }

  ShellStatus = OpenFileHelper (OutFileName, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, &OutShellFileHandle);
  if (ShellStatus != SHELL_SUCCESS) {
    goto Exit;
  }

  ShellStatus = CompressFile (InShellFileHandle, OutShellFileHandle, OutFileName);

Exit:
  if (InShellFileHandle != NULL) {
    gEfiShellProtocol->CloseFile (InShellFileHandle);
  }

  if (OutShellFileHandle != NULL) {
    gEfiShellProtocol->CloseFile (OutShellFileHandle);
  }

  SHELL_FREE_NON_NULL (InFileName);

  return ShellStatus;
}

/**
  Function for 'compress' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEfiCompress (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;
  Package     = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"eficompress", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }

    return ShellStatus;
  }

  ShellStatus = MainCmdEfiCompress (Package);

  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}
