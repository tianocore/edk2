/** @file
  Main file for EfiCompress shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include "Compress.h"

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
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  SHELL_FILE_HANDLE   InShellFileHandle;
  SHELL_FILE_HANDLE   OutShellFileHandle;
  UINT64              OutSize;
  UINTN               OutSize2;
  VOID                *OutBuffer;
  UINT64              InSize;
  UINTN               InSize2;
  VOID                *InBuffer;
  CHAR16              *InFileName;
  CONST CHAR16        *OutFileName;
  CONST CHAR16        *TempParam;

  InFileName          = NULL;
  OutFileName         = NULL;
  OutSize             = 0;
  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  OutBuffer           = NULL;
  InShellFileHandle   = NULL;
  OutShellFileHandle  = NULL;
  InBuffer            = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"eficompress", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"eficompress");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) < 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"eficompress");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      TempParam = ShellCommandLineGetRawValue(Package, 1);
      ASSERT(TempParam != NULL);
      InFileName = ShellFindFilePath(TempParam);
      OutFileName = ShellCommandLineGetRawValue(Package, 2);
      if (InFileName == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, L"eficompress", TempParam);  
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        if (ShellIsDirectory(InFileName) == EFI_SUCCESS){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, L"eficompress", InFileName);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        if (ShellIsDirectory(OutFileName) == EFI_SUCCESS){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, L"eficompress", OutFileName);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        if (ShellStatus == SHELL_SUCCESS) {
          Status = ShellOpenFileByName(InFileName, &InShellFileHandle, EFI_FILE_MODE_READ, 0);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"eficompress", ShellCommandLineGetRawValue(Package, 1));  
            ShellStatus = SHELL_NOT_FOUND;
          }
          Status = ShellOpenFileByName(OutFileName, &OutShellFileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"eficompress", ShellCommandLineGetRawValue(Package, 2));  
            ShellStatus = SHELL_NOT_FOUND;
          }
        }
        if (ShellStatus == SHELL_SUCCESS) {
          Status = gEfiShellProtocol->GetFileSize(InShellFileHandle, &InSize);
          ASSERT_EFI_ERROR(Status);
          InBuffer = AllocateZeroPool((UINTN)InSize);
          ASSERT(InBuffer != NULL);
          InSize2 = (UINTN)InSize;
          Status = gEfiShellProtocol->ReadFile(InShellFileHandle, &InSize2, InBuffer);
          InSize = InSize2;
          ASSERT_EFI_ERROR(Status);
          Status = Compress(InBuffer, InSize, OutBuffer, &OutSize);
          if (Status == EFI_BUFFER_TOO_SMALL) {
            OutBuffer = AllocateZeroPool((UINTN)OutSize);
            ASSERT(OutBuffer != NULL);
            Status = Compress(InBuffer, InSize, OutBuffer, &OutSize);
          }
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_EFI_COMPRESS_FAIL), gShellDebug1HiiHandle, Status);
            ShellStatus = SHELL_DEVICE_ERROR;
          } else {
            OutSize2 = (UINTN)OutSize;
            Status = gEfiShellProtocol->WriteFile(OutShellFileHandle, &OutSize2, OutBuffer);
            if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_WRITE_FAIL), gShellDebug1HiiHandle, L"eficompress", OutFileName);  
              ShellStatus = SHELL_DEVICE_ERROR;
            }
          }
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  if (InShellFileHandle != NULL) {
    gEfiShellProtocol->CloseFile(InShellFileHandle);
  }
  if (OutShellFileHandle != NULL) {
    gEfiShellProtocol->CloseFile(OutShellFileHandle);
  }
  SHELL_FREE_NON_NULL(InFileName);
  SHELL_FREE_NON_NULL(InBuffer);
  SHELL_FREE_NON_NULL(OutBuffer);

  return (ShellStatus);
}
