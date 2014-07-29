//
// Copyright (c) 2011 - 2014 ARM LTD. All rights reserved.<BR>
// Portion of Copyright (c) 2014 NVIDIA Corporation. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

#include <Library/PcdLib.h>
#include <AsmMacroIoLibV8.h>

/*
  This is the stack constructed by the exception handler (low address to high address).
  X0 to FAR makes up the EFI_SYSTEM_CONTEXT for AArch64.

  UINT64  X0;     0x000
  UINT64  X1;     0x008
  UINT64  X2;     0x010
  UINT64  X3;     0x018
  UINT64  X4;     0x020
  UINT64  X5;     0x028
  UINT64  X6;     0x030
  UINT64  X7;     0x038
  UINT64  X8;     0x040
  UINT64  X9;     0x048
  UINT64  X10;    0x050
  UINT64  X11;    0x058
  UINT64  X12;    0x060
  UINT64  X13;    0x068
  UINT64  X14;    0x070
  UINT64  X15;    0x078
  UINT64  X16;    0x080
  UINT64  X17;    0x088
  UINT64  X18;    0x090
  UINT64  X19;    0x098
  UINT64  X20;    0x0a0
  UINT64  X21;    0x0a8
  UINT64  X22;    0x0b0
  UINT64  X23;    0x0b8
  UINT64  X24;    0x0c0
  UINT64  X25;    0x0c8
  UINT64  X26;    0x0d0
  UINT64  X27;    0x0d8
  UINT64  X28;    0x0e0
  UINT64  FP;     0x0e8   // x29 - Frame Pointer
  UINT64  LR;     0x0f0   // x30 - Link Register
  UINT64  SP;     0x0f8   // x31 - Stack Pointer

  // FP/SIMD Registers. 128bit if used as Q-regs.
  UINT64  V0[2];  0x100
  UINT64  V1[2];  0x110
  UINT64  V2[2];  0x120
  UINT64  V3[2];  0x130
  UINT64  V4[2];  0x140
  UINT64  V5[2];  0x150
  UINT64  V6[2];  0x160
  UINT64  V7[2];  0x170
  UINT64  V8[2];  0x180
  UINT64  V9[2];  0x190
  UINT64  V10[2]; 0x1a0
  UINT64  V11[2]; 0x1b0
  UINT64  V12[2]; 0x1c0
  UINT64  V13[2]; 0x1d0
  UINT64  V14[2]; 0x1e0
  UINT64  V15[2]; 0x1f0
  UINT64  V16[2]; 0x200
  UINT64  V17[2]; 0x210
  UINT64  V18[2]; 0x220
  UINT64  V19[2]; 0x230
  UINT64  V20[2]; 0x240
  UINT64  V21[2]; 0x250
  UINT64  V22[2]; 0x260
  UINT64  V23[2]; 0x270
  UINT64  V24[2]; 0x280
  UINT64  V25[2]; 0x290
  UINT64  V26[2]; 0x2a0
  UINT64  V27[2]; 0x2b0
  UINT64  V28[2]; 0x2c0
  UINT64  V29[2]; 0x2d0
  UINT64  V30[2]; 0x2e0
  UINT64  V31[2]; 0x2f0

  // System Context
  UINT64  ELR;    0x300   // Exception Link Register
  UINT64  SPSR;   0x308   // Saved Processor Status Register
  UINT64  FPSR;   0x310   // Floating Point Status Register
  UINT64  ESR;    0x318   // Exception syndrome register
  UINT64  FAR;    0x320   // Fault Address Register
  UINT64  Padding;0x328   // Required for stack alignment
*/

GCC_ASM_EXPORT(ExceptionHandlersStart)
GCC_ASM_EXPORT(ExceptionHandlersEnd)
GCC_ASM_EXPORT(CommonExceptionEntry)
GCC_ASM_EXPORT(AsmCommonExceptionEntry)
GCC_ASM_EXPORT(CommonCExceptionHandler)

.text
.align 11

#define GP_CONTEXT_SIZE    (32 *  8)
#define FP_CONTEXT_SIZE    (32 * 16)
#define SYS_CONTEXT_SIZE   ( 6 *  8) // 5 SYS regs + Alignment requirement (ie: the stack must be aligned on 0x10)

// Cannot str x31 directly
#define ALL_GP_REGS                                     \
        REG_PAIR (x0,  x1,  0x000, GP_CONTEXT_SIZE);    \
        REG_PAIR (x2,  x3,  0x010, GP_CONTEXT_SIZE);    \
        REG_PAIR (x4,  x5,  0x020, GP_CONTEXT_SIZE);    \
        REG_PAIR (x6,  x7,  0x030, GP_CONTEXT_SIZE);    \
        REG_PAIR (x8,  x9,  0x040, GP_CONTEXT_SIZE);    \
        REG_PAIR (x10, x11, 0x050, GP_CONTEXT_SIZE);    \
        REG_PAIR (x12, x13, 0x060, GP_CONTEXT_SIZE);    \
        REG_PAIR (x14, x15, 0x070, GP_CONTEXT_SIZE);    \
        REG_PAIR (x16, x17, 0x080, GP_CONTEXT_SIZE);    \
        REG_PAIR (x18, x19, 0x090, GP_CONTEXT_SIZE);    \
        REG_PAIR (x20, x21, 0x0a0, GP_CONTEXT_SIZE);    \
        REG_PAIR (x22, x23, 0x0b0, GP_CONTEXT_SIZE);    \
        REG_PAIR (x24, x25, 0x0c0, GP_CONTEXT_SIZE);    \
        REG_PAIR (x26, x27, 0x0d0, GP_CONTEXT_SIZE);    \
        REG_PAIR (x28, x29, 0x0e0, GP_CONTEXT_SIZE);    \
        REG_ONE  (x30,      0x0f0, GP_CONTEXT_SIZE);

// In order to save the SP we need to put it somewhere else first.
// STR only works with XZR/WZR directly
#define SAVE_SP \
        add x1, sp, #(FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE); \
        REG_ONE (x1,        0x0f8, GP_CONTEXT_SIZE);

#define ALL_FP_REGS                                     \
        REG_PAIR (q0,  q1,  0x000, FP_CONTEXT_SIZE);    \
        REG_PAIR (q2,  q3,  0x020, FP_CONTEXT_SIZE);    \
        REG_PAIR (q4,  q5,  0x040, FP_CONTEXT_SIZE);    \
        REG_PAIR (q6,  q7,  0x060, FP_CONTEXT_SIZE);    \
        REG_PAIR (q8,  q9,  0x080, FP_CONTEXT_SIZE);    \
        REG_PAIR (q10, q11, 0x0a0, FP_CONTEXT_SIZE);    \
        REG_PAIR (q12, q13, 0x0c0, FP_CONTEXT_SIZE);    \
        REG_PAIR (q14, q15, 0x0e0, FP_CONTEXT_SIZE);    \
        REG_PAIR (q16, q17, 0x100, FP_CONTEXT_SIZE);    \
        REG_PAIR (q18, q19, 0x120, FP_CONTEXT_SIZE);    \
        REG_PAIR (q20, q21, 0x140, FP_CONTEXT_SIZE);    \
        REG_PAIR (q22, q23, 0x160, FP_CONTEXT_SIZE);    \
        REG_PAIR (q24, q25, 0x180, FP_CONTEXT_SIZE);    \
        REG_PAIR (q26, q27, 0x1a0, FP_CONTEXT_SIZE);    \
        REG_PAIR (q28, q29, 0x1c0, FP_CONTEXT_SIZE);    \
        REG_PAIR (q30, q31, 0x1e0, FP_CONTEXT_SIZE);

#define ALL_SYS_REGS                                    \
        REG_PAIR (x1,  x2,  0x000, SYS_CONTEXT_SIZE);   \
        REG_PAIR (x3,  x4,  0x010, SYS_CONTEXT_SIZE);   \
        REG_ONE  (x5,       0x020, SYS_CONTEXT_SIZE);

//
// This code gets copied to the ARM vector table
// VectorTableStart - VectorTableEnd gets copied
//
ASM_PFX(ExceptionHandlersStart):

//
// Current EL with SP0 : 0x0 - 0x180
//
.align 7
ASM_PFX(SynchronousExceptionSP0):
  b   ASM_PFX(SynchronousExceptionEntry)

.align 7
ASM_PFX(IrqSP0):
  b   ASM_PFX(IrqEntry)

.align 7
ASM_PFX(FiqSP0):
  b   ASM_PFX(FiqEntry)

.align 7
ASM_PFX(SErrorSP0):
  b   ASM_PFX(SErrorEntry)

//
// Current EL with SPx: 0x200 - 0x380
//
.align 7
ASM_PFX(SynchronousExceptionSPx):
  b   ASM_PFX(SynchronousExceptionEntry)

.align 7
ASM_PFX(IrqSPx):
  b   ASM_PFX(IrqEntry)

.align 7
ASM_PFX(FiqSPx):
  b   ASM_PFX(FiqEntry)

.align 7
ASM_PFX(SErrorSPx):
  b   ASM_PFX(SErrorEntry)

//
// Lower EL using AArch64 : 0x400 - 0x580
//
.align 7
ASM_PFX(SynchronousExceptionA64):
  b   ASM_PFX(SynchronousExceptionEntry)

.align 7
ASM_PFX(IrqA64):
  b   ASM_PFX(IrqEntry)

.align 7
ASM_PFX(FiqA64):
  b   ASM_PFX(FiqEntry)

.align 7
ASM_PFX(SErrorA64):
  b   ASM_PFX(SErrorEntry)

//
// Lower EL using AArch32 : 0x0 - 0x180
//
.align 7
ASM_PFX(SynchronousExceptionA32):
  b   ASM_PFX(SynchronousExceptionEntry)

.align 7
ASM_PFX(IrqA32):
  b   ASM_PFX(IrqEntry)

.align 7
ASM_PFX(FiqA32):
  b   ASM_PFX(FiqEntry)

.align 7
ASM_PFX(SErrorA32):
  b   ASM_PFX(SErrorEntry)


#undef  REG_PAIR
#undef  REG_ONE
#define REG_PAIR(REG1, REG2, OFFSET, CONTEXT_SIZE)  stp  REG1, REG2, [sp, #(OFFSET-CONTEXT_SIZE)]
#define REG_ONE(REG1, OFFSET, CONTEXT_SIZE)         stur REG1, [sp, #(OFFSET-CONTEXT_SIZE)]

ASM_PFX(SynchronousExceptionEntry):
  // Move the stackpointer so we can reach our structure with the str instruction.
  sub sp, sp, #(FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)

  // Save all the General regs before touching x0 and x1.
  // This does not save r31(SP) as it is special. We do that later.
  ALL_GP_REGS

  // Record the type of exception that occurred.
  mov       x0, #EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS

  // Jump to our general handler to deal with all the common parts and process the exception.
  ldr       x1, ASM_PFX(CommonExceptionEntry)
  br        x1

ASM_PFX(IrqEntry):
  sub sp, sp, #(FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)
  ALL_GP_REGS
  mov       x0, #EXCEPT_AARCH64_IRQ
  ldr       x1, ASM_PFX(CommonExceptionEntry)
  br        x1

ASM_PFX(FiqEntry):
  sub sp, sp, #(FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)
  ALL_GP_REGS
  mov       x0, #EXCEPT_AARCH64_FIQ
  ldr       x1, ASM_PFX(CommonExceptionEntry)
  br        x1

ASM_PFX(SErrorEntry):
  sub sp, sp, #(FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)
  ALL_GP_REGS
  mov       x0, #EXCEPT_AARCH64_SERROR
  ldr       x1, ASM_PFX(CommonExceptionEntry)
  br        x1


//
// This gets patched by the C code that patches in the vector table
//
.align 3
ASM_PFX(CommonExceptionEntry):
  .8byte ASM_PFX(AsmCommonExceptionEntry)

ASM_PFX(ExceptionHandlersEnd):



//
// This code runs from CpuDxe driver loaded address. It is patched into
// CommonExceptionEntry.
//
ASM_PFX(AsmCommonExceptionEntry):
  /* NOTE:
     We have to break up the save code because the immediate value to be used
     with the SP is too big to do it all in one step so we need to shuffle the SP
     along as we go. (we only have 9bits of immediate to work with) */

  // Save the current Stack pointer before we start modifying it.
  SAVE_SP

  // Preserve the stack pointer we came in with before we modify it
  EL1_OR_EL2(x1)
1:mrs      x1, elr_el1   // Exception Link Register
  mrs      x2, spsr_el1  // Saved Processor Status Register 32bit
  mrs      x3, fpsr      // Floating point Status Register  32bit
  mrs      x4, esr_el1   // EL1 Exception syndrome register 32bit
  mrs      x5, far_el1   // EL1 Fault Address Register
  b        3f

2:mrs      x1, elr_el2   // Exception Link Register
  mrs      x2, spsr_el2  // Saved Processor Status Register 32bit
  mrs      x3, fpsr      // Floating point Status Register  32bit
  mrs      x4, esr_el2   // EL2 Exception syndrome register 32bit
  mrs      x5, far_el2   // EL2 Fault Address Register

  // Adjust SP to save next set
3:add      sp, sp, #FP_CONTEXT_SIZE

  // Push FP regs to Stack.
  ALL_FP_REGS

  // Adjust SP to save next set
  add      sp, sp, #SYS_CONTEXT_SIZE

  // Save the SYS regs
  ALL_SYS_REGS

  // Point to top of struct after all regs saved
  sub      sp, sp, #(GP_CONTEXT_SIZE + FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)

  // x0 still holds the exception type.
  // Set x1 to point to the top of our struct on the Stack
  mov      x1, sp

// CommonCExceptionHandler (
//   IN     EFI_EXCEPTION_TYPE           ExceptionType,   R0
//   IN OUT EFI_SYSTEM_CONTEXT           SystemContext    R1
//   )

  // Call the handler as defined above

  // For now we spin in the handler if we received an abort of some kind.
  // We do not try to recover.
  bl       ASM_PFX(CommonCExceptionHandler) // Call exception handler


// Defines for popping from stack

#undef REG_PAIR
#undef REG_ONE
#define REG_PAIR(REG1, REG2, OFFSET, CONTEXT_SIZE)    ldp  REG1, REG2, [sp, #(OFFSET-CONTEXT_SIZE)]
#define REG_ONE(REG1, OFFSET, CONTEXT_SIZE)           ldur REG1, [sp, #(OFFSET-CONTEXT_SIZE)]

  // Adjust SP to pop system registers
  add     sp, sp, #(GP_CONTEXT_SIZE + FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)
  ALL_SYS_REGS

  EL1_OR_EL2(x6)
1:msr      elr_el1, x1   // Exception Link Register
  msr      spsr_el1,x2   // Saved Processor Status Register 32bit
  msr      fpsr, x3      // Floating point Status Register  32bit
  msr      esr_el1, x4   // EL1 Exception syndrome register 32bit
  msr      far_el1, x5   // EL1 Fault Address Register
  b        3f
2:msr      elr_el2, x1   // Exception Link Register
  msr      spsr_el2,x2   // Saved Processor Status Register 32bit
  msr      fpsr, x3      // Floating point Status Register  32bit
  msr      esr_el2, x4   // EL2 Exception syndrome register 32bit
  msr      far_el2, x5   // EL2 Fault Address Register

3:// pop all regs and return from exception.
  sub     sp, sp, #(FP_CONTEXT_SIZE + SYS_CONTEXT_SIZE)
  ALL_GP_REGS

  // Adjust SP to pop next set
  add      sp, sp, #FP_CONTEXT_SIZE
  // Pop FP regs to Stack.
  ALL_FP_REGS

  // Adjust SP to be where we started from when we came into the handler.
  // The handler can not change the SP.
  add      sp, sp, #SYS_CONTEXT_SIZE

  eret

#undef REG_PAIR
#undef REG_ONE
