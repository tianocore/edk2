/** @file
  Main file for Dmem shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Protocol/PciRootBridgeIo.h>
#include <Guid/Acpi.h>
#include <Guid/Mps.h>
#include <Guid/SmBios.h>
#include <Guid/SalSystemTable.h>

/**
  Make a printable character.

  If Char is printable then return it, otherwise return a question mark.

  @param[in] Char     The character to make printable.

  @return A printable character representing Char.
**/
CHAR16
EFIAPI
MakePrintable(
  IN CONST CHAR16 Char
  )
{
  if ((Char < 0x20 && Char > 0)||(Char > 126)) {
    return (L'?');
  }
  return (Char);
}

/**
  Display some Memory-Mapped-IO memory.

  @param[in] Address    The starting address to display.
  @param[in] Size       The length of memory to display.
**/
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
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle, L"dmem");  
    return (SHELL_NOT_FOUND);
  }
  Buffer = AllocateZeroPool(Size);
  ASSERT(Buffer != NULL);

  Status = PciRbIo->Mem.Read(PciRbIo, EfiPciWidthUint8, (UINT64)(UINTN)Address, Size, Buffer);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_ER), gShellDebug1HiiHandle, L"dmem");  
    ShellStatus = SHELL_NOT_FOUND;
  } else {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMEM_MMIO_HEADER_ROW), gShellDebug1HiiHandle, (UINT64)(UINTN)Address, Size);
    DumpHex(2, (UINTN)Address, Size, Buffer);
  }

  FreePool(Buffer);
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-mmio", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'dmem' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
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
  UINT64              Size;
  CONST CHAR16        *Temp1;
  UINT64              AcpiTableAddress;
  UINT64              Acpi20TableAddress;
  UINT64              SalTableAddress;
  UINT64              SmbiosTableAddress;
  UINT64              MpsTableAddress;
  UINTN               TableWalker;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"dmem", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"dmem");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Temp1 = ShellCommandLineGetRawValue(Package, 1);
      if (Temp1 == NULL) {
        Address = gST;
        Size = 512;
      } else {
        if (!ShellIsHexOrDecimalNumber(Temp1, TRUE, FALSE) || EFI_ERROR(ShellConvertStringToUint64(Temp1, (UINT64*)&Address, TRUE, FALSE))) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dmem", Temp1);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } 
        Temp1 = ShellCommandLineGetRawValue(Package, 2);
        if (Temp1 == NULL) {
          Size = 512;
        } else {
          if (!ShellIsHexOrDecimalNumber(Temp1, FALSE, FALSE) || EFI_ERROR(ShellConvertStringToUint64(Temp1, &Size, TRUE, FALSE))) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"dmem", Temp1);  
            ShellStatus = SHELL_INVALID_PARAMETER;
          }
        }
      }
    }

    if (ShellStatus == SHELL_SUCCESS) {
      if (!ShellCommandLineGetFlag(Package, L"-mmio")) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMEM_HEADER_ROW), gShellDebug1HiiHandle, (UINT64)(UINTN)Address, Size);
        DumpHex(2, (UINTN)Address, (UINTN)Size, Address);
        if (Address == (VOID*)gST) {
          Acpi20TableAddress  = 0;
          AcpiTableAddress    = 0;
          SalTableAddress     = 0;
          SmbiosTableAddress  = 0;
          MpsTableAddress     = 0;
          for (TableWalker = 0 ; TableWalker < gST->NumberOfTableEntries ; TableWalker++) {
            if (CompareGuid(&gST->ConfigurationTable[TableWalker].VendorGuid, &gEfiAcpi20TableGuid)) {
              Acpi20TableAddress = (UINT64)(UINTN)gST->ConfigurationTable[TableWalker].VendorTable;
              continue;
            }
            if (CompareGuid(&gST->ConfigurationTable[TableWalker].VendorGuid, &gEfiAcpi10TableGuid)) {
              AcpiTableAddress = (UINT64)(UINTN)gST->ConfigurationTable[TableWalker].VendorTable;
              continue;
            }
            if (CompareGuid(&gST->ConfigurationTable[TableWalker].VendorGuid, &gEfiSalSystemTableGuid)) {
              SalTableAddress = (UINT64)(UINTN)gST->ConfigurationTable[TableWalker].VendorTable;
              continue;
            }
            if (CompareGuid(&gST->ConfigurationTable[TableWalker].VendorGuid, &gEfiSmbiosTableGuid)) {
              SmbiosTableAddress = (UINT64)(UINTN)gST->ConfigurationTable[TableWalker].VendorTable;
              continue;
            }
            if (CompareGuid(&gST->ConfigurationTable[TableWalker].VendorGuid, &gEfiMpsTableGuid)) {
              MpsTableAddress = (UINT64)(UINTN)gST->ConfigurationTable[TableWalker].VendorTable;
              continue;
            }
          }

          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_DMEM_SYSTEM_TABLE), gShellDebug1HiiHandle, 
            (UINT64)(UINTN)Address,
            gST->Hdr.HeaderSize,
            gST->Hdr.Revision,
            (UINT64)(UINTN)gST->ConIn,
            (UINT64)(UINTN)gST->ConOut,
            (UINT64)(UINTN)gST->StdErr,
            (UINT64)(UINTN)gST->RuntimeServices,
            (UINT64)(UINTN)gST->BootServices,
            SalTableAddress,
            AcpiTableAddress,
            Acpi20TableAddress,
            MpsTableAddress,
            SmbiosTableAddress
            );
        }
      } else {
        ShellStatus = DisplayMmioMemory(Address, (UINTN)Size);
      }
    }


    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
