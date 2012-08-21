/**@file
  This file holds all the needed Xen Hypercalls.

  Copyright (c) 2011-2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>

#include <Library/XenHypercallLib.h>
#include <Library/XenLib.h>
#include "../GasketHypercall.h"

/**
  This method passes the hypercall parameters and hypercall number to UNIX ABI function, as well as the address of hypercall page.

  @param  HypercallNum    The Number of the hypercall.
  @param  Arg1-Arg5       Parameters for the hypercall.

**/
EFI_STATUS
EFIAPI
Hypercall2Abi (
  IN  UINTN  HypercallNum,
  IN  UINTN  Arg1,
  IN  UINTN  Arg2,
  IN  UINTN  Arg3,
  IN  UINTN  Arg4,
  IN  UINTN  Arg5
  )
{
  VOID *HypercallPages;

  HypercallPages = InitializeHypercallPage ();
  if (HypercallPages == NULL) {
    return 0;
  }

  //
  // All the hypercalls call this function.
  //
  return Hypercall (HypercallPages, HypercallNum * 32, Arg1, Arg2, Arg3, Arg4, Arg5);
}

/**
  Xen Hypercall. All the hypercalls call one function.

  @param  Cmd       parameter.
  @param  Arg       Parameter.

**/

/**
  Hypercall #0: HYPERVISOR_SET_TRAP_TABLE
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetTrapTable (
  IN OUT TRAP_INFO     *Table
  )
{
  return Hypercall2Abi (HYPERVISOR_SET_TRAP_TABLE, (UINTN) Table, 0, 0, 0, 0);
}

/**
  Hypercall #1: HYPERVISOR_MMU_UPDATE
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorMmuUpdate (
  IN  MMU_UPDATE    *Req,
  IN  UINTN         Count,
  OUT UINTN         *SuccessCount,
  IN  DOMID         DomId
  )
{
  return Hypercall2Abi (HYPERVISOR_MMU_UPDATE, (UINTN) Req, (UINTN) Count, (UINTN) SuccessCount, (UINTN) DomId, 0);
}

/**
  Hypercall #2: HYPERVISOR_SET_GDT
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetGdt (
  IN  UINTN         *FrameList,
  IN  UINTN         Entries
  )
{
  return Hypercall2Abi (HYPERVISOR_SET_GDT, (UINTN) FrameList, (UINTN) Entries, 0, 0, 0);
}

/**
  Hypercall #3: HYPERVISOR_STACK_SWITCH
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorStackSwitch (
  IN  UINTN         SS,
  IN  UINTN         Esp
  )
{
  return Hypercall2Abi (HYPERVISOR_STACK_SWITCH, (UINTN) SS, (UINTN) Esp, 0, 0, 0);
}

/**
  Hypercall #4: HYPERVISOR_SET_CALLBACKS (IA32/X64)
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetCallbacks (
  IN  UINTN         EventAddress,
  IN  UINTN         FailsafeAddress,
  IN  UINTN         SyscallAddress
  )
{
  return Hypercall2Abi (HYPERVISOR_SET_CALLBACKS, (UINTN) EventAddress, (UINTN) FailsafeAddress, (UINTN) SyscallAddress, 0, 0);
}

/**
  Hypercall #5: HYPERVISOR_FPU_TASKSWITCH
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorFpuTaskswitch (
  IN  UINTN         Set
  )
{
  return Hypercall2Abi (HYPERVISOR_FPU_TASKSWITCH, (UINTN) Set, 0, 0, 0, 0);
}

/**
  Hypercall #8: HYPERVISOR_SET_DEBUGREG
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetDebugreg (
  IN  UINTN         Reg,
  IN  UINTN         Value
  )
{
  return Hypercall2Abi (HYPERVISOR_SET_DEBUGREG, (UINTN) Reg, (UINTN) Value, 0, 0, 0);
}

/**
  Hypercall #9: HYPERVISOR_GET_DEBUGREG
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorGetDebugreg (
  IN  UINTN         Reg
  )
{
  return Hypercall2Abi (HYPERVISOR_GET_DEBUGREG, (UINTN) Reg, 0, 0, 0, 0);
}

/**
  Hypercall #10: HYPERVISOR_UPDATE_DESCRIPTOR (IA32/X64)
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorUpdateDescriptor (
  IN  UINTN         Ma,
  IN  UINTN         Word
  )
{
  return Hypercall2Abi (HYPERVISOR_UPDATE_DESCRIPTOR, (UINTN) Ma, (UINTN) Word, 0, 0, 0);
}

/**
  Hypercall #12: HYPERVISOR_MEMORY_OP
**/
EFI_STATUS
EFIAPI
HypervisorMemoryOp (
  IN UINTN      Cmd,
  IN VOID       *Arg
  )
{
  return Hypercall2Abi (HYPERVISOR_MEMORY_OP, (UINTN) Cmd, (UINTN) Arg, 0, 0, 0);
}

/**
  Hypercall #13: HYPERVISOR_MULTICALL
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorMulticall (
  IN  VOID          *CallList,
  IN  UINTN         NrCalls
  )
{
  return Hypercall2Abi (HYPERVISOR_MULTICALL, (UINTN) CallList, (UINTN) NrCalls, 0, 0, 0);
}

/**
  Hypercall #14: HYPERVISOR_UPDATE_VA_MAPPING (IA32/X64)
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorUpdateVaMapping (
  IN  UINTN         Va,
  IN  PTE           NewVal,
  IN  UINTN         Flags
  )
{
  return Hypercall2Abi (HYPERVISOR_UPDATE_VA_MAPPING, (UINTN) Va, (UINTN) NewVal.Pte, (UINTN) Flags, 0, 0);
}

/**
  Hypercall #15: HYPERVISOR_SET_TIMER_OP (IA32/X64)
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetTimerOp (
  IN UINT64     Timeout
  )
{
  return Hypercall2Abi (HYPERVISOR_SET_TIMER_OP, (UINTN) Timeout, 0, 0, 0, 0);
}

/**
  Hypercall #17: HYPERVISOR_XEN_VERSION
**/
EFI_STATUS
EFIAPI
HypervisorXenVersion (
  IN UINTN      Cmd,
  IN VOID       *Arg
  )
{
  return Hypercall2Abi (HYPERVISOR_XEN_VERSION, (UINTN) Cmd, (UINTN) Arg, 0, 0, 0);
}

/**
  Hypercall #18: HYPERVISOR_CONSOLE_IO
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorConsoleIo (
  IN  UINTN         Cmd,
  IN  UINTN         Count,
  OUT CHAR8         *Str
  )
{
  return Hypercall2Abi (HYPERVISOR_CONSOLE_IO, (UINTN) Cmd, (UINTN) Count, (UINTN) Str, 0, 0);
}

/**
  Hypercall #20: HYPERVISOR_GRANT_TABLE_OP
**/
EFI_STATUS
EFIAPI
HypervisorGrantTableOp(
  IN UINTN      Cmd,
  IN VOID       *Uop,
  IN UINTN      Count
)
{
  return Hypercall2Abi (HYPERVISOR_GRANT_TABLE_OP, (UINTN) Cmd, (UINTN) Uop, (UINTN) Count, 0, 0);
}

/**
  Hypercall #21: HYPERVISOR_VM_ASSIST
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorVmAssist (
  IN  UINTN         Cmd,
  IN  UINTN         Type
  )
{
  return Hypercall2Abi(HYPERVISOR_VM_ASSIST, (UINTN) Cmd, (UINTN) Type, 0, 0, 0);
}

/**
  Hypercall #22: HYPERVISOR_UPDATE_VA_MAPPING_OTHERDOMAIN (IA32/X64)
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorUpdateVaMappingOtherdomain (
  IN  UINTN         Va,
  IN  PTE           NewVal,
  IN  UINTN         Flags,
  IN  DOMID         DomId
  )
{
  return Hypercall2Abi (HYPERVISOR_UPDATE_VA_MAPPING_OTHERDOMAIN, (UINTN) Va, (UINTN) NewVal.Pte, (UINTN) Flags, (UINTN) DomId, 0);
}

/**
  Hypercall #24: HYPERVISOR_VCPU_OP
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorVcpuOp (
  IN  UINTN         Cmd,
  IN  UINTN         VcpuId,
  IN  VOID          *ExtraArgs
  )
{
  return Hypercall2Abi(HYPERVISOR_VCPU_OP, (UINTN) Cmd, (UINTN) VcpuId, (UINTN) ExtraArgs, 0, 0);
}

/**
  Hypercall #25: HYPERVISOR_SET_SEGMENT_BASE (IA32/X64)
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetSegmentBase (
  IN  UINTN         Reg,
  IN  UINTN         Value
  )
{
  return Hypercall2Abi(HYPERVISOR_SET_SEGMENT_BASE, (UINTN) Reg, (UINTN) Value, 0, 0, 0);
}

/**
  Hypercall #26: HYPERVISOR_MMUEXT_OP
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorMmuextOp (
  IN  MMUEXT_OP     *Op,
  IN  UINTN         Count,
  OUT UINTN         *SuccessCount,
  IN  DOMID         DomId
  )
{
  return Hypercall2Abi (HYPERVISOR_MMUEXT_OP, (UINTN) Op, (UINTN) Count, (UINTN) SuccessCount, (UINTN) DomId, 0);
}

/**
  Hypercall #28: HYPERVISOR_NMI_OP
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorNmiOp (
  IN  UINTN         Cmd,
  IN  UINTN         Arg
  )
{
  return Hypercall2Abi(HYPERVISOR_NMI_OP, (UINTN) Cmd, (UINTN) Arg, 0, 0, 0);
}

/**
  Hypercall #29: HYPERVISOR_SCHED_OP
**/
EFI_STATUS
EFIAPI
HypervisorSchedOp (
  IN UINTN      Cmd,
  IN VOID       *Arg
  )
{
  return Hypercall2Abi(HYPERVISOR_SCHED_OP, (UINTN) Cmd, (UINTN) Arg, 0, 0, 0);
}

/**
  Hypercall #29-2: HYPERVISOR_SCHED_OP
**/
EFI_STATUS
EFIAPI
HypervisorSuspend (
  IN UINTN      Srec
  )
{
  return Hypercall2Abi(HYPERVISOR_SCHED_OP, (UINTN) SCHEDOP_SHUTDOWN, (UINTN) SHUTDOWN_SUSPEND, (UINTN) Srec, 0, 0);
}

/**
  Hypercall #32: HYPERVISOR_EVENT_CHANNEL_OP
**/
EFI_STATUS
EFIAPI
HypervisorEventChannelOp (
  IN UINTN      Cmd,
  IN VOID       *Arg
  )
{
  return Hypercall2Abi(HYPERVISOR_EVENT_CHANNEL_OP, (UINTN) Cmd, (UINTN) Arg, 0, 0, 0);
}

/**
  Hypercall #33: HYPERVISOR_PHYSDEV_OP 
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorPhysdevOp (
  IN VOID       *Arg
  )
{
  return Hypercall2Abi(HYPERVISOR_PHYSDEV_OP, (UINTN) Arg, 0, 0, 0, 0);
}

/**
  Hypercall #34: HYPERVISOR_HVM_OP
**/
EFI_STATUS
EFIAPI
HypervisorHvmOp (
  IN UINTN      Cmd,
  IN VOID       *Arg
  )
{
  return Hypercall2Abi (HYPERVISOR_HVM_OP, (UINTN) Cmd, (UINTN) Arg, 0, 0, 0);
}

/**
  Hypercall #35: HYPERVISOR_SYSCTL
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSysctl (
  IN UINTN      Op
  )
{
  return Hypercall2Abi (HYPERVISOR_SYSCTL, (UINTN) Op, 0, 0, 0, 0);
}

/**
  Hypercall #36: HYPERVISOR_DOMCTL
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorDomctl (
  IN UINTN      Op
  )
{
  return Hypercall2Abi (HYPERVISOR_DOMCTL, (UINTN) Op, 0, 0, 0, 0);
}

/*
  Add more hypercall here

**/

