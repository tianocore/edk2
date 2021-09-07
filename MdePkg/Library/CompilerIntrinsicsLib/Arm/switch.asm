///------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------




    INCLUDE AsmMacroExport.inc

 RVCT_ASM_EXPORT __ARM_switch8
  LDRB    r12,[lr,#-1]
  CMP      r3,r12
  LDRBCC  r3,[lr,r3]
  LDRBCS  r3,[lr,r12]
  ADD      r12,lr,r3,LSL #1
  BX      r12

    END
