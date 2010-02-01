/** @file
  Add custom commands for BeagleBoard development.

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
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

//PcdEmbeddedFdBaseAddress

/**
  Fill Me In

  Argv[0] - "%CommandName%"

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
  UINT8   *Ptr;
  UINT32  Address;
  UINT32  Count;
  CHAR8   Buffer[80];
  
  if (Argc < 2) {
    return EFI_INVALID_PARAMETER;
  }
  
  Address = AsciiStrHexToUintn (Argv[1]);
  Count   = (Argc > 2) ? (UINT32)AsciiStrHexToUintn (Argv[2]) : 10;

  Ptr = (UINT8 *)(UINTN)Address;  
  while (Count-- > 0) {
    DisassembleInstruction (&Ptr, TRUE, TRUE, Buffer, sizeof (Buffer));
    AsciiPrint ("0x%08x: %a", Address, Buffer);
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
