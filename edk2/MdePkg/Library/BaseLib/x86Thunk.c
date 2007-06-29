/** @file
  Real Mode Thunk Functions for IA32 and X64.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  x86Thunk.c

**/

//
// Include common header file for this module.
//
#include <BaseLibInternals.h>


//
// Byte packed structure for a segment descriptor in a GDT/LDT
//
typedef union {
  struct {
    UINT32  LimitLow:16;
    UINT32  BaseLow:16;
    UINT32  BaseMid:8;
    UINT32  Type:4;
    UINT32  S:1;
    UINT32  DPL:2;
    UINT32  P:1;
    UINT32  LimitHigh:4;
    UINT32  AVL:1;
    UINT32  L:1;
    UINT32  DB:1;
    UINT32  G:1;
    UINT32  BaseHigh:8;
  } Bits;
  UINT64  Uint64;
} IA32_SEGMENT_DESCRIPTOR;

extern CONST UINT8                  m16Start;
extern CONST UINT16                 m16Size;
extern CONST UINT16                 mThunk16Attr;
extern CONST UINT16                 m16Gdt;
extern CONST UINT16                 m16GdtrBase;
extern CONST UINT16                 mTransition;

/**
  Invokes 16-bit code in big real mode and returns the updated register set.

  This function transfers control to the 16-bit code specified by CS:EIP using
  the stack specified by SS:ESP in RegisterSet. The updated registers are saved
  on the real mode stack and the starting address of the save area is returned.

  @param  RegisterSet Values of registers before invocation of 16-bit code.
  @param  Transition  Pointer to the transition code under 1MB.

  @return The pointer to a IA32_REGISTER_SET structure containing the updated
          register values.

**/
IA32_REGISTER_SET *
EFIAPI
InternalAsmThunk16 (
  IN      IA32_REGISTER_SET         *RegisterSet,
  IN OUT  VOID                      *Transition
  );

/**
  Retrieves the properties for 16-bit thunk functions.

  Computes the size of the buffer and stack below 1MB required to use the
  AsmPrepareThunk16(), AsmThunk16() and AsmPrepareAndThunk16() functions. This
  buffer size is returned in RealModeBufferSize, and the stack size is returned
  in ExtraStackSize. If parameters are passed to the 16-bit real mode code,
  then the actual minimum stack size is ExtraStackSize plus the maximum number
  of bytes that need to be passed to the 16-bit real mode code.

  If RealModeBufferSize is NULL, then ASSERT().
  If ExtraStackSize is NULL, then ASSERT().

  @param  RealModeBufferSize  A pointer to the size of the buffer below 1MB
                              required to use the 16-bit thunk functions.
  @param  ExtraStackSize      A pointer to the extra size of stack below 1MB
                              that the 16-bit thunk functions require for
                              temporary storage in the transition to and from
                              16-bit real mode.

**/
VOID
EFIAPI
AsmGetThunk16Properties (
  OUT     UINT32                    *RealModeBufferSize,
  OUT     UINT32                    *ExtraStackSize
  )
{
  ASSERT (RealModeBufferSize != NULL);
  ASSERT (ExtraStackSize != NULL);

  *RealModeBufferSize = m16Size;

  //
  // Extra 4 bytes for return address, and another 4 bytes for mode transition
  //
  *ExtraStackSize = sizeof (IA32_DWORD_REGS) + 8;
}

/**
  Prepares all structures a code required to use AsmThunk16().

  Prepares all structures and code required to use AsmThunk16().

  If ThunkContext is NULL, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmPrepareThunk16 (
  OUT     THUNK_CONTEXT             *ThunkContext
  )
{
  IA32_SEGMENT_DESCRIPTOR           *RealModeGdt;

  ASSERT (ThunkContext != NULL);
  ASSERT ((UINTN)ThunkContext->RealModeBuffer < 0x100000);
  ASSERT (ThunkContext->RealModeBufferSize >= m16Size);
  ASSERT ((UINTN)ThunkContext->RealModeBuffer + m16Size <= 0x100000);

  CopyMem (ThunkContext->RealModeBuffer, &m16Start, m16Size);

  //
  // Point RealModeGdt to the GDT to be used in transition
  //
  // RealModeGdt[0]: Reserved as NULL descriptor
  // RealModeGdt[1]: Code Segment
  // RealModeGdt[2]: Data Segment
  // RealModeGdt[3]: Call Gate
  //
  RealModeGdt = (IA32_SEGMENT_DESCRIPTOR*)(
                  (UINTN)ThunkContext->RealModeBuffer + m16Gdt);

  //
  // Update Code & Data Segment Descriptor
  //
  RealModeGdt[1].Bits.BaseLow =
    (UINT32)(UINTN)ThunkContext->RealModeBuffer & ~0xf;
  RealModeGdt[1].Bits.BaseMid =
    (UINT32)(UINTN)ThunkContext->RealModeBuffer >> 16;

  //
  // Update transition code entry point offset
  //
  *(UINT32*)((UINTN)ThunkContext->RealModeBuffer + mTransition) +=
    (UINT32)(UINTN)ThunkContext->RealModeBuffer & 0xf;

  //
  // Update Segment Limits for both Code and Data Segment Descriptors
  //
  if ((ThunkContext->ThunkAttributes & THUNK_ATTRIBUTE_BIG_REAL_MODE) == 0) {
    //
    // Set segment limits to 64KB
    //
    RealModeGdt[1].Bits.LimitHigh = 0;
    RealModeGdt[1].Bits.G = 0;
    RealModeGdt[2].Bits.LimitHigh = 0;
    RealModeGdt[2].Bits.G = 0;
  }

  //
  // Update GDTBASE for this thunk context
  //
  *(VOID**)((UINTN)ThunkContext->RealModeBuffer + m16GdtrBase) = RealModeGdt;

  //
  // Update Thunk Attributes
  //
  *(UINT32*)((UINTN)ThunkContext->RealModeBuffer + mThunk16Attr) =
    ThunkContext->ThunkAttributes;
}

/**
  Transfers control to a 16-bit real mode entry point and returns the results.

  Transfers control to a 16-bit real mode entry point and returns the results.
  AsmPrepareThunk16() must be called with ThunkContext before this function is
  used. This function must be called with interrupts disabled.

  If ThunkContext is NULL, then ASSERT().
  If AsmPrepareThunk16() was not previously called with ThunkContext, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmThunk16 (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  )
{
  IA32_REGISTER_SET                 *UpdatedRegs;

  ASSERT (ThunkContext != NULL);
  ASSERT ((UINTN)ThunkContext->RealModeBuffer < 0x100000);
  ASSERT (ThunkContext->RealModeBufferSize >= m16Size);
  ASSERT ((UINTN)ThunkContext->RealModeBuffer + m16Size <= 0x100000);

  UpdatedRegs = InternalAsmThunk16 (
                  ThunkContext->RealModeState,
                  ThunkContext->RealModeBuffer
                  );

  CopyMem (ThunkContext->RealModeState, UpdatedRegs, sizeof (*UpdatedRegs));
}

/**
  Prepares all structures and code for a 16-bit real mode thunk, transfers
  control to a 16-bit real mode entry point, and returns the results.

  Prepares all structures and code for a 16-bit real mode thunk, transfers
  control to a 16-bit real mode entry point, and returns the results. If the
  caller only need to perform a single 16-bit real mode thunk, then this
  service should be used. If the caller intends to make more than one 16-bit
  real mode thunk, then it is more efficient if AsmPrepareThunk16() is called
  once and AsmThunk16() can be called for each 16-bit real mode thunk. This
  function must be called with interrupts disabled.

  If ThunkContext is NULL, then ASSERT().

  @param  ThunkContext  A pointer to the context structure that describes the
                        16-bit real mode code to call.

**/
VOID
EFIAPI
AsmPrepareAndThunk16 (
  IN OUT  THUNK_CONTEXT             *ThunkContext
  )
{
  AsmPrepareThunk16 (ThunkContext);
  AsmThunk16 (ThunkContext);
}
