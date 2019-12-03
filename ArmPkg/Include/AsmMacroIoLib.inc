;%HEADER%
;/** @file
;  Macros to work around lack of Apple support for LDR register, =expr
;
;  Copyright (c) 2009, Apple Inc. All rights reserved.<BR>
;  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>
;
;  SPDX-License-Identifier: BSD-2-Clause-Patent
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
