/**

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

@file
  CommonIncludes.h

@brief
  This file defines common equates.

**/
#ifndef _COMMON_INCLUDES_H_
#define _COMMON_INCLUDES_H_

#define V_INTEL_VID 0x8086

#ifndef STALL_ONE_MICRO_SECOND
#define STALL_ONE_MICRO_SECOND  1
#endif
#ifndef STALL_ONE_MILLI_SECOND
#define STALL_ONE_MILLI_SECOND  1000
#endif
///
/// Min Max
///
#define V_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define V_MAX(a, b) (((a) > (b)) ? (a) : (b))

///
/// Bit map macro
///
#ifndef BIT0

#define BIT63 0x8000000000000000
#define BIT62 0x4000000000000000
#define BIT61 0x2000000000000000
#define BIT60 0x1000000000000000
#define BIT59 0x0800000000000000
#define BIT58 0x0400000000000000
#define BIT57 0x0200000000000000
#define BIT56 0x0100000000000000
#define BIT55 0x0080000000000000
#define BIT54 0x0040000000000000
#define BIT53 0x0020000000000000
#define BIT52 0x0010000000000000
#define BIT51 0x0008000000000000
#define BIT50 0x0004000000000000
#define BIT49 0x0002000000000000
#define BIT48 0x0001000000000000
#define BIT47 0x0000800000000000
#define BIT46 0x0000400000000000
#define BIT45 0x0000200000000000
#define BIT44 0x0000100000000000
#define BIT43 0x0000080000000000
#define BIT42 0x0000040000000000
#define BIT41 0x0000020000000000
#define BIT40 0x0000010000000000
#define BIT39 0x0000008000000000
#define BIT38 0x0000004000000000
#define BIT37 0x0000002000000000
#define BIT36 0x0000001000000000
#define BIT35 0x0000000800000000
#define BIT34 0x0000000400000000
#define BIT33 0x0000000200000000
#define BIT32 0x0000000100000000

#define BIT31 0x80000000
#define BIT30 0x40000000
#define BIT29 0x20000000
#define BIT28 0x10000000
#define BIT27 0x08000000
#define BIT26 0x04000000
#define BIT25 0x02000000
#define BIT24 0x01000000
#define BIT23 0x00800000
#define BIT22 0x00400000
#define BIT21 0x00200000
#define BIT20 0x00100000
#define BIT19 0x00080000
#define BIT18 0x00040000
#define BIT17 0x00020000
#define BIT16 0x00010000
#define BIT15 0x00008000
#define BIT14 0x00004000
#define BIT13 0x00002000
#define BIT12 0x00001000
#define BIT11 0x00000800
#define BIT10 0x00000400
#define BIT9  0x00000200
#define BIT8  0x00000100
#define BIT7  0x00000080
#define BIT6  0x00000040
#define BIT5  0x00000020
#define BIT4  0x00000010
#define BIT3  0x00000008
#define BIT2  0x00000004
#define BIT1  0x00000002
#define BIT0  0x00000001
#endif

#define BITS(x) (1 << (x))

//
// Notes :
// 1.  Bit position always starts at 0.
// 2.  Following macros are applicable only for Word alligned integers.
//
#define BIT(Pos, Value)               (1 << (Pos) & (Value))
#define BITRANGE(From, Width, Value)  (((Value) >> (From)) & ((1 << (Width)) - 1))

#endif
