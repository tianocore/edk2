//------------------------------------------------------------------------------
// RiscV64/StackCookieInterrupt.S
//
// Copyright (c) Xiang W <wangxiang@iscas.ac.cn>.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//------------------------------------------------------------------------------

    .text

//------------------------------------------------------------------------------
// Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
//
// VOID
// TriggerStackCookieInterrupt (
//   VOID
//   );
//------------------------------------------------------------------------------
.global ASM_PFX(TriggerStackCookieInterrupt)
ASM_PFX(TriggerStackCookieInterrupt):
    li a0, FixedPcdGet8 (PcdStackCookieExceptionVector)
    ebreak
    ret
