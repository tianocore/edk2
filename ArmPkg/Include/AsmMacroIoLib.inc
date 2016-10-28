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
  adrll   $Reg, $Symbol
  add     $Reg, pc, #-8
  RELOC   R_ARM_ALU_PC_G0_NC, $Symbol
  add     $Reg, $Reg, #-4
  RELOC   R_ARM_ALU_PC_G1_NC, $Symbol
  add     $Reg, $Reg, #0
  RELOC   R_ARM_ALU_PC_G2, $Symbol
  MEND

  MACRO
  ldrl    $Reg, $Symbol
  add     $Reg, pc, #-8
  RELOC   R_ARM_ALU_PC_G0_NC, $Symbol
  add     $Reg, $Reg, #-4
  RELOC   R_ARM_ALU_PC_G1_NC, $Symbol
  ldr     $Reg, [$Reg, #0]
  RELOC   R_ARM_LDR_PC_G2, $Symbol
  MEND

  END
