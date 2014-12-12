/** @file
  Macros to work around lack of Apple support for LDR register, =expr

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __MACRO_IO_LIB_H__
#define __MACRO_IO_LIB_H__

#if defined(__APPLE__)

//
//  ldr reg, =expr does not work with current Apple tool chain. So do the work our selves
//

// returns _Data in R0 and _Address in R1
#define MmioWrite32(_Address, _Data) \
  ldr  r1, [pc, #8]     ;            \
  ldr  r0, [pc, #8]     ;            \
  str  r0, [r1]         ;            \
  b    1f               ;            \
  .long (_Address)      ;            \
  .long (_Data) ;                    \
1:

// returns _Data in R0 and _Address in R1, and _OrData in r2
#define MmioOr32(_Address, _OrData) \
  ldr  r1, [pc, #16]    ;           \
  ldr  r2, [pc, #16]    ;           \
  ldr  r0, [r1]         ;           \
  orr  r0, r0, r2       ;           \
  str  r0, [r1]         ;           \
  b    1f               ;           \
  .long (_Address)      ;           \
  .long (_OrData)       ;           \
1:

// returns _Data in R0 and _Address in R1, and _OrData in r2
#define MmioAnd32(_Address, _AndData) \
  ldr  r1, [pc, #16]    ;             \
  ldr  r2, [pc, #16]    ;             \
  ldr  r0, [r1]         ;             \
  and  r0, r0, r2       ;             \
  str  r0, [r1]         ;             \
  b    1f               ;             \
  .long (_Address)      ;             \
  .long (_AndData)       ;             \
1:

// returns result in R0, _Address in R1, and _OrData in r2
#define MmioAndThenOr32(_Address, _AndData, _OrData)  \
  ldr  r1, [pc, #24]    ;                             \
  ldr  r0, [r1]         ;                             \
  ldr  r2, [pc, #20]    ;                             \
  and  r0, r0, r2       ;                             \
  ldr  r2, [pc, #16]    ;                             \
  orr  r0, r0, r2       ;                             \
  str  r0, [r1]         ;                             \
  b    1f               ;                             \
  .long (_Address)      ;                             \
  .long (_AndData)      ;                             \
  .long (_OrData)       ;                             \
1:

// returns _Data in _Reg and _Address in R1
#define MmioWriteFromReg32(_Address, _Reg) \
  ldr  r1, [pc, #4]     ;                  \
  str  _Reg, [r1]       ;                  \
  b    1f               ;                  \
  .long (_Address)      ;                  \
1:


// returns _Data in R0 and _Address in R1
#define MmioRead32(_Address)   \
  ldr  r1, [pc, #4]     ;      \
  ldr  r0, [r1]         ;      \
  b    1f               ;      \
  .long (_Address)      ;      \
1:

// returns _Data in Reg and _Address in R1
#define MmioReadToReg32(_Address, _Reg) \
  ldr  r1, [pc, #4]     ;               \
  ldr  _Reg, [r1]       ;               \
  b    1f               ;               \
  .long (_Address)      ;               \
1:


// load R0 with _Data
#define LoadConstant(_Data)  \
  ldr  r0, [pc, #0]     ;    \
  b    1f               ;    \
  .long (_Data)         ;    \
1:

// load _Reg with _Data
#define LoadConstantToReg(_Data, _Reg)  \
  ldr  _Reg, [pc, #0]   ;               \
  b    1f               ;               \
  .long (_Data)         ;               \
1:

// load _Reg with _Data if eq
#define LoadConstantToRegIfEq(_Data, _Reg)  \
  ldreq  _Reg, [pc, #0]   ;                 \
  b    1f                 ;                 \
  .long (_Data)           ;                 \
1:

// Reserve a region at the top of the Primary Core stack
// for Global variables for the XIP phase
#define SetPrimaryStack(StackTop, GlobalSize, Tmp)  \
  and     Tmp, GlobalSize, #7         ;             \
  rsbne   Tmp, Tmp, #8                ;             \
  add     GlobalSize, GlobalSize, Tmp ;             \
  sub     sp, StackTop, GlobalSize    ;             \
                                      ;             \
  mov     Tmp, sp                     ;             \
  mov     GlobalSize, #0x0            ;             \
_SetPrimaryStackInitGlobals:          ;             \
  cmp     Tmp, StackTop               ;             \
  beq     _SetPrimaryStackEnd         ;             \
  str     GlobalSize, [Tmp], #4       ;             \
  b       _SetPrimaryStackInitGlobals ;             \
_SetPrimaryStackEnd:

// Initialize the Global Variable with '0'
#define InitializePrimaryStack(GlobalSize, Tmp1)    \
  and     Tmp1, GlobalSize, #7        ;             \
  rsbne   Tmp1, Tmp1, #8              ;             \
  add     GlobalSize, GlobalSize, Tmp1 ;            \
                                      ;             \
  mov     Tmp1, sp                    ;             \
  sub     sp, GlobalSize              ;             \
  mov     GlobalSize, #0x0            ;             \
_InitializePrimaryStackLoop:          ;             \
  cmp     Tmp1, sp                    ;             \
  bls     _InitializePrimaryStackEnd  ;             \
  str     GlobalSize, [Tmp1, #-4]!    ;             \
  b       _InitializePrimaryStackLoop ;             \
_InitializePrimaryStackEnd:

#elif defined (__GNUC__)

#define MmioWrite32(Address, Data) \
  ldr  r1, =Address ;              \
  ldr  r0, =Data    ;              \
  str  r0, [r1]

#define MmioOr32(Address, OrData) \
  ldr  r1, =Address ;             \
  ldr  r2, =OrData  ;             \
  ldr  r0, [r1]     ;             \
  orr  r0, r0, r2   ;             \
  str  r0, [r1]

#define MmioAnd32(Address, AndData) \
  ldr  r1, =Address ;               \
  ldr  r2, =AndData ;               \
  ldr  r0, [r1]     ;               \
  and  r0, r0, r2   ;               \
  str  r0, [r1]

#define MmioAndThenOr32(Address, AndData, OrData) \
  ldr  r1, =Address ;                             \
  ldr  r0, [r1]     ;                             \
  ldr  r2, =AndData ;                             \
  and  r0, r0, r2   ;                             \
  ldr  r2, =OrData  ;                             \
  orr  r0, r0, r2   ;                             \
  str  r0, [r1]

#define MmioWriteFromReg32(Address, Reg) \
  ldr  r1, =Address ;                    \
  str  Reg, [r1]

#define MmioRead32(Address) \
  ldr  r1, =Address ;       \
  ldr  r0, [r1]

#define MmioReadToReg32(Address, Reg) \
  ldr  r1, =Address ;                 \
  ldr  Reg, [r1]

#define LoadConstant(Data) \
  ldr  r0, =Data

#define LoadConstantToReg(Data, Reg) \
  ldr  Reg, =Data

#define SetPrimaryStack(StackTop, GlobalSize, Tmp)  \
  and     Tmp, GlobalSize, #7         ;             \
  rsbne   Tmp, Tmp, #8                ;             \
  add     GlobalSize, GlobalSize, Tmp ;             \
  sub     sp, StackTop, GlobalSize    ;             \
                                      ;             \
  mov     Tmp, sp                     ;             \
  mov     GlobalSize, #0x0            ;             \
_SetPrimaryStackInitGlobals:          ;             \
  cmp     Tmp, StackTop               ;             \
  beq     _SetPrimaryStackEnd         ;             \
  str     GlobalSize, [Tmp], #4       ;             \
  b       _SetPrimaryStackInitGlobals ;             \
_SetPrimaryStackEnd:

// Initialize the Global Variable with '0'
#define InitializePrimaryStack(GlobalSize, Tmp1)    \
  and     Tmp1, GlobalSize, #7        ;             \
  rsbne   Tmp1, Tmp1, #8              ;             \
  add     GlobalSize, GlobalSize, Tmp1 ;            \
                                      ;             \
  mov     Tmp1, sp                    ;             \
  sub     sp, GlobalSize              ;             \
  mov     GlobalSize, #0x0            ;             \
_InitializePrimaryStackLoop:          ;             \
  cmp     Tmp1, sp                    ;             \
  bls     _InitializePrimaryStackEnd  ;             \
  str     GlobalSize, [Tmp1, #-4]!    ;             \
  b       _InitializePrimaryStackLoop ;             \
_InitializePrimaryStackEnd:

#else

//
// Use ARM assembly macros, form armasam
//
//  Less magic in the macros if ldr reg, =expr works
//

// returns _Data in R0 and _Address in R1



#define MmioWrite32(Address, Data) MmioWrite32Macro Address, Data




// returns Data in R0 and Address in R1, and OrData in r2
#define MmioOr32(Address, OrData) MmioOr32Macro Address, OrData


// returns _Data in R0 and _Address in R1, and _OrData in r2


#define MmioAnd32(Address, AndData)  MmioAnd32Macro Address, AndData

// returns result in R0, _Address in R1, and _OrData in r2


#define MmioAndThenOr32(Address, AndData, OrData) MmioAndThenOr32Macro Address, AndData, OrData


// returns _Data in _Reg and _Address in R1


#define MmioWriteFromReg32(Address, Reg) MmioWriteFromReg32Macro Address, Reg

// returns _Data in R0 and _Address in R1


#define MmioRead32(Address)  MmioRead32Macro Address

// returns _Data in Reg and _Address in R1


#define MmioReadToReg32(Address, Reg) MmioReadToReg32Macro Address, Reg


// load R0 with _Data


#define LoadConstant(Data)  LoadConstantMacro Data

// load _Reg with _Data


#define LoadConstantToReg(Data, Reg)  LoadConstantToRegMacro Data, Reg

// conditional load testing eq flag
#define LoadConstantToRegIfEq(Data, Reg)  LoadConstantToRegIfEqMacro Data, Reg

#define SetPrimaryStack(StackTop,GlobalSize,Tmp) SetPrimaryStack StackTop, GlobalSize, Tmp

// Initialize the Global Variable with '0'
#define InitializePrimaryStack(GlobalSize, Tmp1) InitializePrimaryStack GlobalSize, Tmp1

#endif

#endif
