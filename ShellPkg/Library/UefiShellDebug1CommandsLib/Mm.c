/** @file
  Main file for Mm shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Library/ShellLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/DeviceIo.h>

typedef enum {
  EfiMemory,
  EFIMemoryMappedIo,
  EfiIo,
  EfiPciConfig,
  EfiPciEConfig
} EFI_ACCESS_TYPE;

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-mmio", TypeFlag},
  {L"-mem", TypeFlag},
  {L"-io", TypeFlag},
  {L"-pci", TypeFlag},
  {L"-pcie", TypeFlag},
  {L"-n", TypeFlag},
  {L"-w", TypeValue},
  {NULL, TypeMax}
  };

STATIC CONST UINT64 MaxNum[9]      = { 0xff, 0xffff, 0xffffffff, 0xffffffffffffffffULL };

/**
  Read some data into a buffer from memory.

  @param[in] Width    The width of each read.
  @param[in] Addresss The memory location to start reading at.
  @param[in] Size     The size of Buffer in Width sized units.
  @param[out] Buffer  The buffer to read into.
**/
VOID
EFIAPI
ReadMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64        Address,
  IN  UINTN         Size,
  OUT VOID          *Buffer
  )
{
  //
  // This function is defective.  This ASSERT prevents the defect from affecting anything.
  //
  ASSERT(Size == 1);
  do {
    if (Width == EfiPciWidthUint8) {
      *(UINT8 *) Buffer = *(UINT8 *) (UINTN) Address;
      Address -= 1;
    } else if (Width == EfiPciWidthUint16) {
      *(UINT16 *) Buffer = *(UINT16 *) (UINTN) Address;
      Address -= 2;
    } else if (Width == EfiPciWidthUint32) {
      *(UINT32 *) Buffer = *(UINT32 *) (UINTN) Address;
      Address -= 4;
    } else if (Width == EfiPciWidthUint64) {
      *(UINT64 *) Buffer = *(UINT64 *) (UINTN) Address;
      Address -= 8;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_READ_ERROR), gShellDebug1HiiHandle, L"mm");  
      break;
    }
    Size--;
  } while (Size > 0);
}

/**
  Write some data to memory.

  @param[in] Width    The width of each write.
  @param[in] Addresss The memory location to start writing at.
  @param[in] Size     The size of Buffer in Width sized units.
  @param[in] Buffer   The buffer to write from.
**/
VOID
EFIAPI
WriteMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64        Address,
  IN  UINTN         Size,
  IN  VOID          *Buffer
  )
{
  //
  // This function is defective.  This ASSERT prevents the defect from affecting anything.
  //
  ASSERT(Size == 1);
  do {
    if (Width == EfiPciWidthUint8) {
      *(UINT8 *) (UINTN) Address = *(UINT8 *) Buffer;
      Address += 1;
    } else if (Width == EfiPciWidthUint16) {
      *(UINT16 *) (UINTN) Address = *(UINT16 *) Buffer;
      Address += 2;
    } else if (Width == EfiPciWidthUint32) {
      *(UINT32 *) (UINTN) Address = *(UINT32 *) Buffer;
      Address += 4;
    } else if (Width == EfiPciWidthUint64) {
      *(UINT64 *) (UINTN) Address = *(UINT64 *) Buffer;
      Address += 8;
    } else {
      ASSERT (FALSE);
    }
    //
    //
    //
    Size--;
  } while (Size > 0);
}

/**
  Convert a string to it's hex data.

  @param[in] str    The pointer to the string of hex data.
  @param[out] data  The pointer to the buffer to fill.  Valid upon a TRUE return.

  @retval TRUE      The conversion was successful.
  @retval FALSE     The conversion failed.
**/
BOOLEAN
EFIAPI
GetHex (
  IN  UINT16  *str,
  OUT UINT64  *data
  )
{
  UINTN   TempUint;
  CHAR16  TempChar;
  BOOLEAN Find;

  Find = FALSE;
  //
  // convert hex digits
  //
  TempUint = 0;
  TempChar = *(str++);
  while (TempChar != CHAR_NULL) {
    if (TempChar >= 'a' && TempChar <= 'f') {
      TempChar -= 'a' - 'A';
    }

    if (TempChar == ' ') {
      break;
    }

    if ((TempChar >= '0' && TempChar <= '9') || (TempChar >= 'A' && TempChar <= 'F')) {
      TempUint     = (TempUint << 4) | (TempChar - (TempChar >= 'A' ? 'A' - 10 : '0'));

      Find  = TRUE;
    } else {
      return FALSE;
    }

    TempChar = *(str++);
  }

  *data = TempUint;
  return Find;
}

/**
  Get the PCI-E Address from a PCI address format 0x0000ssbbddffrrr
  where ss is SEGMENT, bb is BUS, dd is DEVICE, ff is FUNCTION
  and rrr is REGISTER (extension format for PCI-E).

  @param[in] InputAddress       PCI address format on input.
  @param[out]PciEAddress        PCI-E address extention format.
**/
VOID
EFIAPI
GetPciEAddressFromInputAddress (
  IN UINT64                 InputAddress,
  OUT UINT64                *PciEAddress
  )
{
  *PciEAddress = RShiftU64(InputAddress & ~(UINT64) 0xFFF, 4);
  *PciEAddress += LShiftU64((UINT16) InputAddress & 0x0FFF, 32);
}

/**
  Function for 'mm' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *IoDev;
  UINT64                          Address;
  UINT64                          PciEAddress;
  UINT64                          Value;
  UINT32                          SegmentNumber;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH                    Width;
  EFI_ACCESS_TYPE                 AccessType;
  UINT64                          Buffer;
  UINTN                           Index;
  UINTN                           Size;
//  CHAR16                          *ValueStr;
  BOOLEAN                         Complete;
  CHAR16                          *InputStr;
  BOOLEAN                         Interactive;
  EFI_HANDLE                      *HandleBuffer;
  UINTN                           BufferSize;
  UINTN                           ItemValue;
  LIST_ENTRY                      *Package;
  CHAR16                          *ProblemParam;
  SHELL_STATUS                    ShellStatus;
  CONST CHAR16                    *Temp;

  Value         = 0;
  Address       = 0;
  PciEAddress   = 0;
  IoDev         = NULL;
  HandleBuffer  = NULL;
  BufferSize    = 0;
  SegmentNumber = 0;
  ShellStatus   = SHELL_SUCCESS;
  InputStr      = NULL;

  //
  // Parse arguments
  //
  Width       = EfiPciWidthUint8;
  Size        = 1;
  AccessType  = EfiMemory;
//  ValueStr    = NULL;
  Interactive = TRUE;
  Package     = NULL;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"mm", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) < 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"mm");  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else if (ShellCommandLineGetFlag(Package, L"-w") && ShellCommandLineGetValue(Package, L"-w") == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"mm", L"-w"); 
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      if (ShellCommandLineGetFlag(Package, L"-mmio")) {
        AccessType = EFIMemoryMappedIo;
        if (ShellCommandLineGetFlag(Package, L"-mem")
          ||ShellCommandLineGetFlag(Package, L"-io")
          ||ShellCommandLineGetFlag(Package, L"-pci")
          ||ShellCommandLineGetFlag(Package, L"-pcie")
        ){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");  
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag(Package, L"-mem")) {
        AccessType = EfiMemory;
        if (ShellCommandLineGetFlag(Package, L"-io")
          ||ShellCommandLineGetFlag(Package, L"-pci")
          ||ShellCommandLineGetFlag(Package, L"-pcie")
        ){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");  
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag(Package, L"-io")) {
        AccessType = EfiIo;
        if (ShellCommandLineGetFlag(Package, L"-pci")
          ||ShellCommandLineGetFlag(Package, L"-pcie")
        ){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");  
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag(Package, L"-pci")) {
        AccessType = EfiPciConfig;
        if (ShellCommandLineGetFlag(Package, L"-pcie")
        ){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");  
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag(Package, L"-pcie")) {
        AccessType = EfiPciEConfig;
      }
    }

    //
    // Non interactive for a script file or for the specific parameter
    //
    if (gEfiShellProtocol->BatchIsActive() || ShellCommandLineGetFlag (Package, L"-n")) {
      Interactive = FALSE;
    }

    Temp = ShellCommandLineGetValue(Package, L"-w");
    if (Temp != NULL) {
      ItemValue = ShellStrToUintn (Temp);

      switch (ItemValue) {
      case 1:
        Width = EfiPciWidthUint8;
        Size  = 1;
        break;

      case 2:
        Width = EfiPciWidthUint16;
        Size  = 2;
        break;

      case 4:
        Width = EfiPciWidthUint32;
        Size  = 4;
        break;

      case 8:
        Width = EfiPciWidthUint64;
        Size  = 8;
        break;

      default:
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellDebug1HiiHandle, L"mm", Temp, L"-w");  
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    Temp = ShellCommandLineGetRawValue(Package, 1);
    if (!ShellIsHexOrDecimalNumber(Temp, TRUE, FALSE) || EFI_ERROR(ShellConvertStringToUint64(Temp, (UINT64*)&Address, TRUE, FALSE))) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mm", Temp);  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    Temp = ShellCommandLineGetRawValue(Package, 2);
    if (Temp != NULL) {
      //
      // Per spec if value is specified, then -n is assumed.
      //
      Interactive = FALSE;

      if (!ShellIsHexOrDecimalNumber(Temp, TRUE, FALSE) || EFI_ERROR(ShellConvertStringToUint64(Temp, &Value, TRUE, FALSE))) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mm", Temp);  
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
      switch (Size) {
      case 1:
        if (Value > 0xFF) {
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        break;

      case 2:
        if (Value > 0xFFFF) {
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        break;

      case 4:
        if (Value > 0xFFFFFFFF) {
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
        break;

      default:
        break;
      }

      if (ShellStatus != SHELL_SUCCESS) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mm", Temp);  
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    if ((Address & (Size - 1)) != 0) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_NOT_ALIGNED), gShellDebug1HiiHandle, L"mm", Address);  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }
    //
    // locate DeviceIO protocol interface
    //
    if (AccessType != EfiMemory) {
      Status = gBS->LocateHandleBuffer (
                 ByProtocol,
                 &gEfiPciRootBridgeIoProtocolGuid,
                 NULL,
                 &BufferSize,
                 &HandleBuffer
                );
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle, L"mm");  
        ShellStatus = SHELL_NOT_FOUND;
        goto Done;
      }
      //
      // In the case of PCI or PCIE
      // Get segment number and mask the segment bits in Address
      //
      if (AccessType == EfiPciEConfig) {
        SegmentNumber = (UINT32) RShiftU64 (Address, 36) & 0xff;
        Address      &= 0xfffffffffULL;
      } else {
        if (AccessType == EfiPciConfig) {
          SegmentNumber = (UINT32) RShiftU64 (Address, 32) & 0xff;
          Address      &= 0xffffffff;
        }
      }
      //
      // Find the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL of the specified segment number
      //
      for (Index = 0; Index < BufferSize; Index++) {
        Status = gBS->HandleProtocol (
                       HandleBuffer[Index],
                       &gEfiPciRootBridgeIoProtocolGuid,
                       (VOID *) &IoDev
                      );
        if (EFI_ERROR (Status)) {
          continue;
        }
        if (IoDev->SegmentNumber != SegmentNumber) {
          IoDev = NULL;
        }
      }
      if (IoDev == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_SEGMENT_NOT_FOUND), gShellDebug1HiiHandle, L"mm", SegmentNumber);  
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    if (AccessType == EfiIo && Address + Size > 0x10000) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS_RANGE), gShellDebug1HiiHandle, L"mm");  
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (AccessType == EfiPciEConfig) {
      GetPciEAddressFromInputAddress (Address, &PciEAddress);
    }

    //
    // Set value
    //
    if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
      if (AccessType == EFIMemoryMappedIo) {
        IoDev->Mem.Write (IoDev, Width, Address, 1, &Value);
      } else if (AccessType == EfiIo) {
        IoDev->Io.Write (IoDev, Width, Address, 1, &Value);
      } else if (AccessType == EfiPciConfig) {
        IoDev->Pci.Write (IoDev, Width, Address, 1, &Value);
      } else if (AccessType == EfiPciEConfig) {
        IoDev->Pci.Write (IoDev, Width, PciEAddress, 1, &Value);
      } else {
        WriteMem (Width, Address, 1, &Value);
      }

      ASSERT(ShellStatus == SHELL_SUCCESS);
      goto Done;
    }


    //
    // non-interactive mode
    //
    if (!Interactive) {
      Buffer = 0;
      if (AccessType == EFIMemoryMappedIo) {
        if (!gEfiShellProtocol->BatchIsActive()) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_MMIO), gShellDebug1HiiHandle);
        }
        IoDev->Mem.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiIo) {
        if (!gEfiShellProtocol->BatchIsActive()) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_IO), gShellDebug1HiiHandle);
        }
        IoDev->Io.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiPciConfig) {
        if (!gEfiShellProtocol->BatchIsActive()) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_PCI), gShellDebug1HiiHandle);
        }
        IoDev->Pci.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiPciEConfig) {
        if (!gEfiShellProtocol->BatchIsActive()) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_PCIE), gShellDebug1HiiHandle);
        }
        IoDev->Pci.Read (IoDev, Width, PciEAddress, 1, &Buffer);
      } else {
        if (!gEfiShellProtocol->BatchIsActive()) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_MEM), gShellDebug1HiiHandle);
        }
        ReadMem (Width, Address, 1, &Buffer);
      }
      if (!gEfiShellProtocol->BatchIsActive()) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS), gShellDebug1HiiHandle, Address);
      }
      if (Size == 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF2), gShellDebug1HiiHandle, (UINTN)Buffer);
      } else if (Size == 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF4), gShellDebug1HiiHandle, (UINTN)Buffer);
      } else if (Size == 4) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF8), gShellDebug1HiiHandle, (UINTN)Buffer);
      } else if (Size == 8) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF16), gShellDebug1HiiHandle, Buffer);
      }

      ShellPrintEx(-1, -1, L"\r\n");

      ASSERT(ShellStatus == SHELL_SUCCESS);
      goto Done;
    }
    //
    // interactive mode
    //
    Complete = FALSE;
    do {
      if (AccessType == EfiIo && Address + Size > 0x10000) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS_RANGE2), gShellDebug1HiiHandle, L"mm");  
        break;
      }

      Buffer = 0;
      if (AccessType == EFIMemoryMappedIo) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_MMIO), gShellDebug1HiiHandle);
        IoDev->Mem.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiIo) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_IO), gShellDebug1HiiHandle);
        IoDev->Io.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiPciConfig) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_PCI), gShellDebug1HiiHandle);
        IoDev->Pci.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiPciEConfig) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_PCIE), gShellDebug1HiiHandle);
        IoDev->Pci.Read (IoDev, Width, PciEAddress, 1, &Buffer);
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_MEM), gShellDebug1HiiHandle);
        ReadMem (Width, Address, 1, &Buffer);
      }

      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS), gShellDebug1HiiHandle, Address);

      if (Size == 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF2), gShellDebug1HiiHandle, (UINTN)Buffer);
      } else if (Size == 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF4), gShellDebug1HiiHandle, (UINTN)Buffer);
      } else if (Size == 4) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF8), gShellDebug1HiiHandle, (UINTN)Buffer);
      } else if (Size == 8) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF16), gShellDebug1HiiHandle, Buffer);
      }
      ShellPrintEx(-1, -1, L" > ");
      //
      // wait user input to modify
      //
      if (InputStr != NULL) {
        FreePool(InputStr);
        InputStr = NULL;
      }
      ShellPromptForResponse(ShellPromptResponseTypeFreeform, NULL, (VOID**)&InputStr);

      //
      // skip space characters
      //
      for (Index = 0; InputStr != NULL && InputStr[Index] == ' '; Index++);

      //
      // parse input string
      //
      if (InputStr != NULL && (InputStr[Index] == '.' || InputStr[Index] == 'q' || InputStr[Index] == 'Q')) {
        Complete = TRUE;
      } else if (InputStr == NULL || InputStr[Index] == CHAR_NULL) {
        //
        // Continue to next address
        //
      } else if (GetHex (InputStr + Index, &Buffer) && Buffer <= MaxNum[Width]) {
        if (AccessType == EFIMemoryMappedIo) {
          IoDev->Mem.Write (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiIo) {
          IoDev->Io.Write (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiPciConfig) {
          IoDev->Pci.Write (IoDev, Width, Address, 1, &Buffer);
        } else if (AccessType == EfiPciEConfig) {
          IoDev->Pci.Write (IoDev, Width, PciEAddress, 1, &Buffer);
        } else {
          WriteMem (Width, Address, 1, &Buffer);
        }
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ERROR), gShellDebug1HiiHandle, L"mm");  
        continue;
  //      PrintToken (STRING_TOKEN (STR_IOMOD_ERROR), HiiHandle);
      }

      Address += Size;
      if (AccessType == EfiPciEConfig) {
        GetPciEAddressFromInputAddress (Address, &PciEAddress);
      }
      ShellPrintEx(-1, -1, L"\r\n");
  //    Print (L"\n");
    } while (!Complete);
  }
  ASSERT(ShellStatus == SHELL_SUCCESS);
Done:

  if (InputStr != NULL) {
    FreePool(InputStr);
  }
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }
  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }
  return ShellStatus;
}
