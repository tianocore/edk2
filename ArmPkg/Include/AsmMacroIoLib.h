/** @file
  Macros to work around lack of Apple support for LDR register, =expr

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

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

// load _Reg with _Data
#define LoadConstantToReg(_Data, _Reg)  \
  ldr  _Reg, [pc, #0]   ;               \
  b    1f               ;               \
  .long (_Data)         ;               \
1:

#elif defined (__GNUC__)

#define LoadConstantToReg(Data, Reg) \
  ldr  Reg, =Data

#else

//
// Use ARM assembly macros, form armasam
//
//  Less magic in the macros if ldr reg, =expr works
//

// load _Reg with _Data


#define LoadConstantToReg(Data, Reg)  LoadConstantToRegMacro Data, Reg

#endif

#define _ASM_FUNC(Name, Section)    \
  .global   Name                  ; \
  .section  #Section, "ax"        ; \
  .type     Name, %function       ; \
  Name:

#define ASM_FUNC(Name)            _ASM_FUNC(ASM_PFX(Name), .text. ## Name)

#define MOV32(Reg, Val)                       \
  movw      Reg, #(Val) & 0xffff            ; \
  movt      Reg, #(Val) >> 16

#define ADRL(Reg, Sym)                        \
  movw      Reg, #:lower16:(Sym) - (. + 16) ; \
  movt      Reg, #:upper16:(Sym) - (. + 12) ; \
  add       Reg, Reg, pc

#define LDRL(Reg, Sym)                        \
  movw      Reg, #:lower16:(Sym) - (. + 16) ; \
  movt      Reg, #:upper16:(Sym) - (. + 12) ; \
  ldr       Reg, [pc, Reg]

#endif
