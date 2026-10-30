//------------------------------------------------------------------------------
//
// Clear the supervisor trap shadow left behind by kexec.
//
// Copyright 2026 NewFW Technology
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

#include <Register/RiscV64/RiscVImpl.h>

//
// sstatus.SPP is set when a trap is delivered from S-mode and cleared by sret.
// Linux kexec jumps here without sret, so SPP can stay set. EnableInterrupts()
// treats a set SPP as "inside a trap handler" and skips SIE, which leaves the
// supervisor timer interrupt masked and BDS waiting forever.
//
ASM_FUNC (RiscVClearSupervisorTrapShadow)
    li    a0, SSTATUS_SPP
    csrc  CSR_SSTATUS, a0
    ret