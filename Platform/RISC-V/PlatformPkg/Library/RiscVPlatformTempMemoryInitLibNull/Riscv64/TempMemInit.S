//------------------------------------------------------------------------------
//
// RISC-V RiscVPlatformTemporaryMemInit.
//
// Copyright (c) 2016 - 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
#include <Base.h>

.data

.text
.align 3

.global ASM_PFX(RiscVPlatformTemporaryMemInit)

//
// @retval  a0 Temporary memory base.
//          a1 Temporary memory size.
//
ASM_PFX(RiscVPlatformTemporaryMemInit):
    li      a0, FixedPcdGet32 (PcdTemporaryRamBase)
    li      a1, FixedPcdGet32 (PcdTemporaryRamSize)
    ret
