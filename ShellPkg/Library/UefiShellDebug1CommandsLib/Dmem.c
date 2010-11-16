/** @file
  Main file for Dmem shell Debug1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/PciRootBridgeIo.h>

CHAR16
MakePrintable(
  IN CONST CHAR16 Char
  )
{
  if ((Char < 0x20 && Char > 0)||(Char > 126)) {
    return (L'?');
  }
  return (Char);
}

SHELL_STATUS
EFIAPI
DisplayMmioMemory(
  IN CONST VOID   *Address,
  IN CONST UINTN  Size
  )
{
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRbIo;
  EFI_STATUS                      Status;
  VOID                            *Buffer;
  SHELL_STATUS                    ShellStatus;

  ShellStatus = SHELL_SUCCESS;

  Status = gBS->LocateProtocol(&gEfiPciRootBridgeIoProtocolGuid, NULL, (VOID**)&PciRbIo);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle);
    return (SHELL_NOT_FOUND);
  }
  Buffer = AllocateZeroPool(Size);
  ASSERT(Buffer != NULL);

  Status = PciRbIo->Mem.Read(PciRbIo, EfiPciWidthUint8, (UINT64)Address, Size, Buffer);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_ER), gShellDebug1HiiHandle, Status);
    ShellStatus = SHELL_NOT_FOUND;
  } else {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMEM_MMIO_HEADER_ROW), gShellDebug1HiiHandle, (UINT64)Address, Size);
    DumpHex(2,0,Size,Buffer);
  }

  FreePool(Buffer);
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-mmio", TypeFlag},
  {NULL, TypeMax}
  };

SHELL_STATUS
EFIAPI
ShellCommandRunDmem (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  VOID                *Address;
  UINTN               Size;
  CONST CHAR16        *Temp1;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  Address             = NULL;
  Size                = 0;

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
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    Temp1 = ShellCommandLineGetRawValue(Package, 1);
    if (Temp1 == NULL) {
      Address = gST;
      Size = 512;
    } else {
      if (!ShellIsHexOrDecimalNumber(Temp1, TRUE, FALSE)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp1);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Address = (VOID*)StrHexToUintn(Temp1);
      }
      Temp1 = ShellCommandLineGetRawValue(Package, 2);
      if (Temp1 == NULL) {
        Size = 512;
      } else {
        if (!ShellIsHexOrDecimalNumber(Temp1, FALSE, FALSE)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Size = ShellStrToUintn(Temp1);
        }
      }
    }

    if (ShellStatus == SHELL_SUCCESS) {
      if (!ShellCommandLineGetFlag(Package, L"-mmio")) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMEM_HEADER_ROW), gShellDebug1HiiHandle, (UINT64)Address, Size);
        DumpHex(2,0,Size,Address);
      } else {
        ShellStatus = DisplayMmioMemory(Address, Size);
      }
    }


    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
