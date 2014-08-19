/** @file
  Add custom commands for BeagleBoard development.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/EblCmdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/EfiFileLib.h>
#include <Library/ArmDisassemblerLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PerformanceLib.h>
#include <Library/TimerLib.h>

#include <Guid/DebugImageInfoTable.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

/**
  Simple arm disassembler via a library

  Argv[0] - symboltable
  Argv[1] - Optional quoted format string
  Argv[2] - Optional flag

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblSymbolTable (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                        Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER *DebugImageTableHeader = NULL;
  EFI_DEBUG_IMAGE_INFO              *DebugTable;
  UINTN                             Entry;
  CHAR8                             *Format;
  CHAR8                             *Pdb;
  UINT32                            PeCoffSizeOfHeaders;
  UINT32                            ImageBase;
  BOOLEAN                           Elf;

  // Need to add lots of error checking on the passed in string
  // Default string is for RealView debugger or gdb depending on toolchain used.
  if (Argc > 1) {
    Format = Argv[1];
  } else {
#if __GNUC__
    // Assume gdb
    Format = "add-symbol-file %a 0x%x";
#else
    // Default to RVCT
    Format = "load /a /ni /np %a &0x%x";
#endif
  }
  Elf = (Argc > 2) ? FALSE : TRUE;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&DebugImageTableHeader);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DebugTable = DebugImageTableHeader->EfiDebugImageInfoTable;
  if (DebugTable == NULL) {
    return EFI_SUCCESS;
  }

  for (Entry = 0; Entry < DebugImageTableHeader->TableSize; Entry++, DebugTable++) {
    if (DebugTable->NormalImage != NULL) {
      if ((DebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) && (DebugTable->NormalImage->LoadedImageProtocolInstance != NULL)) {
        ImageBase = (UINT32)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
        PeCoffSizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)ImageBase);
        Pdb = PeCoffLoaderGetPdbPointer (DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase);
        if (Pdb != NULL) {
          if (Elf) {
            // ELF and Mach-O images don't include the header so the linked address does not include header
            ImageBase += PeCoffSizeOfHeaders;
          }
          AsciiPrint (Format, Pdb, ImageBase);
          AsciiPrint ("\n");
        } else {
        }
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Simple arm disassembler via a library

  Argv[0] - disasm
  Argv[1] - Address to start disassembling from
  ARgv[2] - Number of instructions to disassembly (optional)

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblDisassembler (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINT8   *Ptr, *CurrentAddress;
  UINT32  Address;
  UINT32  Count;
  CHAR8   Buffer[80];
  UINT32  ItBlock;

  if (Argc < 2) {
    return EFI_INVALID_PARAMETER;
  }

  Address = AsciiStrHexToUintn (Argv[1]);
  Count   = (Argc > 2) ? (UINT32)AsciiStrHexToUintn (Argv[2]) : 20;

  Ptr = (UINT8 *)(UINTN)Address;
  ItBlock = 0;
  do {
    CurrentAddress = Ptr;
    DisassembleInstruction (&Ptr, TRUE, TRUE, &ItBlock, Buffer, sizeof (Buffer));
    AsciiPrint ("0x%08x: %a\n", CurrentAddress, Buffer);
  } while (Count-- > 0);


  return EFI_SUCCESS;
}


CHAR8 *
ImageHandleToPdbFileName (
  IN  EFI_HANDLE    Handle
  )
{
  EFI_STATUS                  Status;
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage;
  CHAR8                       *Pdb;
  CHAR8                       *StripLeading;

  Status = gBS->HandleProtocol (Handle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  if (EFI_ERROR (Status)) {
    return "";
  }

  Pdb = PeCoffLoaderGetPdbPointer (LoadedImage->ImageBase);
  StripLeading = AsciiStrStr (Pdb, "\\ARM\\");
  if (StripLeading == NULL) {
    StripLeading = AsciiStrStr (Pdb, "/ARM/");
    if (StripLeading == NULL) {
      return Pdb;
    }
  }
  // Hopefully we hacked off the unneeded part
  return (StripLeading + 5);
}


CHAR8 *mTokenList[] = {
  "SEC",
  "PEI",
  "DXE",
  "BDS",
  NULL
};

/**
  Simple arm disassembler via a library

  Argv[0] - disasm
  Argv[1] - Address to start disassembling from
  ARgv[2] - Number of instructions to disassembly (optional)

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblPerformance (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN       Key;
  CONST VOID  *Handle;
  CONST CHAR8 *Token, *Module;
  UINT64      Start, Stop, TimeStamp;
  UINT64      Delta, TicksPerSecond, Milliseconds, Microseconds;
  UINTN       Index;

  TicksPerSecond = GetPerformanceCounterProperties (NULL, NULL);

  Key       = 0;
  do {
    Key = GetPerformanceMeasurement (Key, (CONST VOID **)&Handle, &Token, &Module, &Start, &Stop);
    if (Key != 0) {
      if (AsciiStriCmp ("StartImage:", Token) == 0) {
        if (Stop == 0) {
          // The entry for EBL is still running so the stop time will be zero. Skip it
          AsciiPrint ("   running     %a\n", ImageHandleToPdbFileName ((EFI_HANDLE)Handle));
        } else {
          Delta = Stop - Start;
          Microseconds = DivU64x64Remainder (MultU64x32 (Delta, 1000000), TicksPerSecond, NULL);
          AsciiPrint ("%10ld us  %a\n", Microseconds, ImageHandleToPdbFileName ((EFI_HANDLE)Handle));
        }
      }
    }
  } while (Key != 0);

  AsciiPrint ("\n");

  TimeStamp = 0;
  Key       = 0;
  do {
    Key = GetPerformanceMeasurement (Key, (CONST VOID **)&Handle, &Token, &Module, &Start, &Stop);
    if (Key != 0) {
      for (Index = 0; mTokenList[Index] != NULL; Index++) {
        if (AsciiStriCmp (mTokenList[Index], Token) == 0) {
          Delta = Stop - Start;
          TimeStamp += Delta;
          Milliseconds = DivU64x64Remainder (MultU64x32 (Delta, 1000), TicksPerSecond, NULL);
          AsciiPrint ("%6a %6ld ms\n", Token, Milliseconds);
          break;
        }
      }
    }
  } while (Key != 0);

  AsciiPrint ("Total Time = %ld ms\n\n", DivU64x64Remainder (MultU64x32 (TimeStamp, 1000), TicksPerSecond, NULL));

  return EFI_SUCCESS;
}


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mLibCmdTemplate[] =
{
  {
    "disasm address [count]",
    " disassemble count instructions",
    NULL,
    EblDisassembler
  },
  {
    "performance",
    " Display boot performance info",
    NULL,
    EblPerformance
  },
  {
    "symboltable [\"format string\"] [PECOFF]",
    " show symbol table commands for debugger",
    NULL,
    EblSymbolTable
  }
};


VOID
EblInitializeExternalCmd (
  VOID
  )
{
  EblAddCommands (mLibCmdTemplate, sizeof (mLibCmdTemplate)/sizeof (EBL_COMMAND_TABLE));
  return;
}
