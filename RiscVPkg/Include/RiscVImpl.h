/** @file
  RISC-V package definitions.

  Copyright (c) 2016 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISCV_H_
#define RISCV_H_

#include <Uefi.h>
#include <IndustryStandard/RiscV.h>

#define _ASM_FUNC(Name, Section)    \
  .global   Name                  ; \
  .section  #Section, "ax"        ; \
  .type     Name, %function       ; \
  .p2align  2                     ; \
  Name:

#define ASM_FUNC(Name)  _ASM_FUNC(ASM_PFX(Name), .text. ## Name)

#if defined (MDE_CPU_RISCV64)
typedef UINT64 RISC_V_REGS_PROTOTYPE;
#else
#endif

//
// Structure for 128-bit value
//
typedef struct {
  UINT64    Value64_L;
  UINT64    Value64_H;
} RISCV_UINT128;

#define RISCV_MACHINE_CONTEXT_SIZE  0x1000
typedef struct _RISCV_MACHINE_MODE_CONTEXT RISCV_MACHINE_MODE_CONTEXT;

///
/// Exception handlers in context.
///
typedef struct _EXCEPTION_HANDLER_CONTEXT {
  EFI_PHYSICAL_ADDRESS    InstAddressMisalignedHander;
  EFI_PHYSICAL_ADDRESS    InstAccessFaultHander;
  EFI_PHYSICAL_ADDRESS    IllegalInstHander;
  EFI_PHYSICAL_ADDRESS    BreakpointHander;
  EFI_PHYSICAL_ADDRESS    LoadAddrMisalignedHander;
  EFI_PHYSICAL_ADDRESS    LoadAccessFaultHander;
  EFI_PHYSICAL_ADDRESS    StoreAmoAddrMisalignedHander;
  EFI_PHYSICAL_ADDRESS    StoreAmoAccessFaultHander;
  EFI_PHYSICAL_ADDRESS    EnvCallFromUModeHander;
  EFI_PHYSICAL_ADDRESS    EnvCallFromSModeHander;
  EFI_PHYSICAL_ADDRESS    EnvCallFromHModeHander;
  EFI_PHYSICAL_ADDRESS    EnvCallFromMModeHander;
} EXCEPTION_HANDLER_CONTEXT;

///
/// Exception handlers in context.
///
typedef struct _INTERRUPT_HANDLER_CONTEXT {
  EFI_PHYSICAL_ADDRESS    SoftwareIntHandler;
  EFI_PHYSICAL_ADDRESS    TimerIntHandler;
} INTERRUPT_HANDLER_CONTEXT;

///
/// Interrupt handlers in context.
///
typedef struct _TRAP_HANDLER_CONTEXT {
  EXCEPTION_HANDLER_CONTEXT    ExceptionHandlerContext;
  INTERRUPT_HANDLER_CONTEXT    IntHandlerContext;
} TRAP_HANDLER_CONTEXT;

///
/// Machine mode context used for saveing hart-local context.
///
typedef struct _RISCV_MACHINE_MODE_CONTEXT {
  EFI_PHYSICAL_ADDRESS    PeiService;                /// PEI service.
  EFI_PHYSICAL_ADDRESS    MachineModeTrapHandler;    /// Machine mode trap handler.
  EFI_PHYSICAL_ADDRESS    HypervisorModeTrapHandler; /// Hypervisor mode trap handler.
  EFI_PHYSICAL_ADDRESS    SupervisorModeTrapHandler; /// Supervisor mode trap handler.
  EFI_PHYSICAL_ADDRESS    UserModeTrapHandler;       /// USer mode trap handler.
  TRAP_HANDLER_CONTEXT    MModeHandler;              /// Handler for machine mode.
} RISCV_MACHINE_MODE_CONTEXT;

#endif
