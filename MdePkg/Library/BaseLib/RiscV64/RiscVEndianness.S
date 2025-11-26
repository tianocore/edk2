//------------------------------------------------------------------------------
//
// RISC-V Supervisor Mode get endianness
//
// Copyright (c) 2020, Xiang W <wangxiang@iscas.ac.cn>. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

#include <Register/RiscV64/RiscVImpl.h>

ASM_FUNC(RiscVGetSupervisorEndianness)
  csrr  a0, CSR_SSTATUS
  li    a1, SSTATUS_SBE
  and   a0, a0, a1
  ret
