/** @file
  Macros to work around lack of Clang support for LDR register, =expr

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
#if defined(__clang__)

// load x0 with _Data
#define LoadConstant(_Data)              \
  ldr  x0, 1f                          ; \
  b    2f                              ; \
.align(8)                              ; \
1:                                       \
  .8byte (_Data)                       ; \
2:

// load _Reg with _Data
#define LoadConstantToReg(_Data, _Reg)    \
  ldr  _Reg, 1f                         ; \
  b    2f                               ; \
.align(8)                               ; \
1:                                        \
  .8byte (_Data)                        ; \
2:

#elif defined (__GNUC__)

#define LoadConstant(Data) \
  ldr  x0, =Data

#define LoadConstantToReg(Data, Reg) \
  ldr  Reg, =Data

#endif // __GNUC__

#endif // __MACRO_IO_LIBV8_H__

