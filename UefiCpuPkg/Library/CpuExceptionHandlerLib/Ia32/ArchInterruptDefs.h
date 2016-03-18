/** @file
  Ia32 arch definition for CPU Exception Handler Library.

  Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ARCH_CPU_INTERRUPT_DEFS_H_
#define _ARCH_CPU_INTERRUPT_DEFS_H_

typedef struct {
  EFI_SYSTEM_CONTEXT_IA32 SystemContext;
  BOOLEAN                 ExceptionDataFlag;
  UINTN                   OldIdtHandler;
} EXCEPTION_HANDLER_CONTEXT;

//
// Register Structure Definitions
//
typedef struct {
  EFI_STATUS_CODE_DATA      Header;
  EFI_SYSTEM_CONTEXT_IA32   SystemContext;
} CPU_STATUS_CODE_TEMPLATE;

typedef struct {
  SPIN_LOCK   SpinLock;
  UINT32      ApicId;
  UINT32      Attribute;
  UINTN       ExceptonHandler;
  UINTN       OldFlags;
  UINTN       OldCs;
  UINTN       OldIp;
  UINTN       ExceptionData;
  UINT8       HookAfterStubHeaderCode[HOOKAFTER_STUB_SIZE];
} RESERVED_VECTORS_DATA;

#endif
