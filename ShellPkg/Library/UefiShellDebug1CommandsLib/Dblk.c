/** @file
  Main file for Dblk shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/BlockIo.h>

/**
  Display blocks to the screen.

  @param[in] DevPath      The device path to get the blocks from.
  @param[in] Lba          The Lba number to start from.
  @param[in] BlockCount   How many blocks to display.

  @retval SHELL_SUCCESS   The display was successful.
**/
SHELL_STATUS
DisplayTheBlocks (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevPath,
  IN CONST UINT64                    Lba,
  IN CONST UINT8                     BlockCount
  )
{
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_HANDLE             BlockIoHandle;
  EFI_STATUS             Status;
  SHELL_STATUS           ShellStatus;
  UINT8                  *Buffer;
  UINT8                  *OriginalBuffer;
  UINTN                  BufferSize;

  ShellStatus = SHELL_SUCCESS;

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, (EFI_DEVICE_PATH_PROTOCOL **)&DevPath, &BlockIoHandle);
  if (EFI_ERROR (Status)) {
    return (SHELL_NOT_FOUND);
  }

  Status = gBS->OpenProtocol (BlockIoHandle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
    return (SHELL_NOT_FOUND);
  }

  BufferSize = BlockIo->Media->BlockSize * BlockCount;
  if (BlockIo->Media->IoAlign == 0) {
    BlockIo->Media->IoAlign = 1;
  }

  if (BufferSize > 0) {
    OriginalBuffer = AllocateZeroPool (BufferSize + BlockIo->Media->IoAlign);
    Buffer         = ALIGN_POINTER (OriginalBuffer, BlockIo->Media->IoAlign);
  } else {
    ShellPrintDefaultEx (L"  BlockSize: 0x%08x, BlockCount: 0x%08x\r\n", BlockIo->Media->BlockSize, BlockCount);
    OriginalBuffer = NULL;
    Buffer         = NULL;
  }

  Status = BlockIo->ReadBlocks (BlockIo, BlockIo->Media->MediaId, Lba, BufferSize, Buffer);
  if (!EFI_ERROR (Status) && (Buffer != NULL)) {
    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_DBLK_HEADER),
      gShellDebug1HiiHandle,
      Lba,
      BufferSize,
      BlockIo
      );

    DumpHex (2, 0, BufferSize, Buffer);
  } else {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_FILE_READ_FAIL), gShellDebug1HiiHandle, L"dblk", L"BlockIo");
    ShellStatus = SHELL_DEVICE_ERROR;
  }

  if (OriginalBuffer != NULL) {
    FreePool (OriginalBuffer);
  }

  gBS->CloseProtocol (BlockIoHandle, &gEfiBlockIoProtocolGuid, gImageHandle, NULL);
  return (ShellStatus);
}

/**
  Function for 'dblk' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDblk (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;
  LIST_ENTRY                *Package;
  CHAR16                    *ProblemParam;
  SHELL_STATUS              ShellStatus;
  CONST CHAR16              *BlockName;
  CONST CHAR16              *LbaString;
  CONST CHAR16              *BlockCountString;
  UINT64                    Lba;
  UINT64                    BlockCount;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;

  Lba         = 0;
  BlockCount  = 0;
  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;

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
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"dblk", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 4) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"dblk");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) < 2) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"dblk");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // Parse the params
      //
      BlockName        = ShellCommandLineGetRawValue (Package, 1);
      LbaString        = ShellCommandLineGetRawValue (Package, 2);
      BlockCountString = ShellCommandLineGetRawValue (Package, 3);

      if (LbaString == NULL) {
        Lba = 0;
      } else {
        if (!ShellIsHexOrDecimalNumber (LbaString, TRUE, FALSE)) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dblk", LbaString);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }

        if (EFI_ERROR (ShellConvertStringToUint64 (LbaString, &Lba, TRUE, FALSE))) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dblk", LbaString);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }

      if (BlockCountString == NULL) {
        BlockCount = 1;
      } else {
        if (!ShellIsHexOrDecimalNumber (BlockCountString, TRUE, FALSE)) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dblk", BlockCountString);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }

        if (!EFI_ERROR (ShellConvertStringToUint64 (BlockCountString, &BlockCount, TRUE, FALSE))) {
          if (BlockCount > 0x10) {
            BlockCount = 0x10;
          } else if (BlockCount == 0) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dblk", BlockCountString);
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        }
      }

      if (ShellStatus == SHELL_SUCCESS) {
        //
        // do the work if we have a valid block identifier
        //
        if ((BlockName == NULL) || (gEfiShellProtocol->GetDevicePathFromMap (BlockName) == NULL)) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dblk", BlockName);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          DevPath = (EFI_DEVICE_PATH_PROTOCOL *)gEfiShellProtocol->GetDevicePathFromMap (BlockName);
          if (gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &DevPath, NULL) == EFI_NOT_FOUND) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_MAP_PROTOCOL), gShellDebug1HiiHandle, L"dblk", BlockName, L"BlockIo");
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            ShellStatus = DisplayTheBlocks (gEfiShellProtocol->GetDevicePathFromMap (BlockName), Lba, (UINT8)BlockCount);
          }
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
