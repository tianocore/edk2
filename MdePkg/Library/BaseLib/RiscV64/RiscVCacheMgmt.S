//------------------------------------------------------------------------------
//
// RISC-V cache operation.
//
// Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
// Copyright (c) 2022, Rivos Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------
#include <Register/RiscV64/RiscVImpl.h>

.align 3
ASM_GLOBAL ASM_PFX(RiscVInvalidateInstCacheAsm_Fence)
ASM_GLOBAL ASM_PFX(RiscVInvalidateDataCacheAsm_Fence)

ASM_PFX(RiscVInvalidateInstCacheAsm_Fence):
    fence.i
    ret

ASM_PFX(RiscVInvalidateDataCacheAsm_Fence):
    fence
    ret

ASM_GLOBAL ASM_PFX (RiscVCpuCacheFlushAsm_Cbo)
ASM_PFX (RiscVCpuCacheFlushAsm_Cbo):

  .long 0x0025200f
  ret

ASM_GLOBAL ASM_PFX (RiscVCpuCacheCleanAsm_Cbo)
ASM_PFX (RiscVCpuCacheCleanAsm_Cbo):
  .long 0x0015200f
  ret

ASM_GLOBAL ASM_PFX (RiscVCpuCacheInvalAsm_Cbo)
ASM_PFX (RiscVCpuCacheInvalAsm_Cbo):
  .long 0x0005200f
  ret

ASM_GLOBAL ASM_PFX(RiscvIsCbiEnabledAsm)
ASM_PFX(RiscvIsCbiEnabledAsm):
    li    a0, 3
    csrr  a1, CSR_SENVCFG
    and   a1, a1, SENVCFG_CBIE
    beqz  a1, skip

    and   a1, a1, 0x10
    beqz  a1, skip

    mv    a0, x0
skip:
    ret

ASM_GLOBAL ASM_PFX(RiscvIsCbcfeEnabledAsm)
ASM_PFX(RiscvIsCbcfeEnabledAsm):
    li    a0, 3
    csrr  a1, CSR_SENVCFG
    and   a1, a1, SENVCFG_CBCFE
    beqz  a1, next

    mv    a0, x0
next:
    ret
