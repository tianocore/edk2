/** @file
  Main file for EfiDecompress shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/Decompress.h>

/**
  Function for 'decompress' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEfiDecompress (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  LIST_ENTRY               *Package;
  CHAR16                   *ProblemParam;
  SHELL_STATUS             ShellStatus;
  SHELL_FILE_HANDLE        InFileHandle;
  SHELL_FILE_HANDLE        OutFileHandle;
  UINT32                   OutSize;
  UINTN                    OutSizeTemp;
  VOID                     *OutBuffer;
  UINTN                    InSize;
  VOID                     *InBuffer;
  CHAR16                   *InFileName;
  CONST CHAR16             *OutFileName;
  UINT64                   Temp64Bit;
  UINT32                   ScratchSize;
  VOID                     *ScratchBuffer;
  EFI_DECOMPRESS_PROTOCOL  *Decompress;
  CONST CHAR16             *TempParam;

  InFileName    = NULL;
  OutFileName   = NULL;
  OutSize       = 0;
  ScratchSize   = 0;
  ShellStatus   = SHELL_SUCCESS;
  Status        = EFI_SUCCESS;
  OutBuffer     = NULL;
  InBuffer      = NULL;
  ScratchBuffer = NULL;
  InFileHandle  = NULL;
  OutFileHandle = NULL;
  Decompress    = NULL;

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
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"efidecompress", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 3) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"efidecompress");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) < 3) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"efidecompress");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      TempParam = ShellCommandLineGetRawValue (Package, 1);
      if (TempParam == NULL) {
        ASSERT (TempParam != NULL);
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"efidecompress");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      InFileName  = ShellFindFilePath (TempParam);
      OutFileName = ShellCommandLineGetRawValue (Package, 2);
      if ((InFileName == NULL) || (OutFileName == NULL)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_FIND_FAIL), gShellDebug1HiiHandle, L"efidecompress", TempParam);
        ShellStatus = SHELL_NOT_FOUND;
      } else {
        if (ShellIsDirectory (InFileName) == EFI_SUCCESS) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, L"efidecompress", InFileName);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }

        if (ShellIsDirectory (OutFileName) == EFI_SUCCESS) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_NOT_DIR), gShellDebug1HiiHandle, L"efidecompress", OutFileName);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }

        if (ShellStatus == SHELL_SUCCESS) {
          Status = ShellOpenFileByName (InFileName, &InFileHandle, EFI_FILE_MODE_READ, 0);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"efidecompress", ShellCommandLineGetRawValue (Package, 1));
            ShellStatus = SHELL_NOT_FOUND;
          }
        }

        if (ShellStatus == SHELL_SUCCESS) {
          Status = FileHandleGetSize (InFileHandle, &Temp64Bit);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"efidecompress", ShellCommandLineGetRawValue (Package, 1));
            ShellStatus = SHELL_NOT_FOUND;
          }
        }

        if (ShellStatus == SHELL_SUCCESS) {
          //
          // Limit the File Size to UINT32, even though calls accept UINTN.
          // 32 bits = 4gb.
          //
          Status = SafeUint64ToUint32 (Temp64Bit, (UINT32 *)&InSize);
          if (EFI_ERROR (Status)) {
            ASSERT_EFI_ERROR (Status);
            ShellStatus = SHELL_BAD_BUFFER_SIZE;
            goto Done;
          }

          InBuffer = AllocateZeroPool (InSize);
          if (InBuffer == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
          } else {
            Status = gEfiShellProtocol->ReadFile (InFileHandle, &InSize, InBuffer);
            ASSERT_EFI_ERROR (Status);

            Status = gBS->LocateProtocol (&gEfiDecompressProtocolGuid, NULL, (VOID **)&Decompress);
            ASSERT_EFI_ERROR (Status);

            Status = Decompress->GetInfo (Decompress, InBuffer, (UINT32)InSize, &OutSize, &ScratchSize);
          }

          if (EFI_ERROR (Status) || (OutSize == 0)) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_DECOMPRESS_NOPE), gShellDebug1HiiHandle, InFileName);
            ShellStatus = SHELL_NOT_FOUND;
          } else {
            Status = ShellOpenFileByName (OutFileName, &OutFileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
            if (EFI_ERROR (Status)) {
              ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_OPEN_FAIL), gShellDebug1HiiHandle, ShellCommandLineGetRawValue (Package, 2), Status);
              ShellStatus = SHELL_NOT_FOUND;
            } else {
              OutBuffer     = AllocateZeroPool (OutSize);
              ScratchBuffer = AllocateZeroPool (ScratchSize);
              if ((OutBuffer == NULL) || (ScratchBuffer == NULL)) {
                Status = EFI_OUT_OF_RESOURCES;
              } else {
                Status = Decompress->Decompress (Decompress, InBuffer, (UINT32)InSize, OutBuffer, OutSize, ScratchBuffer, ScratchSize);
              }
            }
          }

          if (EFI_ERROR (Status)) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_EFI_DECOMPRESS_FAIL), gShellDebug1HiiHandle, Status);
            ShellStatus = ((Status == EFI_OUT_OF_RESOURCES) ? SHELL_OUT_OF_RESOURCES : SHELL_DEVICE_ERROR);
          } else {
            OutSizeTemp = OutSize;
            Status      = gEfiShellProtocol->WriteFile (OutFileHandle, &OutSizeTemp, OutBuffer);
            OutSize     = (UINT32)OutSizeTemp;
            if (EFI_ERROR (Status)) {
              ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_WRITE_FAIL), gShellDebug1HiiHandle, L"efidecompress", OutFileName, Status);
              ShellStatus = SHELL_DEVICE_ERROR;
            }
          }
        }
      }
    }

Done:

    ShellCommandLineFreeVarList (Package);
  }

  if (InFileHandle != NULL) {
    gEfiShellProtocol->CloseFile (InFileHandle);
  }

  if (OutFileHandle != NULL) {
    gEfiShellProtocol->CloseFile (OutFileHandle);
  }

  SHELL_FREE_NON_NULL (InFileName);
  SHELL_FREE_NON_NULL (InBuffer);
  SHELL_FREE_NON_NULL (OutBuffer);
  SHELL_FREE_NON_NULL (ScratchBuffer);

  return (ShellStatus);
}
