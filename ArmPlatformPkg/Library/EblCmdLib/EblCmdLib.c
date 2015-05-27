/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiDxe.h>
#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/EblCmdLib.h>
#include <Library/BaseLib.h>
#include <Library/DxeServicesTableLib.h>
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
#include <Library/BdsLib.h>

#include <Guid/DebugImageInfoTable.h>

#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>

EFI_STATUS
EblDumpMmu (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  );

EFI_STATUS
EblDumpFdt (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  );

/**
  Simple arm disassembler via a library

  Argv[0] - symboltable
  Argv[1] - Optional qoted format string
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
  // Default string is for RealView debugger
#if (__ARMCC_VERSION < 500000)
  Format = (Argc > 1) ? Argv[1] : "load /a /ni /np %a &0x%x";
#else
  Format = (Argc > 1) ? Argv[1] : "add-symbol-file %a 0x%x";
#endif
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
        ImageBase = (UINTN)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
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


STATIC CHAR8 *mTokenList[] = {
  /*"SEC",*/
  "PEI",
  "DXE",
  /*"BDS",*/
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
  BOOLEAN     CountUp;

  TicksPerSecond = GetPerformanceCounterProperties (&Start, &Stop);
  if (Start < Stop) {
    CountUp = TRUE;
  } else {
    CountUp = FALSE;
  }

  Key       = 0;
  do {
    Key = GetPerformanceMeasurement (Key, (CONST VOID **)&Handle, &Token, &Module, &Start, &Stop);
    if (Key != 0) {
      if (AsciiStriCmp ("StartImage:", Token) == 0) {
        if (Stop == 0) {
          // The entry for EBL is still running so the stop time will be zero. Skip it
          AsciiPrint ("   running     %a\n", ImageHandleToPdbFileName ((EFI_HANDLE)Handle));
        } else {
          Delta =  CountUp?(Stop - Start):(Start - Stop);
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
          Delta =  CountUp?(Stop - Start):(Start - Stop);
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

#define EFI_MEMORY_PORT_IO  0x4000000000000000ULL

EFI_STATUS
EblDumpGcd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                        Status;
  UINTN                           NumberOfDescriptors;
  UINTN i;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR *MemorySpaceMap;
  EFI_GCD_IO_SPACE_DESCRIPTOR *IoSpaceMap;

  Status = gDS->GetMemorySpaceMap(&NumberOfDescriptors,&MemorySpaceMap);
  if (EFI_ERROR (Status)) {
      return Status;
  }
  AsciiPrint ("    Address Range       Image     Device   Attributes\n");
  AsciiPrint ("__________________________________________________________\n");
  for (i=0; i < NumberOfDescriptors; i++) {
    AsciiPrint ("MEM %016lx - %016lx",(UINT64)MemorySpaceMap[i].BaseAddress,MemorySpaceMap[i].BaseAddress+MemorySpaceMap[i].Length-1);
    AsciiPrint (" %08x %08x",MemorySpaceMap[i].ImageHandle,MemorySpaceMap[i].DeviceHandle);

    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_RUNTIME)
        AsciiPrint (" RUNTIME");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_PORT_IO)
        AsciiPrint (" PORT_IO");

    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_UC)
        AsciiPrint (" MEM_UC");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_WC)
        AsciiPrint (" MEM_WC");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_WT)
        AsciiPrint (" MEM_WT");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_WB)
        AsciiPrint (" MEM_WB");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_UCE)
        AsciiPrint (" MEM_UCE");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_WP)
        AsciiPrint (" MEM_WP");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_RP)
        AsciiPrint (" MEM_RP");
    if (MemorySpaceMap[i].Attributes & EFI_MEMORY_XP)
        AsciiPrint (" MEM_XP");

    switch (MemorySpaceMap[i].GcdMemoryType) {
      case EfiGcdMemoryTypeNonExistent:
        AsciiPrint (" TYPE_NONEXISTENT");
        break;
      case EfiGcdMemoryTypeReserved:
        AsciiPrint (" TYPE_RESERVED");
        break;
      case EfiGcdMemoryTypeSystemMemory:
        AsciiPrint (" TYPE_SYSMEM");
        break;
      case EfiGcdMemoryTypeMemoryMappedIo:
        AsciiPrint (" TYPE_MEMMAP");
        break;
      default:
        AsciiPrint (" TYPE_UNKNOWN");
        break;
    }

    AsciiPrint ("\n");
  }

  FreePool (MemorySpaceMap);

  Status = gDS->GetIoSpaceMap(&NumberOfDescriptors,&IoSpaceMap);
  if (EFI_ERROR (Status)) {
      return Status;
  }
  for (i=0; i < NumberOfDescriptors; i++) {
    AsciiPrint ("IO  %08lx - %08lx",IoSpaceMap[i].BaseAddress,IoSpaceMap[i].BaseAddress+IoSpaceMap[i].Length);
    AsciiPrint ("\t%08x %08x",IoSpaceMap[i].ImageHandle,IoSpaceMap[i].DeviceHandle);

    switch (IoSpaceMap[i].GcdIoType) {
      case EfiGcdIoTypeNonExistent:
        AsciiPrint (" TYPE_NONEXISTENT");
        break;
      case EfiGcdIoTypeReserved:
        AsciiPrint (" TYPE_RESERVED");
        break;
      case EfiGcdIoTypeIo:
        AsciiPrint (" TYPE_IO");
        break;
      default:
        AsciiPrint (" TYPE_UNKNOWN");
        break;
    }

    AsciiPrint ("\n");
  }

  FreePool (IoSpaceMap);

  return EFI_SUCCESS;
}

EFI_STATUS
EblDevicePaths (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS Status;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuffer;
  UINTN                              Index;
  CHAR16*                            String;
  EFI_DEVICE_PATH_PROTOCOL*          DevicePathProtocol;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL*  DevicePathToTextProtocol;

  BdsConnectAllDrivers();

  Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("Did not find the DevicePathToTextProtocol.\n");
    return EFI_SUCCESS;
  }

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiDevicePathProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("No device path found\n");
    return EFI_SUCCESS;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathProtocol);
    String = DevicePathToTextProtocol->ConvertDevicePathToText(DevicePathProtocol,TRUE,TRUE);
    Print (L"[0x%X] %s\n",(UINTN)HandleBuffer[Index], String);
  }

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
  },
  {
    "dumpgcd",
    " dump Global Coherency Domain",
    NULL,
    EblDumpGcd
  },
  {
    "dumpmmu",
    " dump MMU Table",
    NULL,
    EblDumpMmu
  },
  {
    "devicepaths",
    " list all the Device Paths",
    NULL,
    EblDevicePaths
  },
  {
    "dumpfdt",
    " dump the current fdt or the one defined in the arguments",
    NULL,
    EblDumpFdt
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
