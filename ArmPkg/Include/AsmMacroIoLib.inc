;%HEADER%
;/** @file
;  Macros to work around lack of Apple support for LDR register, =expr
;
;  Copyright (c) 2009, Apple Inc. All rights reserved.<BR>
;  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>
;
;  This program and the accompanying materials
;  are licensed and made available under the terms and conditions of the BSD License
;  which accompanies this distribution.  The full text of the license may be found at
;  http://opensource.org/licenses/bsd-license.php
;
;  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
;  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;**/


  MACRO
  MmioWrite32Macro $Address, $Data
  ldr  r1, = ($Address)
  ldr  r0, = ($Data)
  str  r0, [r1]
  MEND

  MACRO
  MmioOr32Macro $Address, $OrData
  ldr  r1, =($Address)
  ldr  r2, =($OrData)
  ldr  r0, [r1]
  orr  r0, r0, r2
  str  r0, [r1]
  MEND

  MACRO
  MmioAnd32Macro $Address, $AndData
  ldr  r1, =($Address)
  ldr  r2, =($AndData)
  ldr  r0, [r1]
  and  r0, r0, r2
  str  r0, [r1]
  MEND

  MACRO
  MmioAndThenOr32Macro $Address, $AndData, $OrData
  ldr  r1, =($Address)
  ldr  r0, [r1]
  ldr  r2, =($AndData)
  and  r0, r0, r2
  ldr  r2, =($OrData)
  orr  r0, r0, r2
  str  r0, [r1]
  MEND

  MACRO
  MmioWriteFromReg32Macro $Address, $Reg
  ldr  r1, =($Address)
  str  $Reg, [r1]
  MEND

  MACRO
  MmioRead32Macro $Address
  ldr  r1, =($Address)
  ldr  r0, [r1]
  MEND

  MACRO
  MmioReadToReg32Macro $Address, $Reg
  ldr  r1, =($Address)
  ldr  $Reg, [r1]
  MEND

  MACRO
  LoadConstantMacro $Data
  ldr  r0, =($Data)
  MEND

  MACRO
  LoadConstantToRegMacro $Data, $Reg
  ldr  $Reg, =($Data)
  MEND

  ; The reserved place must be 8-bytes aligned for pushing 64-bit variable on the stack
  ; Note: Global Size will be modified
  MACRO
  SetPrimaryStack $StackTop, $GlobalSize, $Tmp
  and     $Tmp, $GlobalSize, #7
  rsbne   $Tmp, $Tmp, #8
  add     $GlobalSize, $GlobalSize, $Tmp
  sub     sp, $StackTop, $GlobalSize

  ; Set all the global variables to 0
  mov     $Tmp, sp
  mov     $GlobalSize, #0x0
_SetPrimaryStackInitGlobals
  cmp     $Tmp, $StackTop
  beq     _SetPrimaryStackEnd
  str     $GlobalSize, [$Tmp], #4
  b       _SetPrimaryStackInitGlobals
_SetPrimaryStackEnd
  MEND

  MACRO
  InitializePrimaryStack $GlobalSize, $Tmp1
  and     $Tmp1, $GlobalSize, #7
  rsbne   $Tmp1, $Tmp1, #8
  add     $GlobalSize, $GlobalSize, $Tmp1

  mov     $Tmp1, sp
  sub     sp, $GlobalSize
  ; Set all the global variables to 0
  mov     $GlobalSize, #0x0
_InitializePrimaryStackLoop
  cmp     $Tmp1, sp
  bls     _InitializePrimaryStackEnd
  str     $GlobalSize, [$Tmp1, #-4]!
  b       _InitializePrimaryStackLoop
_InitializePrimaryStackEnd
  MEND

  END
