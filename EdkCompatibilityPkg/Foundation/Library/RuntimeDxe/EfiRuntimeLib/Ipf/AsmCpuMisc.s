/// @file
///   Contains an implementation of EcpEfiBreakPoint and EcpMemoryFence on Itanium-based
///   architecture.
///
/// Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
/// This program and the accompanying materials
/// are licensed and made available under the terms and conditions of the BSD License
/// which accompanies this distribution.  The full text of the license may be found at
/// http://opensource.org/licenses/bsd-license.php
///
/// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
/// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
///
/// Module Name:  AsmCpuMisc.s
///
///


.text
.proc EcpEfiBreakPoint
.type EcpEfiBreakPoint, @function

EcpEfiBreakPoint::
        break.i 0;;
        br.ret.dpnt    b0;;

.endp EcpEfiBreakPoint

.proc EcpMemoryFence
.type EcpMemoryFence, @function

EcpMemoryFence::
        mf;;    // memory access ordering

        // do we need the mf.a also here?
        mf.a    // wait for any IO to complete?
        
        // not sure if we need serialization here, just put it, in case...
        
        srlz.d;;
        srlz.i;;
        
        br.ret.dpnt    b0;;
.endp EcpMemoryFence