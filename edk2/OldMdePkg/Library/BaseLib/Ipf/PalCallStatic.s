/// @file
///   Contains an implementation of CallPalProcStatic on Itanium-based
///   architecture.
///
/// Copyright (c) 2006, Intel Corporation
/// All rights reserved. This program and the accompanying materials
/// are licensed and made available under the terms and conditions of the BSD License
/// which accompanies this distribution.  The full text of the license may be found at
/// http://opensource.org/licenses/bsd-license.php
///
/// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
/// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
///
/// Module Name:  PalCallStatic.s
///
///

.auto
.text

.proc   PalCallStatic
.type   PalCallStatic, @function
.regstk 5, 0, 0, 0
PalCallStatic::
        cmp.eq              p15 = in0, r0
        mov                 r31 = in4
        mov                 r8  = ip

(p15)   mov                 in0 = ar.k5
        add                 r8  = (_PalProcReturn - PalCallStatic), r8
        mov                 r30 = in3

        mov                 in4 = psr
        mov                 in3 = b0
        mov                 b7  = in0

        rsm                 1 << 14                 // Disable interrupts
        mov                 r29 = in2
        mov                 r28 = in1

        mov                 b0  = r8
        br.cond.sptk.many   b7

_PalProcReturn:
        mov                 psr.l = in4
        mov                 b0  = in3
        br.ret.sptk.many    b0
.endp   PalCallStatic
