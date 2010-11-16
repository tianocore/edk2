/** @file
  Main file for SerMode shell Debug1 function.

  Copyright (c) 20052010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Library/ShellLib.h>
#include <Protocol/SerialIo.h>

SHELL_STATUS
EFIAPI
iDisplaySettings (
  IN UINTN                   HandleIdx,
  IN BOOLEAN                 HandleValid
  )
{
  EFI_SERIAL_IO_PROTOCOL  *SerialIo;
  UINTN                   NoHandles;
  EFI_HANDLE              *Handles;
  EFI_STATUS              Status;
  UINTN                   Index;
  CHAR16                  *StopBits;
  CHAR16                  Parity;
  SHELL_STATUS            ShellStatus;

  Handles   = NULL;
  StopBits  = NULL;

  ShellStatus = SHELL_SUCCESS;

  Status    = gBS->LocateHandleBuffer (ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &NoHandles, &Handles);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_NO_FOUND), gShellDebug1HiiHandle);
    return SHELL_INVALID_PARAMETER;
  }

  for (Index = 0; Index < NoHandles; Index++) {
    if (HandleValid) {
      if (ConvertHandleIndexToHandle(HandleIdx) != Handles[Index]) {
        continue;
      }
    }

    Status = gBS->HandleProtocol (Handles[Index], &gEfiSerialIoProtocolGuid, (VOID**)&SerialIo);
    if (!EFI_ERROR (Status)) {
      switch (SerialIo->Mode->Parity) {
      case DefaultParity:

        Parity = 'D';
        break;

      case NoParity:

        Parity = 'N';
        break;

      case EvenParity:

        Parity = 'E';
        break;

      case OddParity:

        Parity = 'O';
        break;

      case MarkParity:

        Parity = 'M';
        break;

      case SpaceParity:

        Parity = 'S';
        break;

      default:

        Parity = 'U';
      }

      switch (SerialIo->Mode->StopBits) {
      case DefaultStopBits:

        StopBits = L"Default";
        break;

      case OneStopBit:

        StopBits = L"1";
        break;

      case TwoStopBits:

        StopBits = L"2";
        break;

      case OneFiveStopBits:

        StopBits = L"1.5";
        break;

      default:

        StopBits = L"Unknown";
      }
      ShellPrintHiiEx(
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_SERMODE_DISPLAY),
        gShellDebug1HiiHandle,
        ConvertHandleToHandleIndex (Handles[Index]),
        Handles[Index],
        SerialIo->Mode->BaudRate,
        Parity,
        SerialIo->Mode->DataBits,
        StopBits
       );
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_NO_FOUND), gShellDebug1HiiHandle);
      ShellStatus = SHELL_NOT_FOUND;
      break;
    }

    if (HandleValid) {
      break;
    }
  }

  if (Index == NoHandles) {
    if ((NoHandles != 0 && HandleValid) || 0 == NoHandles) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_NOT_FOUND), gShellDebug1HiiHandle);
      ShellStatus = SHELL_NOT_FOUND;
    }
  }

  return ShellStatus;
}

SHELL_STATUS
EFIAPI
ShellCommandRunSerMode (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  SHELL_STATUS            ShellStatus;
  UINTN                   Index;
  UINTN                   NoHandles;
  EFI_HANDLE              *Handles;
  EFI_PARITY_TYPE         Parity;
  EFI_STOP_BITS_TYPE      StopBits;
  UINTN                   HandleIdx;
  UINTN                   BaudRate;
  UINTN                   DataBits;
  UINTN                   Value;
  EFI_SERIAL_IO_PROTOCOL  *SerialIo;
  LIST_ENTRY              *Package;
  CHAR16                  *ProblemParam;
  CONST CHAR16            *Temp;

  ShellStatus = SHELL_SUCCESS;
  HandleIdx   = 0;
  Parity      = DefaultParity;
  Handles     = NULL;
  NoHandles   = 0;
  Index       = 0;
  Package     = NULL;

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
    if (ShellCommandLineGetCount(Package) < 5 && ShellCommandLineGetCount(Package) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) > 5) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Temp = ShellCommandLineGetRawValue(Package, 1);
      if (Temp != NULL) {
        HandleIdx = StrHexToUintn(Temp);
        Temp = ShellCommandLineGetRawValue(Package, 2);
        if (Temp == NULL) {
          ShellStatus = iDisplaySettings (HandleIdx, TRUE);
          goto Done;
        }
      } else {
        ShellStatus = iDisplaySettings (0, FALSE);
        goto Done;
      }
      Temp = ShellCommandLineGetRawValue(Package, 2);
      if (Temp != NULL) {
        BaudRate = StrHexToUintn(Temp);
      } else {
        ASSERT(FALSE);
        BaudRate = 0;
      }
      Temp = ShellCommandLineGetRawValue(Package, 3);
      if (Temp == NULL || StrLen(Temp)>1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        switch(Temp[0]){
        case 'd':
        case 'D':
          Parity = DefaultParity;
          break;
        case 'n':
        case 'N':
          Parity = NoParity;
          break;
        case 'e':
        case 'E':
          Parity = EvenParity;
          break;
        case 'o':
        case 'O':
          Parity = OddParity;
          break;
        case 'm':
        case 'M':
          Parity = MarkParity;
          break;
        case 's':
        case 'S':
          Parity = SpaceParity;
          break;
        default:
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp);
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      }
      Temp = ShellCommandLineGetRawValue(Package, 4);
      if (Temp != NULL) {
        DataBits = StrHexToUintn(Temp);
      } else {
        //
        // make sure this is some number not in the list below.
        //
        DataBits = 0;
      }
      switch (DataBits) {
      case 4:
      case 7:
      case 8:
        break;
      default:
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
      Temp = ShellCommandLineGetRawValue(Package, 5);
      Value = StrHexToUintn(Temp);
      switch (Value) {
      case 0:
        StopBits = DefaultStopBits;
        break;

      case 1:
        StopBits = OneStopBit;
        break;

      case 2:
        StopBits = TwoStopBits;
        break;

      case 15:
        StopBits = OneFiveStopBits;
        break;

      default:
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
      Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSerialIoProtocolGuid, NULL, &NoHandles, &Handles);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_NO_FOUND), gShellDebug1HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      for (Index = 0; Index < NoHandles; Index++) {
        if (ConvertHandleIndexToHandle (HandleIdx) != Handles[Index]) {
          continue;
        }

        Status = gBS->HandleProtocol (Handles[Index], &gEfiSerialIoProtocolGuid, (VOID**)&SerialIo);
        if (!EFI_ERROR (Status)) {
          Status = SerialIo->SetAttributes (
                              SerialIo,
                              (UINT64) BaudRate,
                              SerialIo->Mode->ReceiveFifoDepth,
                              SerialIo->Mode->Timeout,
                              Parity,
                              (UINT8) DataBits,
                              StopBits
                             );
          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_SET_FAIL), gShellDebug1HiiHandle, ConvertHandleToHandleIndex(Handles[Index]), Status);
            ShellStatus = SHELL_ACCESS_DENIED;
          } else {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_SET_HANDLE), gShellDebug1HiiHandle, ConvertHandleToHandleIndex(Handles[Index]));
          }
          break;
        }
      }
    }
  }

  if (Index == NoHandles) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SERMODE_BAD_HANDLE), gShellDebug1HiiHandle, HandleIdx);
    ShellStatus = SHELL_INVALID_PARAMETER;
  }

Done:
  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  if (Handles != NULL) {
    FreePool (Handles);
  }
  return ShellStatus;
}
