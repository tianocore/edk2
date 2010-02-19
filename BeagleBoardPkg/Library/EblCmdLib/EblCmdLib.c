/** @file
  Add custom commands for BeagleBoard development.

  Copyright (c) 2008-2010, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
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

#include <Guid/DebugImageInfoTable.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

/**
  Simple arm disassembler via a library

  Argv[0] - symboltable
  Argv[1] - Optional qoted format string 
  Argv[2] - Optional flag

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line. 
                 Argv[0] is the comamnd name

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
  Format = (Argc > 1) ? Argv[1] : "load /a /ni /np %a & 0x%x\n";
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
        if (Elf) {
          // ELF and Mach-O images don't include the header so the linked address does not include header
          ImageBase += PeCoffSizeOfHeaders;
        } 
        AsciiPrint (Format, Pdb, ImageBase);
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
                 Argv[0] is the comamnd name

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


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mLibCmdTemplate[] =
{
  {
    "disasm address [count]",
    " disassemble count instructions",
    NULL,
    EblDisassembler
  },
  {
    "symboltable [\"format string\"] [TRUE]",
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
