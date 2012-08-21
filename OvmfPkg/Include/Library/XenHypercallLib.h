/** @file

  This XenHypercallLib library provides consumers all Xen Hypercall functions.

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __XEN_HYPERCALL_LIB__
#define __XEN_HYPERCALL_LIB__

#include <Library/XenLib.h>

//
// HYPERCALLS NUMBER
//
#define HYPERVISOR_SET_TRAP_TABLE          0
#define HYPERVISOR_MMU_UPDATE              1
#define HYPERVISOR_SET_GDT                 2
#define HYPERVISOR_STACK_SWITCH            3
#define HYPERVISOR_SET_CALLBACKS           4
#define HYPERVISOR_FPU_TASKSWITCH          5
#define HYPERVISOR_SCHED_OP_COMPAT         6
#define HYPERVISOR_PLATFORM_OP             7
#define HYPERVISOR_SET_DEBUGREG            8
#define HYPERVISOR_GET_DEBUGREG            9
#define HYPERVISOR_UPDATE_DESCRIPTOR       10
#define HYPERVISOR_MEMORY_OP               12
#define HYPERVISOR_MULTICALL               13
#define HYPERVISOR_UPDATE_VA_MAPPING       14
#define HYPERVISOR_SET_TIMER_OP            15
#define HYPERVISOR_EVENT_CHANNEL_OP_COMPAT 16
#define HYPERVISOR_XEN_VERSION             17
#define HYPERVISOR_CONSOLE_IO              18
#define HYPERVISOR_PHYSDEV_OP_COMPAT       19
#define HYPERVISOR_GRANT_TABLE_OP          20
#define HYPERVISOR_VM_ASSIST               21
#define HYPERVISOR_UPDATE_VA_MAPPING_OTHERDOMAIN 22
#define HYPERVISOR_IRET                    23
#define HYPERVISOR_VCPU_OP                 24
#define HYPERVISOR_SET_SEGMENT_BASE        25
#define HYPERVISOR_MMUEXT_OP               26
#define HYPERVISOR_XSM_OP                  27
#define HYPERVISOR_NMI_OP                  28
#define HYPERVISOR_SCHED_OP                29
#define HYPERVISOR_CALLBACK_OP             30
#define HYPERVISOR_XENOPROF_OP             31
#define HYPERVISOR_EVENT_CHANNEL_OP        32
#define HYPERVISOR_PHYSDEV_OP              33
#define HYPERVISOR_HVM_OP                  34
#define HYPERVISOR_SYSCTL                  35
#define HYPERVISOR_DOMCTL                  36
#define HYPERVISOR_KEXEC_OP                37
#define HYPERVISOR_TMEM_OP                 38

#define HYPERVISOR_ARCH_0                  48
#define HYPERVISOR_ARCH_1                  49
#define HYPERVISOR_ARCH_2                  50
#define HYPERVISOR_ARCH_3                  51
#define HYPERVISOR_ARCH_4                  52
#define HYPERVISOR_ARCH_5                  53
#define HYPERVISOR_ARCH_6                  54
#define HYPERVISOR_ARCH_7                  55

///
/// Structs for Hypercall parameters.
///

typedef struct {
  //
  // Exception vector
  //
  UINT8        Vector;
  //
  // 0-3: privilege level; 4: clear event enable?
  //
  UINT8        Flags;
  //
  // Code selector
  //
  UINT16       CS;
  //
  // Code offset
  //
  UINTN        Address;
} TRAP_INFO;

typedef struct {
  //
  // Machine address of PTE (Page Table Entry)
  //
  UINT64       Ptr;    //UINTN?
  //
  // New contents of PTE
  //
  UINT64       Val;   //UINTN?
} MMU_UPDATE;

typedef struct {
  UINT32       Cmd;
  union {
    XEN_PFN      Mfn;
    UINTN        LinearAddr;
  } Arg1;
  union {
    UINT32       NrEnts;
    //
    // This item has something to do with xen interface version.
    //
    const VOID   *Vcpumask;

    XEN_PFN      SrcMfn;
  } Arg2;
} MMUEXT_OP;

//
// Definitions for architecture-specific types
//
#if defined (MDE_CPU_IA32)
typedef struct {
    UINTN        PteLow;
    UINTN        PteHigh;
} PTE;
#endif

#if defined (MDE_CPU_X64)
typedef struct {
    UINTN        Pte;
} PTE;
#endif

/**
  Hypercall methods.

  @param[in]    TODO parameters description. Make clear of IN and OUT of the parameters

**/
/**
  Hypercall #0: HYPERVISOR_SET_TRAP_TABLE
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetTrapTable (
  IN OUT TRAP_INFO     *Table
  );

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
  );

/**
  Hypercall #2: HYPERVISOR_SET_GDT
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetGdt (
  IN  UINTN         *FrameList,
  IN  UINTN         Entries
  );

/**
  Hypercall #3: HYPERVISOR_STACK_SWITCH
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorStackSwitch (
  IN  UINTN         SS,
  IN  UINTN         Esp
  );

/**
  Hypercall #4: HYPERVISOR_SET_CALLBACKS
  TODO Untested
**/
#if defined (MDE_CPU_IA32)
EFI_STATUS
EFIAPI
HypervisorSetCallbacks (
  IN  UINTN         EventSelector,
  IN  UINTN         EventAddress,
  IN  UINTN         FailsafeSelector,
  IN  UINTN         FailsafeAddress
  );
#endif

#if defined (MDE_CPU_X64)
EFI_STATUS
EFIAPI
HypervisorSetCallbacks (
  IN  UINTN         EventAddress,
  IN  UINTN         FailsafeAddress,
  IN  UINTN         SyscallAddress
  );
#endif

/**
  Hypercall #5: HYPERVISOR_FPU_TASKSWITCH
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorFpuTaskswitch (
  IN  UINTN         Set
  );

/**
  Hypercall #6: HYPERVISOR_SCHED_OP_COMPAT
**/
EFI_STATUS
EFIAPI
HypervisorSchedOpCompat (
  //TODO Add parameters
  );

/**
  Hypercall #7: HYPERVISOR_PLATFORM_OP
**/
EFI_STATUS
EFIAPI
HypervisorPlatformOp (
  //TODO Add parameters
  );

/**
  Hypercall #8: HYPERVISOR_SET_DEBUGREG
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetDebugreg (
  IN  UINTN         Reg,
  IN  UINTN         Value
  );

/**
  Hypercall #9: HYPERVISOR_GET_DEBUGREG
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorGetDebugreg (
  IN  UINTN         Reg
  );

/**
  Hypercall #10: HYPERVISOR_UPDATE_DESCRIPTOR
  TODO Untested
**/
#if defined (MDE_CPU_IA32)
EFI_STATUS
EFIAPI
HypervisorUpdateDescriptor (
  IN  UINT64        Ma,
  IN  UINT64        Desc
  );
#endif

#if defined (MDE_CPU_X64)
EFI_STATUS
EFIAPI
HypervisorUpdateDescriptor (
  IN  UINTN         Ma,
  IN  UINTN         Word
  );
#endif

/**
  Hypercall #12: HYPERVISOR_MEMORY_OP
**/
EFI_STATUS
EFIAPI
HypervisorMemoryOp (
  IN  UINTN         Cmd,
  IN  VOID          *Arg
  );

/**
  Hypercall #13: HYPERVISOR_MULTICALL
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorMulticall (
  IN  VOID          *CallList,
  IN  UINTN         NrCalls
  );

/**
  Hypercall #14: HYPERVISOR_UPDATE_VA_MAPPING
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorUpdateVaMapping (
  IN  UINTN         Va,
  IN  PTE           NewVal,
  IN  UINTN         Flags
  );

/**
  Hypercall #15: HYPERVISOR_SET_TIMER_OP
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSetTimerOp (
  IN UINT64     Timeout
  );

/**
  Hypercall #16: HYPERVISOR_EVENT_CHANNEL_OP_COMPAT
**/
EFI_STATUS
EFIAPI
HypervisorEventChannelOpCompat (
  //TODO Add parameters
  );

/**
  Hypercall #17: HYPERVISOR_XEN_VERSION
**/
EFI_STATUS
EFIAPI
HypervisorXenVersion (
  IN UINTN Cmd,
  IN VOID  *Arg
  );

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
  );

/**
  Hypercall #19: HYPERVISOR_PHYSDEV_OP_COMPAT
**/
EFI_STATUS
EFIAPI
HypervisorPhysdevOpCompat (
  //TODO Add parameters
  );

/**
  Hypercall #20: HYPERVISOR_GRANT_TABLE_OP
**/
EFI_STATUS
EFIAPI
HypervisorGrantTableOp(
  IN UINTN      Cmd,
  IN VOID       *Uop,
  IN UINTN      Count
);

/**
  Hypercall #21: HYPERVISOR_VM_ASSIST
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorVmAssist (
  IN  UINTN         Cmd,
  IN  UINTN         Type
  );

/**
  Hypercall #22: HYPERVISOR_UPDATE_VA_MAPPING_OTHERDOMAIN
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorUpdateVaMappingOtherdomain (
  IN  UINTN         Va,
  IN  PTE           NewVal,
  IN  UINTN         Flags,
  IN  DOMID         DomId
  );

/**
  Hypercall #23: HYPERVISOR_IRET
**/
EFI_STATUS
EFIAPI
HypervisorIret (
  //TODO Add parameters
  );

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
  );

/**
  Hypercall #25: HYPERVISOR_SET_SEGMENT_BASE
  TODO Untested
**/
#if defined (MDE_CPU_X64)
EFI_STATUS
EFIAPI
HypervisorSetSegmentBase (
  IN  UINTN         Reg,
  IN  UINTN         Value
  );
#endif

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
  );

/**
  Hypercall #27: HYPERVISOR_XSM_OP
**/
EFI_STATUS
EFIAPI
HypervisorXsmOp (
  //TODO Add parameters
  );

/**
  Hypercall #28: HYPERVISOR_NMI_OP
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorNmiOp (
  IN  UINTN         Cmd,
  IN  UINTN         Arg
  );

/**
  Hypercall #29: HYPERVISOR_SCHED_OP
**/
EFI_STATUS
EFIAPI
HypervisorSchedOp (
  IN UINTN          Cmd,
  IN VOID           *Arg
  );

/**
  Hypercall #29-2: HYPERVISOR_SCHED_OP
**/
EFI_STATUS
EFIAPI
HypervisorSuspend (
  IN UINTN      Srec
  );

/**
  Hypercall #30: HYPERVISOR_CALLBACK_OP
**/
EFI_STATUS
EFIAPI
HypervisorCallbackOp (
  //TODO Add parameters
  );

/**
  Hypercall #31: HYPERVISOR_XENOPROF_OP
**/
EFI_STATUS
EFIAPI
HypervisorXenoprofOp (
  //TODO Add parameters
  );

/**
  Hypercall #32: HYPERVISOR_EVENT_CHANNEL_OP
**/
EFI_STATUS
EFIAPI
HypervisorEventChannelOp (
  IN UINTN     Cmd,
  IN VOID      *Arg
  );

/**
  Hypercall #33: HYPERVISOR_PHYSDEV_OP 
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorPhysdevOp (
  IN VOID       *Arg
  );

/**
  Hypercall #34: HYPERVISOR_HVM_OP
**/
EFI_STATUS
EFIAPI
HypervisorHvmOp (
  IN UINTN      Cmd,
  IN VOID       *Arg
  );

/**
  Hypercall #35: HYPERVISOR_SYSCTL
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorSysctl (
  IN UINTN      Op
  );

/**
  Hypercall #36: HYPERVISOR_DOMCTL
  TODO Untested
**/
EFI_STATUS
EFIAPI
HypervisorDomctl (
  IN UINTN      Op
  );

/**
  Hypercall #37: HYPERVISOR_KEXEC_OP
**/
EFI_STATUS
EFIAPI
HypervisorKexecOp (
  //TODO Add parameters
  );

/**
  Hypercall #38: HYPERVISOR_TMEM_OP
**/
EFI_STATUS
EFIAPI
HypervisorTmemOp (
  //TODO Add parameters
  );

//
// Architecture-specific hypercall definitions.
//
/**
  Hypercall #48: HYPERVISOR_ARCH_0
**/
EFI_STATUS
EFIAPI
HypervisorArch0 (
  //TODO Add parameters
  );

/**
  Hypercall #49: HYPERVISOR_ARCH_1
**/
EFI_STATUS
EFIAPI
HypervisorArch1 (
  //TODO Add parameters
  );

/**
  Hypercall #50: HYPERVISOR_ARCH_2
**/
EFI_STATUS
EFIAPI
HypervisorArch2 (
  //TODO Add parameters
  );

/**
  Hypercall #51: HYPERVISOR_ARCH_3
**/
EFI_STATUS
EFIAPI
HypervisorArch3 (
  //TODO Add parameters
  );

/**
  Hypercall #52: HYPERVISOR_ARCH_4
**/
EFI_STATUS
EFIAPI
HypervisorArch4 (
  //TODO Add parameters
  );

/**
  Hypercall #53: HYPERVISOR_ARCH_5
**/
EFI_STATUS
EFIAPI
HypervisorArch5 (
  //TODO Add parameters
  );

/**
  Hypercall #54: HYPERVISOR_ARCH_6
**/
EFI_STATUS
EFIAPI
HypervisorArch6 (
  //TODO Add parameters
  );

/**
  Hypercall #55: HYPERVISOR_ARCH_7
**/
EFI_STATUS
EFIAPI
HypervisorArch7 (
  //TODO Add parameters
  );

/**
  Initialize the hypercalls: Get Xen Hypercall page from HOB.
  
  @param[out]   A pointer to hypercall page.

**/
VOID *
InitializeHypercallPage (
  VOID
  );

#endif  //__XEN_HYPERCALL_LIB__

