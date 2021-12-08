/** @file
  Main file for Mm shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Library/ShellLib.h>
#include <Library/IoLib.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/DeviceIo.h>

typedef enum {
  ShellMmMemory,
  ShellMmMemoryMappedIo,
  ShellMmIo,
  ShellMmPci,
  ShellMmPciExpress
} SHELL_MM_ACCESS_TYPE;

CONST UINT16  mShellMmAccessTypeStr[] = {
  STRING_TOKEN (STR_MM_MEM),
  STRING_TOKEN (STR_MM_MMIO),
  STRING_TOKEN (STR_MM_IO),
  STRING_TOKEN (STR_MM_PCI),
  STRING_TOKEN (STR_MM_PCIE)
};

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-mmio", TypeFlag  },
  { L"-mem",  TypeFlag  },
  { L"-io",   TypeFlag  },
  { L"-pci",  TypeFlag  },
  { L"-pcie", TypeFlag  },
  { L"-n",    TypeFlag  },
  { L"-w",    TypeValue },
  { NULL,     TypeMax   }
};

CONST UINT64                                 mShellMmMaxNumber[] = {
  0, MAX_UINT8, MAX_UINT16, 0, MAX_UINT32, 0, 0, 0, MAX_UINT64
};
CONST EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH  mShellMmRootBridgeIoWidth[] = {
  0, EfiPciWidthUint8, EfiPciWidthUint16, 0, EfiPciWidthUint32, 0, 0, 0, EfiPciWidthUint64
};
CONST EFI_CPU_IO_PROTOCOL_WIDTH              mShellMmCpuIoWidth[] = {
  0, EfiCpuIoWidthUint8, EfiCpuIoWidthUint16, 0, EfiCpuIoWidthUint32, 0, 0, 0, EfiCpuIoWidthUint64
};

/**
  Extract the PCI segment, bus, device, function, register from
  from a PCI or PCIE format of address..

  @param[in]  PciFormat      Whether the address is of PCI format of PCIE format.
  @param[in]  Address        PCI or PCIE address.
  @param[out] Segment        PCI segment number.
  @param[out] Bus            PCI bus number.
  @param[out] Device         PCI device number.
  @param[out] Function       PCI function number.
  @param[out] Register       PCI register offset.
**/
VOID
ShellMmDecodePciAddress (
  IN BOOLEAN  PciFormat,
  IN UINT64   Address,
  OUT UINT32  *Segment,
  OUT UINT8   *Bus,
  OUT UINT8   *Device    OPTIONAL,
  OUT UINT8   *Function  OPTIONAL,
  OUT UINT32  *Register  OPTIONAL
  )
{
  if (PciFormat) {
    //
    // PCI Configuration Space.The address will have the format ssssbbddffrr,
    // where ssss = Segment, bb = Bus, dd = Device, ff = Function and rr = Register.
    //
    *Segment = (UINT32)(RShiftU64 (Address, 32) & 0xFFFF);
    *Bus     = (UINT8)(((UINT32)Address) >> 24);

    if (Device != NULL) {
      *Device = (UINT8)(((UINT32)Address) >> 16);
    }

    if (Function != NULL) {
      *Function = (UINT8)(((UINT32)Address) >> 8);
    }

    if (Register != NULL) {
      *Register = (UINT8)Address;
    }
  } else {
    //
    // PCI Express Configuration Space.The address will have the format ssssssbbddffrrr,
    // where ssss = Segment, bb = Bus, dd = Device, ff = Function and rrr = Register.
    //
    *Segment = (UINT32)(RShiftU64 (Address, 36) & 0xFFFF);
    *Bus     = (UINT8)RShiftU64 (Address, 28);
    if (Device != NULL) {
      *Device = (UINT8)(((UINT32)Address) >> 20);
    }

    if (Function != NULL) {
      *Function = (UINT8)(((UINT32)Address) >> 12);
    }

    if (Register != NULL) {
      *Register = (UINT32)(Address & 0xFFF);
    }
  }
}

/**
  Read or write some data from or into the Address.

  @param[in]      AccessType      Access type.
  @param[in]      PciRootBridgeIo PciRootBridgeIo instance.
  @param[in]      CpuIo           CpuIo instance.
  @param[in]      Read            TRUE for read, FALSE for write.
  @param[in]      Addresss        The memory location to access.
  @param[in]      Size            The size of Buffer in Width sized units.
  @param[in, out] Buffer          The buffer to read into or write from.
**/
VOID
ShellMmAccess (
  IN     SHELL_MM_ACCESS_TYPE             AccessType,
  IN     EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo,
  IN     EFI_CPU_IO2_PROTOCOL             *CpuIo,
  IN     BOOLEAN                          Read,
  IN     UINT64                           Address,
  IN     UINTN                            Size,
  IN OUT VOID                             *Buffer
  )
{
  EFI_STATUS                              Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_IO_MEM  RootBridgeIoMem;
  EFI_CPU_IO_PROTOCOL_IO_MEM              CpuIoMem;
  UINT32                                  Segment;
  UINT8                                   Bus;
  UINT8                                   Device;
  UINT8                                   Function;
  UINT32                                  Register;

  if (AccessType == ShellMmMemory) {
    if (Read) {
      CopyMem (Buffer, (VOID *)(UINTN)Address, Size);
    } else {
      CopyMem ((VOID *)(UINTN)Address, Buffer, Size);
    }
  } else {
    RootBridgeIoMem = NULL;
    CpuIoMem        = NULL;
    switch (AccessType) {
      case ShellMmPci:
      case ShellMmPciExpress:
        ASSERT (PciRootBridgeIo != NULL);
        ShellMmDecodePciAddress ((BOOLEAN)(AccessType == ShellMmPci), Address, &Segment, &Bus, &Device, &Function, &Register);
        if (Read) {
          Status = PciRootBridgeIo->Pci.Read (
                                          PciRootBridgeIo,
                                          mShellMmRootBridgeIoWidth[Size],
                                          EFI_PCI_ADDRESS (Bus, Device, Function, Register),
                                          1,
                                          Buffer
                                          );
        } else {
          Status = PciRootBridgeIo->Pci.Write (
                                          PciRootBridgeIo,
                                          mShellMmRootBridgeIoWidth[Size],
                                          EFI_PCI_ADDRESS (Bus, Device, Function, Register),
                                          1,
                                          Buffer
                                          );
        }

        ASSERT_EFI_ERROR (Status);
        return;

      case ShellMmMemoryMappedIo:
        if (PciRootBridgeIo != NULL) {
          RootBridgeIoMem = Read ? PciRootBridgeIo->Mem.Read : PciRootBridgeIo->Mem.Write;
        }

        if (CpuIo != NULL) {
          CpuIoMem = Read ? CpuIo->Mem.Read : CpuIo->Mem.Write;
        }

        break;

      case ShellMmIo:
        if (PciRootBridgeIo != NULL) {
          RootBridgeIoMem = Read ? PciRootBridgeIo->Io.Read : PciRootBridgeIo->Io.Write;
        }

        if (CpuIo != NULL) {
          CpuIoMem = Read ? CpuIo->Io.Read : CpuIo->Io.Write;
        }

        break;
      default:
        ASSERT (FALSE);
        break;
    }

    Status = EFI_UNSUPPORTED;
    if (RootBridgeIoMem != NULL) {
      Status = RootBridgeIoMem (PciRootBridgeIo, mShellMmRootBridgeIoWidth[Size], Address, 1, Buffer);
    }

    if (EFI_ERROR (Status) && (CpuIoMem != NULL)) {
      Status = CpuIoMem (CpuIo, mShellMmCpuIoWidth[Size], Address, 1, Buffer);
    }

    if (EFI_ERROR (Status)) {
      if (AccessType == ShellMmIo) {
        switch (Size) {
          case 1:
            if (Read) {
              *(UINT8 *)Buffer = IoRead8 ((UINTN)Address);
            } else {
              IoWrite8 ((UINTN)Address, *(UINT8 *)Buffer);
            }

            break;
          case 2:
            if (Read) {
              *(UINT16 *)Buffer = IoRead16 ((UINTN)Address);
            } else {
              IoWrite16 ((UINTN)Address, *(UINT16 *)Buffer);
            }

            break;
          case 4:
            if (Read) {
              *(UINT32 *)Buffer = IoRead32 ((UINTN)Address);
            } else {
              IoWrite32 ((UINTN)Address, *(UINT32 *)Buffer);
            }

            break;
          case 8:
            if (Read) {
              *(UINT64 *)Buffer = IoRead64 ((UINTN)Address);
            } else {
              IoWrite64 ((UINTN)Address, *(UINT64 *)Buffer);
            }

            break;
          default:
            ASSERT (FALSE);
            break;
        }
      } else {
        switch (Size) {
          case 1:
            if (Read) {
              *(UINT8 *)Buffer = MmioRead8 ((UINTN)Address);
            } else {
              MmioWrite8 ((UINTN)Address, *(UINT8 *)Buffer);
            }

            break;
          case 2:
            if (Read) {
              *(UINT16 *)Buffer = MmioRead16 ((UINTN)Address);
            } else {
              MmioWrite16 ((UINTN)Address, *(UINT16 *)Buffer);
            }

            break;
          case 4:
            if (Read) {
              *(UINT32 *)Buffer = MmioRead32 ((UINTN)Address);
            } else {
              MmioWrite32 ((UINTN)Address, *(UINT32 *)Buffer);
            }

            break;
          case 8:
            if (Read) {
              *(UINT64 *)Buffer = MmioRead64 ((UINTN)Address);
            } else {
              MmioWrite64 ((UINTN)Address, *(UINT64 *)Buffer);
            }

            break;
          default:
            ASSERT (FALSE);
            break;
        }
      }
    }
  }
}

/**
  Find the CpuIo instance and PciRootBridgeIo instance in the platform.
  If there are multiple PciRootBridgeIo instances, the instance which manages
  the Address is returned.

  @param[in]  AccessType      Access type.
  @param[in]  Address         Address to access.
  @param[out] CpuIo           Return the CpuIo instance.
  @param[out] PciRootBridgeIo Return the proper PciRootBridgeIo instance.

  @retval TRUE  There are PciRootBridgeIo instances in the platform.
  @retval FALSE There isn't PciRootBridgeIo instance in the platform.
**/
BOOLEAN
ShellMmLocateIoProtocol (
  IN SHELL_MM_ACCESS_TYPE              AccessType,
  IN UINT64                            Address,
  OUT EFI_CPU_IO2_PROTOCOL             **CpuIo,
  OUT EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  **PciRootBridgeIo
  )
{
  EFI_STATUS                         Status;
  UINTN                              Index;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuffer;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL    *Io;
  UINT32                             Segment;
  UINT8                              Bus;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptors;

  Status = gBS->LocateProtocol (&gEfiCpuIo2ProtocolGuid, NULL, (VOID **)CpuIo);
  if (EFI_ERROR (Status)) {
    *CpuIo = NULL;
  }

  *PciRootBridgeIo = NULL;
  HandleBuffer     = NULL;
  Status           = gBS->LocateHandleBuffer (
                            ByProtocol,
                            &gEfiPciRootBridgeIoProtocolGuid,
                            NULL,
                            &HandleCount,
                            &HandleBuffer
                            );
  if (EFI_ERROR (Status) || (HandleCount == 0) || (HandleBuffer == NULL)) {
    return FALSE;
  }

  Segment = 0;
  Bus     = 0;
  if ((AccessType == ShellMmPci) || (AccessType == ShellMmPciExpress)) {
    ShellMmDecodePciAddress ((BOOLEAN)(AccessType == ShellMmPci), Address, &Segment, &Bus, NULL, NULL, NULL);
  }

  //
  // Find the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL of the specified segment & bus number
  //
  for (Index = 0; (Index < HandleCount) && (*PciRootBridgeIo == NULL); Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciRootBridgeIoProtocolGuid,
                    (VOID *)&Io
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((((AccessType == ShellMmPci) || (AccessType == ShellMmPciExpress)) && (Io->SegmentNumber == Segment)) ||
        ((AccessType == ShellMmIo) || (AccessType == ShellMmMemoryMappedIo))
        )
    {
      Status = Io->Configuration (Io, (VOID **)&Descriptors);
      if (!EFI_ERROR (Status)) {
        while (Descriptors->Desc != ACPI_END_TAG_DESCRIPTOR) {
          //
          // Compare the segment and bus range for PCI/PCIE access
          //
          if ((Descriptors->ResType == ACPI_ADDRESS_SPACE_TYPE_BUS) &&
              ((AccessType == ShellMmPci) || (AccessType == ShellMmPciExpress)) &&
              ((Bus >= Descriptors->AddrRangeMin) && (Bus <= Descriptors->AddrRangeMax))
              )
          {
            *PciRootBridgeIo = Io;
            break;

            //
            // Compare the address range for MMIO/IO access
            //
          } else if ((((Descriptors->ResType == ACPI_ADDRESS_SPACE_TYPE_IO) && (AccessType == ShellMmIo)) ||
                      ((Descriptors->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM) && (AccessType == ShellMmMemoryMappedIo))
                      ) && ((Address >= Descriptors->AddrRangeMin) && (Address <= Descriptors->AddrRangeMax))
                     )
          {
            *PciRootBridgeIo = Io;
            break;
          }

          Descriptors++;
        }
      }
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return TRUE;
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
  EFI_STATUS                       Status;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *PciRootBridgeIo;
  EFI_CPU_IO2_PROTOCOL             *CpuIo;
  UINT64                           Address;
  UINT64                           Value;
  SHELL_MM_ACCESS_TYPE             AccessType;
  UINT64                           Buffer;
  UINTN                            Index;
  UINTN                            Size;
  BOOLEAN                          Complete;
  CHAR16                           *InputStr;
  BOOLEAN                          Interactive;
  LIST_ENTRY                       *Package;
  CHAR16                           *ProblemParam;
  SHELL_STATUS                     ShellStatus;
  CONST CHAR16                     *Temp;
  BOOLEAN                          HasPciRootBridgeIo;

  Value       = 0;
  Address     = 0;
  ShellStatus = SHELL_SUCCESS;
  InputStr    = NULL;
  Size        = 1;
  AccessType  = ShellMmMemory;

  //
  // Parse arguments
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"mm", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) < 2) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"mm");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else if (ShellCommandLineGetCount (Package) > 3) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else if (ShellCommandLineGetFlag (Package, L"-w") && (ShellCommandLineGetValue (Package, L"-w") == NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"mm", L"-w");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      if (ShellCommandLineGetFlag (Package, L"-mmio")) {
        AccessType = ShellMmMemoryMappedIo;
        if (  ShellCommandLineGetFlag (Package, L"-mem")
           || ShellCommandLineGetFlag (Package, L"-io")
           || ShellCommandLineGetFlag (Package, L"-pci")
           || ShellCommandLineGetFlag (Package, L"-pcie")
              )
        {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag (Package, L"-mem")) {
        AccessType = ShellMmMemory;
        if (  ShellCommandLineGetFlag (Package, L"-io")
           || ShellCommandLineGetFlag (Package, L"-pci")
           || ShellCommandLineGetFlag (Package, L"-pcie")
              )
        {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag (Package, L"-io")) {
        AccessType = ShellMmIo;
        if (  ShellCommandLineGetFlag (Package, L"-pci")
           || ShellCommandLineGetFlag (Package, L"-pcie")
              )
        {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag (Package, L"-pci")) {
        AccessType = ShellMmPci;
        if (ShellCommandLineGetFlag (Package, L"-pcie")
            )
        {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mm");
          ShellStatus = SHELL_INVALID_PARAMETER;
          goto Done;
        }
      } else if (ShellCommandLineGetFlag (Package, L"-pcie")) {
        AccessType = ShellMmPciExpress;
      }
    }

    //
    // Non interactive for a script file or for the specific parameter
    //
    Interactive = TRUE;
    if (gEfiShellProtocol->BatchIsActive () || ShellCommandLineGetFlag (Package, L"-n")) {
      Interactive = FALSE;
    }

    Temp = ShellCommandLineGetValue (Package, L"-w");
    if (Temp != NULL) {
      Size = ShellStrToUintn (Temp);
    }

    if ((Size != 1) && (Size != 2) && (Size != 4) && (Size != 8)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellDebug1HiiHandle, L"mm", Temp, L"-w");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    Temp   = ShellCommandLineGetRawValue (Package, 1);
    Status = ShellConvertStringToUint64 (Temp, &Address, TRUE, FALSE);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mm", Temp);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if ((Address & (Size - 1)) != 0) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_NOT_ALIGNED), gShellDebug1HiiHandle, L"mm", Address);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    //
    // locate IO protocol interface
    //
    HasPciRootBridgeIo = ShellMmLocateIoProtocol (AccessType, Address, &CpuIo, &PciRootBridgeIo);
    if ((AccessType == ShellMmPci) || (AccessType == ShellMmPciExpress)) {
      if (!HasPciRootBridgeIo) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PCIRBIO_NF), gShellDebug1HiiHandle, L"mm");
        ShellStatus = SHELL_NOT_FOUND;
        goto Done;
      }

      if (PciRootBridgeIo == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_PCIE_ADDRESS_RANGE), gShellDebug1HiiHandle, L"mm", Address);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Mode 1: Directly set a value
    //
    Temp = ShellCommandLineGetRawValue (Package, 2);
    if (Temp != NULL) {
      Status = ShellConvertStringToUint64 (Temp, &Value, TRUE, FALSE);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mm", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (Value > mShellMmMaxNumber[Size]) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mm", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      ShellMmAccess (AccessType, PciRootBridgeIo, CpuIo, FALSE, Address, Size, &Value);
      goto Done;
    }

    //
    // Mode 2: Directly show a value
    //
    if (!Interactive) {
      if (!gEfiShellProtocol->BatchIsActive ()) {
        ShellPrintHiiEx (-1, -1, NULL, mShellMmAccessTypeStr[AccessType], gShellDebug1HiiHandle);
      }

      ShellMmAccess (AccessType, PciRootBridgeIo, CpuIo, TRUE, Address, Size, &Buffer);

      if (!gEfiShellProtocol->BatchIsActive ()) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS), gShellDebug1HiiHandle, Address);
      }

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_BUF), gShellDebug1HiiHandle, Size * 2, Buffer & mShellMmMaxNumber[Size]);
      ShellPrintEx (-1, -1, L"\r\n");
      goto Done;
    }

    //
    // Mode 3: Show or set values in interactive mode
    //
    Complete = FALSE;
    do {
      ShellMmAccess (AccessType, PciRootBridgeIo, CpuIo, TRUE, Address, Size, &Buffer);
      ShellPrintHiiEx (-1, -1, NULL, mShellMmAccessTypeStr[AccessType], gShellDebug1HiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_ADDRESS), gShellDebug1HiiHandle, Address);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_BUF), gShellDebug1HiiHandle, Size * 2, Buffer & mShellMmMaxNumber[Size]);
      ShellPrintEx (-1, -1, L" > ");
      //
      // wait user input to modify
      //
      if (InputStr != NULL) {
        FreePool (InputStr);
        InputStr = NULL;
      }

      ShellPromptForResponse (ShellPromptResponseTypeFreeform, NULL, (VOID **)&InputStr);

      if (InputStr != NULL) {
        //
        // skip space characters
        //
        for (Index = 0; InputStr[Index] == ' '; Index++) {
        }

        if (InputStr[Index] != CHAR_NULL) {
          if ((InputStr[Index] == '.') || (InputStr[Index] == 'q') || (InputStr[Index] == 'Q')) {
            Complete = TRUE;
          } else if (!EFI_ERROR (ShellConvertStringToUint64 (InputStr + Index, &Buffer, TRUE, TRUE)) &&
                     (Buffer <= mShellMmMaxNumber[Size])
                     )
          {
            ShellMmAccess (AccessType, PciRootBridgeIo, CpuIo, FALSE, Address, Size, &Buffer);
          } else {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MM_ERROR), gShellDebug1HiiHandle, L"mm");
            continue;
          }
        }
      }

      Address += Size;
      ShellPrintEx (-1, -1, L"\r\n");
    } while (!Complete);
  }

  ASSERT (ShellStatus == SHELL_SUCCESS);

Done:
  if (InputStr != NULL) {
    FreePool (InputStr);
  }

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  return ShellStatus;
}
