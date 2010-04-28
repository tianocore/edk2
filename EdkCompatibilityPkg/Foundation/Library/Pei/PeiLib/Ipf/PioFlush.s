//++
// Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
// This program and the accompanying materials                          
// are licensed and made available under the terms and conditions of the BSD License         
// which accompanies this distribution.  The full text of the license may be found at        
// http://opensource.org/licenses/bsd-license.php                                            
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
// 
//  Module Name:
//
//    pioflush.s 
//
//  Abstract:
//
//
// Revision History:
//
//--

  .file  "pioflush.c"
  .radix  C
  .section  .text,  "ax", "progbits"
  .align 32
  .section  .pdata,  "a", "progbits"
  .align 4
  .section  .xdata,  "a", "progbits"
  .align 8
  .section  .data,  "wa", "progbits"
  .align 16
  .section  .rdata,  "a", "progbits"
  .align 16
  .section  .bss,  "wa", "nobits"
  .align 16
  .section  .tls$,  "was", "progbits"
  .align 16
  .section  .sdata,  "was", "progbits"
  .align 16
  .section  .sbss,  "was", "nobits"
  .align 16
  .section  .srdata,  "as", "progbits"
  .align 16
  .section  .rdata,  "a", "progbits"
  .align 16
  .section  .rtcode,  "ax", "progbits"
  .align 32
  .type  RtPioICacheFlush#  ,@function 
        .globl RtPioICacheFlush#
// Function compile flags: /Ogsy
  .section  .rtcode

// Begin code for function: RtPioICacheFlush:
  .proc  RtPioICacheFlush#
  .align 32
RtPioICacheFlush:  
// File e:\tmp\pioflush.c
 {   .mii  //R-Addr: 0X00 
  alloc  r3=2, 0, 0, 0            //11, 00000002H
  cmp4.leu p0,p6=32, r33;;          //15, 00000020H
  (p6)  mov  r33=32;;            //16, 00000020H
 }
 {   .mii  //R-Addr: 0X010 
  nop.m   0
  zxt4  r29=r33;;            //21
  dep.z  r30=r29, 0, 5;;            //21, 00000005H
 }
 {   .mii  //R-Addr: 0X020 
  cmp4.eq  p0,p7=r0, r30            //21
  shr.u  r28=r29, 5;;            //19, 00000005H
  (p7)  adds  r28=1, r28;;            //22, 00000001H
 }
 {   .mii  //R-Addr: 0X030 
  nop.m   0
  shl  r27=r28, 5;;            //25, 00000005H
  zxt4  r26=r27;;            //25
 }
 {   .mfb  //R-Addr: 0X040 
  add  r31=r26, r32            //25
  nop.f   0
  nop.b   0
 }
$L143:
 {   .mii  //R-Addr: 0X050 
  fc   r32              //27
  adds  r32=32, r32;;            //28, 00000020H
  cmp.ltu  p14,p15=r32, r31          //29
 }
 {   .mfb  //R-Addr: 0X060 
  nop.m   0
  nop.f   0
  (p14)  br.cond.dptk.few $L143#;;          //29, 880000/120000
 }
 {   .mmi
    sync.i;;
    srlz.i
    nop.i   0;;
 }
 {   .mfb  //R-Addr: 0X070 
  nop.m   0
  nop.f   0
  br.ret.sptk.few b0;;            //31
 }
// End code for function:
  .endp  RtPioICacheFlush#
// END
