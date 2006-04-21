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
/// Module Name:  CallPalProc.s
///
///

.auto
.text

.proc   CallPalProcStatic
.type   CallPalProcStatic, @function
CallPalProcStatic::
        mov                 r9  = ar.k5
        mov                 r8  = ip
        add                 r8  = (PalProcReturn - CallPalProcStatic), r8
        mov                 r28 = r32
        mov                 b7  = r9
        mov                 r29 = r33
        mov                 r30 = r34
        mov                 r31 = r35
        mov                 r32 = b0
        mov                 b0  = r8
        br.sptk             b7
PalProcReturn:
        mov                 b0 = r32
        br.ret.sptk.many    b0
.endp   CallPalProcStatic
