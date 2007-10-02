//++
// Copyright (c) 2006, Intel Corporation
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//  Module Name:
//
//    IpfCpuCore.s
//
//  Abstract:
//    IPF Specific assembly routines
//
//--

.file  "IpfCpuCore.s"

#include  "IpfMacro.i"
#include  "Ipf/IpfCpuCore.i"

//---------------------------------------------------------------------------------
//++
// GetHandOffStatus
//
// This routine is called by all processors simultaneously, to get some hand-off
// status that has been captured by IPF dispatcher and recorded in kernel registers.
//
// Arguments :
//
// On Entry :  None.
//
// Return Value: Lid, R20Status.
//
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (GetHandOffStatus)

        NESTED_SETUP (0,2+0,0,0)

        mov     r8 = ar.k6              // Health Status (Self test params)
        mov     r9 = ar.k4              // LID bits
        mov     r10 = ar.k3;;           // SAL_E entry state
        mov     r11 = ar.k1             // Return address to PAL

        NESTED_RETURN
PROCEDURE_EXIT (GetHandOffStatus)
//----------------------------------------------------------------------------------


