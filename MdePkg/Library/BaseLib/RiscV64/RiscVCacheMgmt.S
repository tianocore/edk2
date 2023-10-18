//------------------------------------------------------------------------------
//
// RISC-V cache operation.
//
// Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
// Copyright (c) 2023, Rivos Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
.include "RiscVasm.inc"

.align 3
ASM_GLOBAL ASM_PFX(RiscVInvalidateInstCacheAsmFence)
ASM_GLOBAL ASM_PFX(RiscVInvalidateDataCacheAsmFence)

ASM_PFX(RiscVInvalidateInstCacheAsmFence):
    fence.i
    ret

ASM_PFX(RiscVInvalidateDataCacheAsmFence):
    fence
    ret

ASM_GLOBAL ASM_PFX (RiscVCpuCacheFlushAsmCmo)
ASM_PFX (RiscVCpuCacheFlushAsmCmo):
    RISCVCMOFLUSH
    ret

ASM_GLOBAL ASM_PFX (RiscVCpuCacheCleanAsmCmo)
ASM_PFX (RiscVCpuCacheCleanAsmCmo):
    RISCVCMOCLEAN
    ret

ASM_GLOBAL ASM_PFX (RiscVCpuCacheInvalAsmCmo)
ASM_PFX (RiscVCpuCacheInvalAsmCmo):
    RISCVCMOINVALIDATE
    ret
