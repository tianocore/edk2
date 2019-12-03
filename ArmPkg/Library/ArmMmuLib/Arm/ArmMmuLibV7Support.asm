//------------------------------------------------------------------------------
//
// Copyright (c) 2016, Linaro Limited. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------



    INCLUDE AsmMacroExport.inc


//------------------------------------------------------------------------------

 RVCT_ASM_EXPORT ArmHasMpExtensions
  mrc     p15,0,R0,c0,c0,5
  // Get Multiprocessing extension (bit31)
  lsr     R0, R0, #31
  bx      LR

 RVCT_ASM_EXPORT ArmReadIdMmfr0
  mrc    p15, 0, r0, c0, c1, 4     ; Read ID_MMFR0 Register
  bx     lr

  END
