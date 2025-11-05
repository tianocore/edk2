/** @file
  X64 arch definition for CPU Exception Handler Library.

  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ARCH_CPU_INTERRUPT_DEFS_H_
#define _ARCH_CPU_INTERRUPT_DEFS_H_

typedef struct {
  EFI_SYSTEM_CONTEXT_X64    SystemContext;
  BOOLEAN                   ExceptionDataFlag;
  UINTN                     OldIdtHandler;
} EXCEPTION_HANDLER_CONTEXT;

//
// Register Structure Definitions
//
typedef struct {
  EFI_STATUS_CODE_DATA      Header;
  EFI_SYSTEM_CONTEXT_X64    SystemContext;
} CPU_STATUS_CODE_TEMPLATE;

typedef struct {
  SPIN_LOCK    SpinLock;
  UINT32       ApicId;
  UINT32       Attribute;
  UINTN        ExceptonHandler;
  UINTN        OldSs;
  UINTN        OldSp;
  UINTN        OldFlags;
  UINTN        OldCs;
  UINTN        OldIp;
  UINTN        ExceptionData;
  UINT8        HookAfterStubHeaderCode[HOOKAFTER_STUB_SIZE];
} RESERVED_VECTORS_DATA;

#define CPU_TSS_DESC_SIZE  sizeof (IA32_TSS_DESCRIPTOR)
#define CPU_TSS_SIZE       sizeof (IA32_TASK_STATE_SEGMENT)

#endif
