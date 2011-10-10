/** @file
  Main file for Comp shell Debug1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"

/**
  Function for 'comp' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunComp (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  UINTN               LoopVar;
  SHELL_FILE_HANDLE   FileHandle1;
  SHELL_FILE_HANDLE   FileHandle2;
  UINT8               ErrorCount;
  UINT64              Size1;
  UINT64              Size2;
  UINT8               DataFromFile1;
  UINT8               DataFromFile2;
  UINT8               ADF_File11;
  UINT8               ADF_File12;
  UINT8               ADF_File13;
  UINT8               ADF_File21;
  UINT8               ADF_File22;
  UINT8               ADF_File23;
  UINTN               DataSizeFromFile1;
  UINTN               DataSizeFromFile2;
  CHAR16              *FileName1;
  CHAR16              *FileName2;
  CONST CHAR16        *TempParam;

  ErrorCount          = 0;
  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  FileName1           = NULL;
  FileName2           = NULL;
  FileHandle1         = NULL;
  FileHandle2         = NULL;
  Size1               = 0;

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
      TempParam = ShellCommandLineGetRawValue(Package, 1);
      ASSERT(TempParam != NULL);
      FileName1 = ShellFindFilePath(TempParam);
      if (FileName1 == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, TempParam);
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        Status = ShellOpenFileByName(FileName1, &FileHandle1, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, TempParam, Status);
          ShellStatus = SHELL_NOT_FOUND;
        }
      }
      TempParam = ShellCommandLineGetRawValue(Package, 2);
      ASSERT(TempParam != NULL);
      FileName2 = ShellFindFilePath(TempParam);
      if (FileName2 == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, TempParam);
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        Status = ShellOpenFileByName(FileName2, &FileHandle2, EFI_FILE_MODE_READ, 0);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, TempParam, Status);
          ShellStatus = SHELL_NOT_FOUND;
        }
      }
      if (ShellStatus == SHELL_SUCCESS) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_COMP_HEADER), gShellDebug1HiiHandle, FileName1, FileName2);
        Status = gEfiShellProtocol->GetFileSize(FileHandle1, &Size1);
        ASSERT_EFI_ERROR(Status);
        Status = gEfiShellProtocol->GetFileSize(FileHandle2, &Size2);
        ASSERT_EFI_ERROR(Status);
        if (Size1 != Size2) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_COMP_SIZE_FAIL), gShellDebug1HiiHandle);
          ErrorCount++;
          ShellStatus = SHELL_NOT_EQUAL;
        }
      }
      if (ShellStatus == SHELL_SUCCESS) {
        for (LoopVar = 0 ; LoopVar < Size1 && ErrorCount <= 10 ; LoopVar++) {
          DataSizeFromFile1 = 1;
          DataSizeFromFile2 = 1;
          Status = gEfiShellProtocol->ReadFile(FileHandle1, &DataSizeFromFile1, &DataFromFile1);
          ASSERT_EFI_ERROR(Status);
          Status = gEfiShellProtocol->ReadFile(FileHandle2, &DataSizeFromFile2, &DataFromFile2);
          ASSERT_EFI_ERROR(Status);
          if (DataFromFile1 != DataFromFile2) {
            ADF_File11 = 0;
            ADF_File12 = 0;
            ADF_File13 = 0;
            ADF_File21 = 0;
            ADF_File22 = 0;
            ADF_File23 = 0;
            if (LoopVar + 1 < Size1) {
              LoopVar++;
              DataSizeFromFile1 = 1;
              DataSizeFromFile2 = 1;
              Status = gEfiShellProtocol->ReadFile(FileHandle1, &DataSizeFromFile1, &ADF_File11);
              ASSERT_EFI_ERROR(Status);
              Status = gEfiShellProtocol->ReadFile(FileHandle2, &DataSizeFromFile2, &ADF_File21);
              ASSERT_EFI_ERROR(Status);
              if (LoopVar + 1 < Size1) {
                LoopVar++;
                DataSizeFromFile1 = 1;
                DataSizeFromFile2 = 1;
                Status = gEfiShellProtocol->ReadFile(FileHandle1, &DataSizeFromFile1, &ADF_File12);
                ASSERT_EFI_ERROR(Status);
                Status = gEfiShellProtocol->ReadFile(FileHandle2, &DataSizeFromFile2, &ADF_File22);
                ASSERT_EFI_ERROR(Status);
                if (LoopVar + 1 < Size1) {
                  LoopVar++;
                  DataSizeFromFile1 = 1;
                  DataSizeFromFile2 = 1;
                  Status = gEfiShellProtocol->ReadFile(FileHandle1, &DataSizeFromFile1, &ADF_File13);
                  ASSERT_EFI_ERROR(Status);
                  Status = gEfiShellProtocol->ReadFile(FileHandle2, &DataSizeFromFile2, &ADF_File23);
                  ASSERT_EFI_ERROR(Status);
                }
              }
            }
            if (ADF_File13 != ADF_File23) {
              ShellPrintHiiEx(
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_COMP_SPOT_FAIL4),
                gShellDebug1HiiHandle,
                ++ErrorCount,
                FileName1,
                LoopVar,
                DataFromFile1, ADF_File11, ADF_File12, ADF_File13,
                DataFromFile1, ADF_File11, ADF_File12, ADF_File13,
                FileName2,
                LoopVar,
                DataFromFile2, ADF_File21, ADF_File22, ADF_File23,
                DataFromFile2, ADF_File21, ADF_File22, ADF_File23
               );
            } else if (ADF_File12 != ADF_File22) {
              ShellPrintHiiEx(
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_COMP_SPOT_FAIL3),
                gShellDebug1HiiHandle,
                ++ErrorCount,
                FileName1,
                LoopVar,
                DataFromFile1, ADF_File11, ADF_File12,
                DataFromFile1, ADF_File11, ADF_File12,
                FileName2,
                LoopVar,
                DataFromFile2, ADF_File21, ADF_File22,
                DataFromFile2, ADF_File21, ADF_File22
               );
            } else if (ADF_File11 != ADF_File21) {
              ShellPrintHiiEx(
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_COMP_SPOT_FAIL2),
                gShellDebug1HiiHandle,
                ++ErrorCount,
                FileName1,
                LoopVar,
                DataFromFile1, ADF_File11,
                DataFromFile1, ADF_File11,
                FileName2,
                LoopVar,
                DataFromFile2, ADF_File21,
                DataFromFile2, ADF_File21
               );
            } else {
              ShellPrintHiiEx(
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_COMP_SPOT_FAIL1),
                gShellDebug1HiiHandle,
                ++ErrorCount,
                FileName1,
                LoopVar,
                DataFromFile1,
                DataFromFile1,
                FileName2,
                LoopVar,
                DataFromFile2,
                DataFromFile2
               );
            }
            ShellStatus = SHELL_NOT_EQUAL;
          }
        }
        if (ErrorCount == 0) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_COMP_FOOTER_PASS), gShellDebug1HiiHandle);
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_COMP_FOOTER_FAIL), gShellDebug1HiiHandle);
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  SHELL_FREE_NON_NULL(FileName1);
  SHELL_FREE_NON_NULL(FileName2);

  if (FileHandle1 != NULL) {
    gEfiShellProtocol->CloseFile(FileHandle1);
  }
  if (FileHandle2 != NULL) {
    gEfiShellProtocol->CloseFile(FileHandle2);
  }

  return (ShellStatus);
}
