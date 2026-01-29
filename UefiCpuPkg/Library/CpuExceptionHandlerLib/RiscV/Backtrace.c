/** @file
  RISC-V backtrace implementation.

  Copyright (c) 2016 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Backtrace.h"

#define MAX_STACK_FRAME_SIZE  SIZE_16KB

STATIC
INTN
CheckFpValid (
  IN UINTN  Fp,
  IN UINTN  Sp
  )
{
  UINTN  Low, High;

  Low  = Sp + 2 * sizeof (UINTN);
  High = ALIGN_VALUE (Sp, MAX_STACK_FRAME_SIZE);

  return !(Fp < Low || Fp > High || Fp & 0x07);
}

STATIC
CONST CHAR8 *
BaseName (
  IN  CONST CHAR8  *FullName
  )
{
  CONST CHAR8  *Str;

  Str = FullName + AsciiStrLen (FullName);

  while (--Str > FullName) {
    if ((*Str == '/') || (*Str == '\\')) {
      return Str + 1;
    }
  }

  return Str;
}

/**
  Helper for displaying a backtrace.

  @param Regs       Pointer to SMODE_TRAP_REGISTERS.
  @param FirstPdb   Pointer to the first symbol file used.
  @param ListImage  If true, only show the full path to symbol file, else
                    show the PC value and its decoded components.
**/
STATIC
VOID
DumpCpuBacktraceHelper (
  IN  SMODE_TRAP_REGISTERS  *Regs,
  IN  CHAR8                 *FirstPdb,
  IN  BOOLEAN               ListImage
  )
{
  UINTN    ImageBase;
  UINTN    PeCoffSizeOfHeader;
  BOOLEAN  IsLeaf;
  UINTN    RootFp;
  UINTN    RootRa;
  UINTN    Sp;
  UINTN    Fp;
  UINTN    Ra;
  UINTN    Idx;
  CHAR8    *Pdb;
  CHAR8    *PrevPdb;

  RootRa = Regs->ra;
  RootFp = Regs->s0;

  Idx     = 0;
  IsLeaf  = TRUE;
  Fp      = RootFp;
  Ra      = RootRa;
  PrevPdb = FirstPdb;
  while (Fp != 0) {
    Pdb = GetImageName (Ra, &ImageBase, &PeCoffSizeOfHeader);
    if (Pdb != NULL) {
      if (Pdb != PrevPdb) {
        Idx++;
        if (ListImage) {
          DEBUG ((DEBUG_ERROR, "[% 2d] %a\n", Idx, Pdb));
        }

        PrevPdb = Pdb;
      }

      if (!ListImage) {
        DEBUG ((
          DEBUG_ERROR,
          "PC 0x%012lx (0x%012lx+0x%08x) [% 2d] %a\n",
          Ra,
          ImageBase,
          Ra - ImageBase,
          Idx,
          BaseName (Pdb)
          ));
      }
    } else if (!ListImage) {
      DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Ra));
    }

    /*
     * After the prologue, the frame pointer register s0 will point
     * to the Canonical Frame Address or CFA, which is the stack
     * pointer value on entry to the current procedure. The previous
     * frame pointer and return address pair will reside just prior
     * to the current stack address held in s0. This puts the return
     * address at s0 - XLEN/8, and the previous frame pointer at
     * s0 - 2 * XLEN/8.
     */
    Sp  = Fp;
    Fp -= sizeof (UINTN) * 2;
    Ra  = *(UINTN *)(Fp + sizeof (UINTN));
    Fp  = *(UINTN *)(Fp);
    if (IsLeaf && CheckFpValid (Ra, Sp)) {
      /* We hit function where ra is not saved on the stack */
      Fp = Ra;
      Ra = RootRa;
    }

    IsLeaf = FALSE;
  }
}

/**
  Display a backtrace.

  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
DumpCpuBacktrace (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  SMODE_TRAP_REGISTERS  *Regs;
  CHAR8                 *Pdb;
  UINTN                 ImageBase;
  UINTN                 PeCoffSizeOfHeader;

  Regs = (SMODE_TRAP_REGISTERS *)SystemContext.SystemContextRiscV64;
  Pdb  = GetImageName (Regs->sepc, &ImageBase, &PeCoffSizeOfHeader);
  if (Pdb != NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "PC 0x%012lx (0x%012lx+0x%08x) [ 0] %a\n",
      Regs->sepc,
      ImageBase,
      Regs->sepc - ImageBase,
      BaseName (Pdb)
      ));
  } else {
    DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Regs->sepc));
  }

  DumpCpuBacktraceHelper (Regs, Pdb, FALSE);

  if (Pdb != NULL) {
    DEBUG ((DEBUG_ERROR, "\n[ 0] %a\n", Pdb));
  }

  DumpCpuBacktraceHelper (Regs, Pdb, TRUE);
}
