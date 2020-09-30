;------------------------------------------------------------------------------
;
; ArmRndr() and ArmRndrrs() for AArch64
;
; Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

#include "BaseLibInternals.h"

  EXPORT ArmRndr
  EXPORT ArmRndrrs
  AREA BaseLib_LowLevel, CODE, READONLY


;/**
;  Generates a random number using RNDR.
;  Returns TRUE on success; FALSE on failure.
;
;**/
;BOOLEAN
;EFIAPI
;ArmRndr (
;  OUT UINT64 *Rand
;  );
;
ArmRndr
  mrs  x1, RNDR
  str  x1, [x0]
  cset x0, ne    // RNDR sets NZCV to 0b0100 on failure
  ret

  END

;/**
;  Generates a random number using RNDRRS.
;  Returns TRUE on success; FALSE on failure.
;
;**/
;BOOLEAN
;EFIAPI
;ArmRndrrs (
;  OUT UINT64 *Rand
;  );
;
ArmRndrrs
  mrs  x1, RNDRRS
  str  x1, [x0]
  cset x0, ne    // RNDRRS sets NZCV to 0b0100 on failure
  ret

  END

