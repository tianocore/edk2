/*
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

 */

#include "PlatformSecLib.h"

ASM_FUNC (_ModuleEntryPoint)
/* Prevent stack unwinding from going further */
li    s0, 0

/* Use Temp memory as the stack for calling to C code */
li    a2, FixedPcdGet32 (PcdOvmfSecPeiTempRamBase)
li    a3, FixedPcdGet32 (PcdOvmfSecPeiTempRamSize)

/* Reserve region to store handoff data
li    a4, SEC_HANDOFF_DATA_RESERVE_SIZE
sub   a3, a3, a4

/* Use Temp memory as the stack for calling to C code */
add   sp, a2, a3

call SecStartupPlatform
