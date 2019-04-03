;/** @file
;  ResetSystemLib implementation using PSCI calls
;
;  Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>
;
;  SPDX-License-Identifier: BSD-2-Clause-Patent
;
;**/

  AREA Reset, CODE, READONLY

  EXPORT DisableMmuAndReenterPei
  IMPORT ArmDisableMmu

DisableMmuAndReenterPei
  stp   x29, x30, [sp, #-16]!
  mov   x29, sp

  bl    ArmDisableMmu

  ; no memory accesses after MMU and caches have been disabled

  movl  x0, FixedPcdGet64 (PcdFvBaseAddress)
  blr   x0

  ; never returns
  nop

  END
