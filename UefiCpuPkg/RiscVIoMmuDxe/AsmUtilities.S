//------------------------------------------------------------------------------
//
// RISC-V CSR read/write functions
//
// Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

#include <Register/RiscV64/RiscVImpl.h>

//
// Get the contents of the Supervisor Status Register.
//
ASM_FUNC (RiscVGetSupervisorStatusRegister)
    csrr  a0, CSR_SSTATUS
    ret
