/** @file
  SMM STM support functions

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/DebugLib.h>

#include "SmmStm.h"

///
/// Page Table Entry
///
#define IA32_PG_P                   BIT0
#define IA32_PG_RW                  BIT1
#define IA32_PG_PS                  BIT7

/**

  Create 4G page table for STM.
  4M Non-PAE page table in IA32 version.

  @param PageTableBase        The page table base in MSEG

**/
VOID
StmGen4GPageTable (
  IN UINTN              PageTableBase
  )
{
  UINTN                             Index;
  UINT32                            *Pte;
  UINT32                            Address;

  Pte = (UINT32*)(UINTN)PageTableBase;

  Address = 0;
  for (Index = 0; Index < SIZE_4KB / sizeof (*Pte); Index++) {
    *Pte = Address | IA32_PG_PS | IA32_PG_RW | IA32_PG_P;
    Pte++;
    Address += SIZE_4MB;
  }
}

/**
  This is SMM exception handle.
  Consumed by STM when exception happen.

  @param Context  STM protection exception stack frame

  @return the EBX value for STM reference.
          EBX = 0: resume SMM guest using register state found on exception stack.
          EBX = 1 to 0x0F: EBX contains a BIOS error code which the STM must record in the
                           TXT.ERRORCODE register and subsequently reset the system via
                           TXT.CMD.SYS_RESET. The value of the TXT.ERRORCODE register is calculated as
                           follows: TXT.ERRORCODE = (EBX & 0x0F) | STM_CRASH_BIOS_PANIC
          EBX = 0x10 to 0xFFFFFFFF - reserved, do not use.

**/
UINT32
EFIAPI
SmmStmExceptionHandler (
  IN OUT STM_PROTECTION_EXCEPTION_STACK_FRAME Context
  )
{
  // TBD - SmmStmExceptionHandler, record information
  DEBUG ((DEBUG_ERROR, "SmmStmExceptionHandler ...\n"));
  //
  // Skip this instruction and continue;
  //
  Context.Ia32StackFrame->Rip += Context.Ia32StackFrame->VmcsExitInstructionLength;

  return 0;
}
