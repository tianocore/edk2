/** @file
  Ia32 arch definition for CPU Exception Handler Library.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ARCH_CPU_INTERRUPT_DEFS_H_
#define _ARCH_CPU_INTERRUPT_DEFS_H_

typedef struct {
  EFI_SYSTEM_CONTEXT_IA32    SystemContext;
  BOOLEAN                    ExceptionDataFlag;
  UINTN                      OldIdtHandler;
} EXCEPTION_HANDLER_CONTEXT;

//
// Register Structure Definitions
//
typedef struct {
  EFI_STATUS_CODE_DATA       Header;
  EFI_SYSTEM_CONTEXT_IA32    SystemContext;
} CPU_STATUS_CODE_TEMPLATE;

typedef struct {
  SPIN_LOCK    SpinLock;
  UINT32       ApicId;
  UINT32       Attribute;
  UINTN        ExceptonHandler;
  UINTN        OldFlags;
  UINTN        OldCs;
  UINTN        OldIp;
  UINTN        ExceptionData;
  UINT8        HookAfterStubHeaderCode[HOOKAFTER_STUB_SIZE];
} RESERVED_VECTORS_DATA;

#define CPU_TSS_DESC_SIZE \
  (sizeof (IA32_TSS_DESCRIPTOR) * \
   (FixedPcdGetSize (PcdCpuStackSwitchExceptionList) + 1))

#define CPU_TSS_SIZE \
  (sizeof (IA32_TASK_STATE_SEGMENT) * \
   (FixedPcdGetSize (PcdCpuStackSwitchExceptionList) + 1))

#endif
