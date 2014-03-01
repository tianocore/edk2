/** @file
  Macros to work around lack of Apple support for LDR register, =expr

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __MACRO_IO_LIBV8_H__
#define __MACRO_IO_LIBV8_H__

#if defined (__GNUC__)

#define MmioWrite32(Address, Data) \
  ldr  x1, =Address ;              \
  ldr  w0, =Data    ;              \
  str  w0, [x1]

#define MmioOr32(Address, OrData) \
  ldr  x1, =Address ;             \
  ldr  w2, =OrData  ;             \
  ldr  w0, [x1]     ;             \
  orr  w0, w0, w2   ;             \
  str  w0, [x1]

#define MmioAnd32(Address, AndData) \
  ldr  x1, =Address ;               \
  ldr  w2, =AndData ;               \
  ldr  w0, [x1]     ;               \
  and  w0, w0, w2   ;               \
  str  w0, [x1]

#define MmioAndThenOr32(Address, AndData, OrData) \
  ldr  x1, =Address ;                             \
  ldr  w0, [x1]     ;                             \
  ldr  w2, =AndData ;                             \
  and  w0, w0, w2   ;                             \
  ldr  w2, =OrData  ;                             \
  orr  w0, w0, w2   ;                             \
  str  w0, [x1]

#define MmioWriteFromReg32(Address, Reg) \
  ldr  x1, =Address ;                    \
  str  Reg, [x1]

#define MmioRead32(Address) \
  ldr  x1, =Address ;       \
  ldr  w0, [x1]

#define MmioReadToReg32(Address, Reg) \
  ldr  x1, =Address ;                 \
  ldr  Reg, [x1]

#define LoadConstant(Data) \
  ldr  x0, =Data

#define LoadConstantToReg(Data, Reg) \
  ldr  Reg, =Data

#define SetPrimaryStack(StackTop, GlobalSize, Tmp, Tmp1)  \
  ands    Tmp, GlobalSize, #15        ;                   \
  mov     Tmp1, #16                   ;                   \
  sub     Tmp1, Tmp1, Tmp             ;                   \
  csel    Tmp, Tmp1, Tmp, ne          ;                   \
  add     GlobalSize, GlobalSize, Tmp ;                   \
  sub     sp, StackTop, GlobalSize    ;                   \
                                      ;                   \
  mov     Tmp, sp                     ;                   \
  mov     GlobalSize, #0x0            ;                   \
_SetPrimaryStackInitGlobals:          ;                   \
  cmp     Tmp, StackTop               ;                   \
  b.eq    _SetPrimaryStackEnd         ;                   \
  str     GlobalSize, [Tmp], #8       ;                   \
  b       _SetPrimaryStackInitGlobals ;                   \
_SetPrimaryStackEnd:

// Initialize the Global Variable with '0'
#define InitializePrimaryStack(GlobalSize, Tmp1, Tmp2) \
  and     Tmp1, GlobalSize, #15       ;             \
  mov     Tmp2, #16                   ;             \
  sub     Tmp2, Tmp2, Tmp1            ;             \
  add     GlobalSize, GlobalSize, Tmp2 ;            \
                                      ;             \
  mov     Tmp1, sp                    ;             \
  sub     sp, sp, GlobalSize          ;             \
  mov     GlobalSize, #0x0            ;             \
_InitializePrimaryStackLoop:          ;             \
  mov     Tmp2, sp                    ;             \
  cmp     Tmp1, Tmp2                  ;             \
  bls     _InitializePrimaryStackEnd  ;             \
  str     GlobalSize, [Tmp1, #-8]!    ;             \
  b       _InitializePrimaryStackLoop ;             \
_InitializePrimaryStackEnd:

// CurrentEL : 0xC = EL3; 8 = EL2; 4 = EL1
// This only selects between EL1 and EL2, else we die.
// Provide the Macro with a safe temp xreg to use.
#define EL1_OR_EL2(SAFE_XREG)        \
        mrs    SAFE_XREG, CurrentEL ;\
        cmp    SAFE_XREG, #0x8      ;\
        b.eq   2f                   ;\
        cmp    SAFE_XREG, #0x4      ;\
        b.ne   .                    ;// We should never get here
// EL1 code starts here

// CurrentEL : 0xC = EL3; 8 = EL2; 4 = EL1
// This only selects between EL1 and EL2 and EL3, else we die.
// Provide the Macro with a safe temp xreg to use.
#define EL1_OR_EL2_OR_EL3(SAFE_XREG) \
        mrs    SAFE_XREG, CurrentEL ;\
        cmp    SAFE_XREG, #0xC      ;\
        b.eq   3f                   ;\
        cmp    SAFE_XREG, #0x8      ;\
        b.eq   2f                   ;\
        cmp    SAFE_XREG, #0x4      ;\
        b.ne   .                    ;// We should never get here
// EL1 code starts here

#else

#error RVCT AArch64 tool chain is not supported

#endif // __GNUC__ 

#endif // __MACRO_IO_LIBV8_H__

