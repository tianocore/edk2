/** @file
  ResetSystemLib implementation using PSCI calls

  Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <AsmMacroIoLib.h>

ASM_FUNC(DisableMmuAndReenterPei)
  push  {lr}

  bl    ArmDisableMmu

  // no memory accesses after MMU and caches have been disabled

  MOV32 (r0, FixedPcdGet64 (PcdFvBaseAddress))
  blx   r0

  // never returns
  nop
