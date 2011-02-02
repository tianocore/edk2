//------------------------------------------------------------------------------ 
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


  .text
  .align 2
  GCC_ASM_EXPORT(__aeabi_ldivmod)

//
// A pair of (unsigned) long longs is returned in {{r0, r1}, {r2, r3}}, 
//  the quotient in {r0, r1}, and the remainder in {r2, r3}.
//
//__value_in_regs lldiv_t
//EFIAPI
//__aeabi_ldivmod (
//  IN UINT64  Dividen
//  IN UINT64  Divisor
//  )//
//

ASM_PFX(__aeabi_ldivmod):
    push     {r4,lr}
    asrs     r4,r1,#1
    eor      r4,r4,r3,LSR #1
    bpl      L_Test1
    rsbs     r0,r0,#0
    rsc      r1,r1,#0
L_Test1:
    tst      r3,r3
    bpl      L_Test2
    rsbs     r2,r2,#0
    rsc      r3,r3,#0
L_Test2:
    bl       ASM_PFX(__aeabi_uldivmod)  
    tst      r4,#0x40000000
    beq      L_Test3
    rsbs     r0,r0,#0
    rsc      r1,r1,#0
L_Test3:
    tst      r4,#0x80000000
    beq      L_Exit
    rsbs     r2,r2,#0
    rsc      r3,r3,#0
L_Exit:
    pop      {r4,pc}
 


