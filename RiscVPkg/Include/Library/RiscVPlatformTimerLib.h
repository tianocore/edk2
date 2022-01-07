/** @file
  RISC-V Platform Timer library definitions.

  Copyright (c) 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RISCV_PLATFORM_TIMER_LIB_H_
#define RISCV_PLATFORM_TIMER_LIB_H_

UINT64
RiscVReadMachineTimer (
  VOID
  );

VOID
  RiscVSetMachineTimerCmp (UINT64);

UINT64
RiscVReadMachineTimerCmp (
  VOID
  );

#endif
