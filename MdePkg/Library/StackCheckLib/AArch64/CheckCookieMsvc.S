//------------------------------------------------------------------------------
// AArch64/CheckCookieMsvc.S
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//------------------------------------------------------------------------------

#include <AArch64/AsmMacroLib.h>

    .text

//------------------------------------------------------------------------------
// Checks the stack cookie value against __security_cookie and calls the
// stack cookie failure handler if there is a mismatch.
//
// VOID
// EFIAPI
// __security_check_cookie (
//   IN UINTN CheckValue
//   );
//------------------------------------------------------------------------------
.global ASM_PFX(__security_check_cookie)
ASM_PFX(__security_check_cookie):
    AARCH64_BTI(c)
    LDR_LIT(x9, ASM_PFX(__security_cookie))
    cmp    x0, x9
    b.ne   ASM_PFX(StackCheckFailure)
    ret
