;/** @file
;  ResetSystemLib implementation using PSCI calls
;
;  Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>
;
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

  INCLUDE AsmMacroExport.inc
  PRESERVE8

  IMPORT ArmDisableMmu

RVCT_ASM_EXPORT DisableMmuAndReenterPei
  push  {lr}

  bl    ArmDisableMmu

  ; no memory accesses after MMU and caches have been disabled

  mov32 r0, FixedPcdGet64 (PcdFvBaseAddress)
  blx   r0

  ; never returns
  nop

  END
