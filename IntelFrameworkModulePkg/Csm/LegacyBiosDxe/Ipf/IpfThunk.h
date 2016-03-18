/** @file

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IPF_THUNK_H_
#define _IPF_THUNK_H_

#include "LegacyBiosInterface.h"
#include <IndustryStandard/Sal.h>

/**
  Template of real mode code.

  @param  CodeStart          Start address of code.
  @param  CodeEnd            End address of code
  @param  ReverseThunkStart  Start of reverse thunk.
  @param  IntThunk           Low memory thunk.

**/
VOID
RealModeTemplate (
  OUT UINTN          *CodeStart,
  OUT UINTN          *CodeEnd,
  OUT UINTN          *ReverseThunkStart,
  LOW_MEMORY_THUNK   *IntThunk
  );

/**
  Register physical address of Esal Data Area

  @param  ReverseThunkCodeAddress Reverse Thunk Address
  @param  IntThunkAddress         IntThunk Address

  @retval EFI_SUCCESS             ESAL data area set successfully.

**/
EFI_STATUS
EsalSetSalDataArea (
  IN UINTN    ReverseThunkCodeAddress,
  IN UINTN    IntThunkAddress
  );

/**
 Get address of reverse thunk.

 @retval EFI_SAL_SUCCESS  Address of reverse thunk returned successfully.

**/
SAL_RETURN_REGS
EsalGetReverseThunkAddress (
  VOID
  );

typedef struct {
  UINT32  Eax;    // 0
  UINT32  Ecx;    // 4
  UINT32  Edx;    // 8
  UINT32  Ebx;    // 12
  UINT32  Esp;    // 16
  UINT32  Ebp;    // 20
  UINT32  Esi;    // 24
  UINT32  Edi;    // 28
  UINT32  Eflag;  // 32
  UINT32  Eip;    // 36
  UINT16  Cs;     // 40
  UINT16  Ds;     // 42
  UINT16  Es;     // 44
  UINT16  Fs;     // 46
  UINT16  Gs;     // 48
  UINT16  Ss;     // 50
} IPF_DWORD_REGS;

/**
  Entrypoint of IA32 code.

  @param  CallTypeData  Data of call type
  @param  DwordRegister Register set of IA32 general registers
                        and segment registers
  @param  StackPointer  Stack pointer.
  @param  StackSize     Size of stack.

**/
VOID
EfiIaEntryPoint (
  UINT64           CallTypeData,
  IPF_DWORD_REGS   *DwordRegister,
  UINT64           StackPointer,
  UINT64           StackSize
  );

#endif
