///------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
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



    EXPORT  __ARM_switch8

    AREA  ArmSwitch, CODE, READONLY

__ARM_switch8
  LDRB    r12,[lr,#-1]
  CMP      r3,r12
  LDRBCC  r3,[lr,r3]
  LDRBCS  r3,[lr,r12]
  ADD      r12,lr,r3,LSL #1
  BX      r12

    END
