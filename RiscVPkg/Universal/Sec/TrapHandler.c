/** @file
  RISC-V trap handler.

  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecMain.h"

/**
  RISC-V User mode trap handler.

**/
VOID
RiscVUserModeTrapHandler (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "Enter RISC-V User Mode Trap Handler.\n"));
  //while (TRUE);
}

/**
  RISC-V Supervisor mode trap handler.

**/
VOID
RiscVSupervisorModeTrapHandler (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "Enter RISC-V Supervisor Mode Trap Handler.\n"));
  //while (TRUE);
}

/**
  RISC-V Hypervisor mode trap handler.

**/
VOID
RiscVHypervisorModeTrapHandler (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "Enter RISC-V Hypervisor Mode Trap Handler.\n"));
  //while (TRUE);
}

/**
  RISC-V Machine mode trap handler.

**/
VOID
RiscVMachineModeTrapHandler (
  VOID
  )
{
  RISCV_TRAP_HANDLER TrapHandle;
  RISCV_MACHINE_MODE_CONTEXT *Context;

  //DEBUG ((EFI_D_INFO, "Enter RISC-V Machine Mode Trap Handler.\n"));
  Context = (RISCV_MACHINE_MODE_CONTEXT *)(UINTN)RiscVGetScratch ();
  TrapHandle = (RISCV_TRAP_HANDLER)(UINTN)Context->MachineModeTrapHandler;
  TrapHandle ();
}

/**
  RISC-V NMI trap handler.

**/
VOID
RiscVNmiHandler (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "Enter RISC-V NMI Trap Handler.\n"));
  //while (TRUE);
}

/**
  SEC RISC-V Machine mode trap handler.

**/
VOID
SecMachineModeTrapHandler (
  IN VOID
  )
{
  //DEBUG ((EFI_D_INFO, "SEC RISC-V Machine Mode Trap Handler.\n"));
  //while (TRUE);
}
