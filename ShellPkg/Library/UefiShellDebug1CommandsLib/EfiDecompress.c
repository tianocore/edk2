/** @file
  Main file for EfiDecompress shell Debug1 function.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/Decompress.h>


SHELL_STATUS
EFIAPI
ShellCommandRunEfiDecompress (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  SHELL_FILE_HANDLE              InFileHandle;
  SHELL_FILE_HANDLE              OutFileHandle;
  UINT32              OutSize;
  UINTN               OutSizeTemp;
  VOID                *OutBuffer;
  UINTN               InSize;
  VOID                *InBuffer;
  CHAR16              *InFileName;
  CONST CHAR16        *OutFileName;
  UINT64              temp;
  UINT32              ScratchSize;
  VOID                *ScratchBuffer;
  EFI_DECOMPRESS_PROTOCOL *Decompress;

  InFileName          = NULL;
  OutFileName         = NULL;
  OutSize             = 0;
  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  OutBuffer           = NULL;
  InBuffer            = NULL;
  ScratchBuffer       = NULL;
  InFileHandle        = NULL;
  OutFileHandle       = NULL;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) < 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      InFileName = ShellFindFilePath(ShellCommandLineGetRawValue(Package, 1));
      OutFileName = ShellCommandLineGetRawValue(Package, 2);
      Status = ShellOpenFileByName(InFileName, &InFileHandle, EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, ShellCommandLineGetRawValue(Package, 1), Status);
        ShellStatus = SHELL_NOT_FOUND;
      }
      Status = ShellOpenFileByName(OutFileName, &OutFileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, ShellCommandLineGetRawValue(Package, 2), Status);
        ShellStatus = SHELL_NOT_FOUND;
      }
      if (FileHandleIsDirectory(InFileHandle) == EFI_SUCCESS){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, InFileName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      if (FileHandleIsDirectory(OutFileHandle) == EFI_SUCCESS){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, OutFileName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
      Status = FileHandleGetSize(InFileHandle, &temp);
      ASSERT(temp <= (UINT32)(-1));
      InSize = (UINTN)temp;
      ASSERT_EFI_ERROR(Status);
      InBuffer = AllocatePool(InSize);
      ASSERT(InBuffer != NULL);
      Status = gEfiShellProtocol->ReadFile(InFileHandle, &InSize, InBuffer);
      ASSERT_EFI_ERROR(Status);

      Status = gBS->LocateProtocol(&gEfiDecompressProtocolGuid, NULL, (VOID**)&Decompress);
      ASSERT_EFI_ERROR(Status);

      Status = Decompress->GetInfo(Decompress, InBuffer, (UINT32)InSize, &OutSize, &ScratchSize);
      ASSERT_EFI_ERROR(Status);

      OutBuffer = AllocatePool(OutSize);
      ScratchBuffer = AllocatePool(ScratchSize);
      ASSERT(OutBuffer != NULL);
      ASSERT(ScratchBuffer != NULL);

      Status = Decompress->Decompress(Decompress, InBuffer, (UINT32)InSize, OutBuffer, OutSize, ScratchBuffer, ScratchSize);
      ASSERT_EFI_ERROR(Status);

      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_EFI_DECOMPRESS_FAIL), gShellDebug1HiiHandle, Status);
        ShellStatus = SHELL_DEVICE_ERROR;
      } else {
        OutSizeTemp = OutSize;
        Status = gEfiShellProtocol->WriteFile(OutFileHandle, &OutSizeTemp, OutBuffer);
        OutSize = (UINT32)OutSizeTemp;
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_WRITE_FAIL), gShellDebug1HiiHandle, OutFileName, Status);
          ShellStatus = SHELL_DEVICE_ERROR;
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  if (InFileName != NULL) {
    FreePool(InFileName);
  }
  if (InFileHandle != NULL) {
    gEfiShellProtocol->CloseFile(InFileHandle);
  }
  if (OutFileHandle != NULL) {
    gEfiShellProtocol->CloseFile(OutFileHandle);
  }
  if (InBuffer != NULL) {
    FreePool(InBuffer);
  }
  if (OutBuffer != NULL) {
    FreePool(OutBuffer);
  }
  if (ScratchBuffer != NULL) {
    FreePool(ScratchBuffer);
  }

  return (ShellStatus);
}
