//------------------------------------------------------------------------------ 
//
// CpuSleep for RISC-V
//
// Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php.
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------
.data
.align 3
.section .text

.global ASM_PFX(_CpuSleep)
                           
ASM_PFX(_CpuSleep):
    wfi
    ret


