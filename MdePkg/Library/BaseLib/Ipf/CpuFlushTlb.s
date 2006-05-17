/// @file
///   CpuFlushTlb() function for Itanium-based architecture.
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
/// Module Name:  CpuFlushTlb.s
///
///

.auto
.text

.proc   CpuFlushTlb
.type   CpuFlushTlb, @function
CpuFlushTlb::
        mov                 r8  = ip
        mov                 r9  = -1
        dep.z               r10 = -1, 61, 3
        and                 r8  = r8, r10
        ptc.l               r8, r9
        br.ret.sptk.many    b0
.endp
