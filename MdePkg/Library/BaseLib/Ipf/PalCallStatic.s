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
        cmp.eq              p6 = r0, in0
        mov                 r31 = in4
        mov                 r8  = ip
(p6)    mov                 in0 = ar.k5
        add                 r8  = (PalProcReturn - PalCallStatic), r8
        mov                 in4 = b0
        mov                 r30 = in3
        mov                 r29 = in2
        mov                 b7  = in0
        mov                 in3 = psr
        rsm                 1 << 14                 // Disable interrupts
        mov                 r28 = in1
        mov                 in0 = 256
        mov                 b0  = r8
        br.cond.sptk        b7
PalProcReturn:
        mov                 psr.l = in3
        cmp.eq              p6 = in0, in1           // in1 == PAL_COPY_PAL?
(p6)    cmp.eq              p6 = r0, r8             // Status == Success?
(p6)    add                 in2 = r9, in2
(p6)    mov                 ar.k5 = in2
        mov                 b0  = in4
        br.ret.sptk.many    b0
.endp   PalCallStatic
