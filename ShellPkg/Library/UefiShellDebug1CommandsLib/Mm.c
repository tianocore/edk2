/** @file
  Main file for Mm shell Debug1 function.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
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

EFI_STATUS
EFIAPI
DumpIoModify (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

VOID
EFIAPI
ReadMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64        Address,
  IN  UINTN         Size,
  IN  VOID          *Buffer
  );

VOID
EFIAPI
WriteMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64        Address,
  IN  UINTN         Size,
  IN  VOID          *Buffer
  );

BOOLEAN
EFIAPI
GetHex (
  IN  UINT16  *str,
  OUT UINT64  *data
  );

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

STATIC CONST UINT64 MaxNum[9]      = { 0xff, 0xffff, 0xffffffff, 0xffffffffffffffff };

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
  CHAR16                          *AddressStr;
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
  AddressStr  = NULL;
//  ValueStr    = NULL;
  Interactive = TRUE;
  Package     = NULL;

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) < 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      if (ShellCommandLineGetFlag(Package, L"-mmio")) {
        AccessType = EFIMemoryMappedIo;
      } else if (ShellCommandLineGetFlag(Package, L"-mem")) {
        AccessType = EfiMemory;
      } else if (ShellCommandLineGetFlag(Package, L"-io")) {
        AccessType = EfiIo;
      } else if (ShellCommandLineGetFlag(Package, L"-pci")) {
        AccessType = EfiPciConfig;
      } else if (ShellCommandLineGetFlag(Package, L"-pcie")) {
        AccessType = EfiPciEConfig;
      }
    }

    if (ShellCommandLineGetFlag (Package, L"-n")) {
      Interactive = FALSE;
    }

    Temp = ShellCommandLineGetValue(Package, L"-w");
    if (Temp != NULL) {
      ItemValue = StrDecimalToUintn (Temp);

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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"-w");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    Temp = ShellCommandLineGetRawValue(Package, 1);
    if (Temp != NULL) {
      Address = StrHexToUint64(Temp);
    }

    Temp = ShellCommandLineGetRawValue(Package, 2);
    if (Temp != NULL) {
      Value = StrHexToUint64(Temp);
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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    if ((Address & (Size - 1)) != 0) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_NOT_ALIGNED), gShellDebug1HiiHandle, Address);
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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle);
        ShellStatus = SHELL_NOT_FOUND;
        goto Done;
      }
      //
      // In the case of PCI or PCIE
      // Get segment number and mask the segment bits in Address
      //
      if (AccessType == EfiPciEConfig) {
        SegmentNumber = (UINT32) RShiftU64 (Address, 36) & 0xff;
        Address      &= 0xfffffffff;
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
        // TODO add token
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_SEGMENT_NOT_FOUND), gShellDebug1HiiHandle, SegmentNumber);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    if (AccessType == EfiIo && Address + Size > 0x10000) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS_RANGE), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (AccessType == EfiPciEConfig) {
      GetPciEAddressFromInputAddress (Address, &PciEAddress);
    }

//    //
//    // Set value
//    //
//    if (ValueStr != NULL) {
//      if (AccessType == EFIMemoryMappedIo) {
//        IoDev->Mem.Write (IoDev, Width, Address, 1, &Value);
//      } else if (AccessType == EfiIo) {
//        IoDev->Io.Write (IoDev, Width, Address, 1, &Value);
//      } else if (AccessType == EfiPciConfig) {
//        IoDev->Pci.Write (IoDev, Width, Address, 1, &Value);
//      } else if (AccessType == EfiPciEConfig) {
//        IoDev->Pci.Write (IoDev, Width, PciEAddress, 1, &Buffer);
//      } else {
//        WriteMem (Width, Address, 1, &Value);
//      }
//
//      ASSERT(ShellStatus == SHELL_SUCCESS);
//      goto Done;
//    }


    //
    // non-interactive mode
    //
    if (!Interactive) {
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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF2), gShellDebug1HiiHandle, Buffer);
      } else if (Size == 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF4), gShellDebug1HiiHandle, Buffer);
      } else if (Size == 4) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF8), gShellDebug1HiiHandle, Buffer);
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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS_RANGE2), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_IO_ADDRESS_2), HiiHandle, L"mm");
        break;
      }

      Buffer = 0;
      if (AccessType == EFIMemoryMappedIo) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_MMIO), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_HMMIO), HiiHandle);
        IoDev->Mem.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiIo) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_IO), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_HIO), HiiHandle);
        IoDev->Io.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiPciConfig) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_PCI), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_HPCI), HiiHandle);
        IoDev->Pci.Read (IoDev, Width, Address, 1, &Buffer);
      } else if (AccessType == EfiPciEConfig) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_PCIE), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_HPCIE), HiiHandle);
        IoDev->Pci.Read (IoDev, Width, PciEAddress, 1, &Buffer);
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_MEM), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_HMEM), HiiHandle);
        ReadMem (Width, Address, 1, &Buffer);
      }

      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS), gShellDebug1HiiHandle, Address);
  //    PrintToken (STRING_TOKEN (STR_IOMOD_ADDRESS), HiiHandle, Address);

      if (Size == 1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF2), gShellDebug1HiiHandle, Buffer);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_BUFFER_2), HiiHandle, Buffer);
      } else if (Size == 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF4), gShellDebug1HiiHandle, Buffer);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_BUFFER_4), HiiHandle, Buffer);
      } else if (Size == 4) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF8), gShellDebug1HiiHandle, Buffer);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_BUFFER_8), HiiHandle, Buffer);
      } else if (Size == 8) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_BUF16), gShellDebug1HiiHandle, Buffer);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_BUFFER_16), HiiHandle, Buffer);
      }
      //
      // wait user input to modify
      //
      if (InputStr != NULL) {
        FreePool(InputStr);
      }
      ShellPromptForResponse(ShellPromptResponseTypeFreeform, NULL, (VOID**)&InputStr);

      //
      // skip space characters
      //
      for (Index = 0; InputStr[Index] == ' '; Index++);

      //
      // parse input string
      //
      if (InputStr[Index] == '.' || InputStr[Index] == 'q' || InputStr[Index] == 'Q') {
        Complete = TRUE;
      } else if (InputStr[Index] == CHAR_NULL) {
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
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_ERROR), gShellDebug1HiiHandle);
  //      PrintToken (STRING_TOKEN (STR_IOMOD_ERROR), HiiHandle);
        continue;
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


VOID
EFIAPI
ReadMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64        Address,
  IN  UINTN         Size,
  IN  VOID          *Buffer
  )
{
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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MM_READ_ERROR), gShellDebug1HiiHandle);
//      PrintToken (STRING_TOKEN (STR_IOMOD_READ_MEM_ERROR), HiiHandle);
      break;
    }
    //
    //
    //
    Size--;
  } while (Size > 0);
}

VOID
EFIAPI
WriteMem (
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  Width,
  IN  UINT64        Address,
  IN  UINTN         Size,
  IN  VOID          *Buffer
  )
{
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

BOOLEAN
EFIAPI
GetHex (
  IN  UINT16  *str,
  OUT UINT64  *data
  )
{
  UINTN   u;
  CHAR16  c;
  BOOLEAN Find;

  Find = FALSE;
  //
  // convert hex digits
  //
  u = 0;
  c = *(str++);
  while (c != CHAR_NULL) {
    if (c >= 'a' && c <= 'f') {
      c -= 'a' - 'A';
    }

    if (c == ' ') {
      break;
    }

    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
      u     = u << 4 | c - (c >= 'A' ? 'A' - 10 : '0');

      Find  = TRUE;
    } else {
      return FALSE;
    }

    c = *(str++);
  }

  *data = u;
  return Find;
}
