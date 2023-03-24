//------------------------------------------------------------------------------
//
// CpuPause for RISC-V
//
// Copyright (c) 2022, Rivos Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(RiscVCpuCacheFlush)
ASM_PFX(RiscVCpuCacheFlush):
  cbo.flush (a0)
  ret

ASM_GLOBAL ASM_PFX(RiscVCpuCacheClean)
ASM_PFX(RiscVCpuCacheClean):
  cbo.clean (a0)
  ret

ASM_GLOBAL ASM_PFX(RiscVCpuCacheInval)
ASM_PFX(RiscVCpuCacheInval):
  cbo.inval (a0)
  ret
