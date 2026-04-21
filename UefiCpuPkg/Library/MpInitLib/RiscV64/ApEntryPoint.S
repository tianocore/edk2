// ------------------------------------------------------------------------------
//
// AP entry function
//
// Copyright (C) 2026 Qualcomm Technologies, Inc. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// ------------------------------------------------------------------------------
.align 3

#include "MpLib.h"

#VOID
#EFIAPI
#ApEntryPoint (
#IN UINTN StackBase
#) ;

  .globl ApEntryPoint
ApEntryPoint:
#Pointer of CpuData is stored in the top of each AP's stack.
addi  sp, a1, -8
ld a0, -8 (a1)
j ApWakeupFunction
