/** @file
  Main file for Dblk shell Debug1 function.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/BlockIo.h>

SHELL_STATUS
EFIAPI
DisplayTheBlocks(
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevPath,
  IN CONST UINT64                   Lba,
  IN CONST UINT8                    BlockCount
  )
{
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL  *Copy;
  EFI_HANDLE                BlockIoHandle;
  EFI_STATUS                Status;
  SHELL_STATUS              ShellStatus;
  UINT8                     *Buffer;
  UINTN                     BufferSize;

  ShellStatus = SHELL_SUCCESS;
  Copy = (EFI_DEVICE_PATH_PROTOCOL *)DevPath;

  Status = gBS->LocateDevicePath(&gEfiBlockIoProtocolGuid, &Copy, &BlockIoHandle);
  ASSERT_EFI_ERROR(Status);

  Status = gBS->OpenProtocol(BlockIoHandle, &gEfiBlockIoProtocolGuid, (VOID**)&BlockIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  ASSERT_EFI_ERROR(Status);

  BufferSize = BlockIo->Media->BlockSize * BlockCount;
  if (BufferSize > 0) {
    Buffer     = AllocatePool(BufferSize);
  } else {
    Buffer    = NULL;
  }

  Status = BlockIo->ReadBlocks(BlockIo, BlockIo->Media->MediaId, Lba, BufferSize, Buffer);
  if (!EFI_ERROR(Status) && Buffer != NULL) {
    DumpHex(2,0,BufferSize,Buffer);
  } else {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_READ_FAIL), gShellDebug1HiiHandle, L"BlockIo", Status);
    ShellStatus = SHELL_DEVICE_ERROR;
  }

  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  gBS->CloseProtocol(BlockIoHandle, &gEfiBlockIoProtocolGuid, gImageHandle, NULL);
  return (ShellStatus);
}

SHELL_STATUS
EFIAPI
ShellCommandRunDblk (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST CHAR16        *BlockName;
  CONST CHAR16        *LbaString;
  CONST CHAR16        *BlockCountString;
  UINT64              Lba;
  UINT8               BlockCount;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;

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
    if (ShellCommandLineGetCount(Package) > 4) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) < 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Parse the params
      //
      BlockName         = ShellCommandLineGetRawValue(Package, 1);
      LbaString         = ShellCommandLineGetRawValue(Package, 2);
      BlockCountString  = ShellCommandLineGetRawValue(Package, 3);

      if (LbaString == NULL) {
        Lba = 0;
      } else {
        Lba = (UINT64)StrHexToUintn(LbaString);
      }

      if (BlockCountString == NULL) {
        BlockCount = 1;
      } else {
        BlockCount = (UINT8)StrHexToUintn(BlockCountString);
        if (BlockCount > 0x10) {
          BlockCount = 0x10;
        }
      }

      //
      // do the work if we have a valid block identifier
      //
      if (mEfiShellProtocol->GetDevicePathFromMap(BlockName) == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, BlockName);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        ShellStatus = DisplayTheBlocks(mEfiShellProtocol->GetDevicePathFromMap(BlockName), Lba, BlockCount);
      }
    }

    ShellCommandLineFreeVarList (Package);
  }
  return (ShellStatus);
}
