/** @file
  Basic command line parser for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  HwDebug.c

  Commands useful for debugging hardware.

**/

#include "Ebl.h"


/**
  Dump memory

  Argv[0] - "md"[.#] # is optional width 1, 2, 4, or 8. Default 1
  Argv[1] - Hex Address to dump
  Argv[2] - Number of hex bytes to dump (0x20 is default)

  md.4 0x123445678 50 ; Dump 0x50 4 byte quantities starting at 0x123445678
  md   0x123445678 40 ; Dump 0x40 1 byte quantities starting at 0x123445678
  md   0x123445678    ; Dump 0x20 1 byte quantities starting at 0x123445678

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblMdCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  STATIC UINT8  *Address = NULL;
  STATIC UINTN  Length   = 0x20;
  STATIC UINTN  Width;

  Width = WidthFromCommandName (Argv[0], 1);

  switch (Argc) {
    case 3:
      Length = AsciiStrHexToUintn(Argv[2]);
    case 2:
      Address = (UINT8 *)AsciiStrHexToUintn (Argv[1]);
    default:
      break;
  }

  OutputData (Address, Length, Width, (UINTN)Address);

  Address += Length;

  return EFI_SUCCESS;
}


/**
  Fill Memory with data

  Argv[0] - "mfill"[.#] # is optional width 1, 2, 4, or 8. Default 4
  Argv[1] - Hex Address to fill
  Argv[2] - Data to write (0x00 is default)
  Argv[3] - Number of units to dump.

  mf.1 0x123445678 aa 100 ; Start at 0x123445678 and write aa (1 byte) to the next 100 bytes
  mf.4 0x123445678 aa 100 ; Start at 0x123445678 and write aa (4 byte) to the next 400 bytes
  mf 0x123445678 aa       ; Start at 0x123445678 and write aa (4 byte) to the next 1 byte
  mf 0x123445678          ; Start at 0x123445678 and write 00 (4 byte) to the next 1 byte

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblMfillCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN   Address;
  UINTN   EndAddress;
  UINT32  Data;
  UINTN   Length;
  UINTN   Width;

  if (Argc < 2) {
    return EFI_INVALID_PARAMETER;
  }

  Width = WidthFromCommandName (Argv[0], 4);

  Address = AsciiStrHexToUintn (Argv[1]);
  Data    = (Argc > 2) ? (UINT32)AsciiStrHexToUintn (Argv[2]) : 0;
  Length  = (Argc > 3) ? AsciiStrHexToUintn (Argv[3]) : 1;

  for (EndAddress = Address + (Length * Width); Address < EndAddress; Address += Width) {
    if (Width == 4) {
      MmioWrite32 (Address, Data);
    } else if (Width == 2) {
      MmioWrite16 (Address, (UINT16)Data);
    } else {
      MmioWrite8 (Address, (UINT8)Data);
    }
  }

  return EFI_SUCCESS;
}


//
// Strings for PCI Class code [2]
//
CHAR8 *gPciDevClass[] = {
  "Old Device             ",
  "Mass storage           ",
  "Network                ",
  "Display                ",
  "Multimedia             ",
  "Memory controller      ",
  "Bridge device          ",
  "simple communications  ",
  "base system peripherals",
  "Input devices          ",
  "Docking stations       ",
  "Processors             ",
  "serial bus             ",
};


CHAR8 *gPciSerialClassCodes[] = {
  "Mass storage           ",
  "Firewire               ",
  "ACCESS bus             ",
  "SSA                    ",
  "USB                     "
};


/**
  PCI Dump

  Argv[0] - "pci"
  Argv[1] - bus
  Argv[2] - dev
  Argv[3] - func

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblPciCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                    Status;
  EFI_PCI_IO_PROTOCOL           *Pci;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         Seg;
  UINTN                         Bus;
  UINTN                         Dev;
  UINTN                         Func;
  UINTN                         BusArg;
  UINTN                         DevArg;
  UINTN                         FuncArg;
  UINTN                         Index;
  UINTN                         Count;
  PCI_TYPE_GENERIC              PciHeader;
  PCI_TYPE_GENERIC              *Header;
  PCI_BRIDGE_CONTROL_REGISTER   *Bridge;
  PCI_DEVICE_HEADER_TYPE_REGION *Device;
  PCI_DEVICE_INDEPENDENT_REGION *Hdr;
  CHAR8                         *Str;
  UINTN                         ThisBus;


  BusArg  = (Argc > 1) ? AsciiStrDecimalToUintn (Argv[1]) : 0;
  DevArg  = (Argc > 2) ? AsciiStrDecimalToUintn (Argv[2]) : 0;
  FuncArg = (Argc > 3) ? AsciiStrDecimalToUintn (Argv[3]) : 0;

  Header = &PciHeader;

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("No PCI devices found in the system\n");
    return EFI_SUCCESS;
  }

  if (Argc == 1) {
    // Dump all PCI devices
    AsciiPrint ("BusDevFun  VendorId DeviceId  Device Class          Sub-Class\n");
    AsciiPrint ("_____________________________________________________________");
    for (ThisBus = 0; ThisBus <= PCI_MAX_BUS; ThisBus++) {
      for (Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **)&Pci);
        if (!EFI_ERROR (Status)) {
          Pci->GetLocation (Pci, &Seg, &Bus, &Dev, &Func);
          if (ThisBus != Bus) {
            continue;
          }
          AsciiPrint ("\n%03d.%02d.%02d", Bus, Dev, Func);
          Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, 0, sizeof (PciHeader)/sizeof (UINT32), &PciHeader);
          if (!EFI_ERROR (Status)) {
            Hdr = &PciHeader.Bridge.Hdr;

            if (Hdr->ClassCode[2] < sizeof (gPciDevClass)/sizeof (VOID *)) {
              Str = gPciDevClass[Hdr->ClassCode[2]];
              if (Hdr->ClassCode[2] == PCI_CLASS_SERIAL) {
                if (Hdr->ClassCode[1] < sizeof (gPciSerialClassCodes)/sizeof (VOID *)) {
                  // print out Firewire or USB inplace of Serial Bus controllers
                  Str = gPciSerialClassCodes[Hdr->ClassCode[1]];
                }
              }
            } else {
              Str = "Unknown device         ";
            }
            AsciiPrint ("  0x%04x   0x%04x    %a 0x%02x", Hdr->VendorId, Hdr->DeviceId, Str, Hdr->ClassCode[1]);
          }
          if (Seg != 0) {
            // Only print Segment if it is non zero. If you only have one PCI segment it is
            // redundent to print it out
            AsciiPrint (" Seg:%d", Seg);
          }
        }
      }
    }
    AsciiPrint ("\n");
  } else {
    // Dump specific PCI device
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **)&Pci);
      if (!EFI_ERROR (Status)) {
        Pci->GetLocation (Pci, &Seg, &Bus, &Dev, &Func);
        if ((Bus == BusArg) && (Dev == DevArg) && (Func == FuncArg)) {
          // Only print Segment if it is non zero. If you only have one PCI segment it is
          // redundant to print it out
          if (Seg != 0) {
            AsciiPrint ("Seg:%d ", Seg);
          }
          AsciiPrint ("Bus:%d Dev:%d Func:%d ", Bus, Dev, Func);

          Status = Pci->Pci.Read (Pci, EfiPciIoWidthUint32, 0, sizeof (PciHeader)/sizeof (UINT32), Header);
          if (!EFI_ERROR (Status)) {
            Hdr = &PciHeader.Bridge.Hdr;
            if (IS_PCI_BRIDGE (&PciHeader.Bridge)) {
              Bridge = &PciHeader.Bridge.Bridge;
              AsciiPrint (
                "PCI Bridge. Bus Primary %d Secondary %d Subordinate %d\n",
                Bridge->PrimaryBus, Bridge->SecondaryBus, Bridge->SubordinateBus
                );
              AsciiPrint ("  Bar 0: 0x%08x  Bar 1: 0x%08x\n", Bridge->Bar[0], Bridge->Bar[1]);
            } else {
              Device = &PciHeader.Device.Device;
              AsciiPrint (
                "VendorId: 0x%04x DeviceId: 0x%04x SubSusVendorId: 0x%04x SubSysDeviceId: 0x%04x\n",
                Hdr->VendorId, Hdr->DeviceId, Device->SubsystemVendorID, Device->SubsystemID
                );
              AsciiPrint ("  Class Code: 0x%02x 0x%02x 0x%02x\n", Hdr->ClassCode[2], Hdr->ClassCode[1], Hdr->ClassCode[0]);
              for (Count = 0; Count < 6; Count++) {
                AsciiPrint ("  Bar %d: 0x%08x\n", Count, Device->Bar[Count]);
              }
            }
          }

          AsciiPrint ("\n");
          break;
        }
      }
    }
  }

  FreePool (HandleBuffer);
  return EFI_SUCCESS;
}


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdPciDebugTemplate[] = {
  {
    "pci",
    " [bus] [dev] [func]; Dump PCI",
    NULL,
    EblPciCmd
  }
};


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdHwDebugTemplate[] =
{
  {
    "md",
    "[.{1|2|4}] [Addr] [Len] [1|2|4]; Memory Dump from Addr Len bytes",
    NULL,
    EblMdCmd
  },
  {
    "mfill",
    "[.{1|2|4}] Addr Len [data]; Memory Fill Addr Len*(1|2|4) bytes of data(0)",
    NULL,
    EblMfillCmd
  },
};



/**
  Initialize the commands in this in this file
**/
VOID
EblInitializemdHwDebugCmds (
  VOID
  )
{
  if (FeaturePcdGet (PcdEmbeddedHwDebugCmd)) {
    EblAddCommands (mCmdHwDebugTemplate, sizeof (mCmdHwDebugTemplate)/sizeof (EBL_COMMAND_TABLE));
  }
  if (FeaturePcdGet (PcdEmbeddedPciDebugCmd)) {
    EblAddCommands (mCmdPciDebugTemplate, sizeof (mCmdPciDebugTemplate)/sizeof (EBL_COMMAND_TABLE));
  }
}

