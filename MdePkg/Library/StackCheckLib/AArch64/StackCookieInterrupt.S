//------------------------------------------------------------------------------
// AArch64/StackCookieInterrupt.S
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//------------------------------------------------------------------------------

#include <Library/StackCheckLib.h>

    .text

//------------------------------------------------------------------------------
// Calls an interrupt using the stack cookie exception vector
// and passes the ExceptionAddress through in x0.
//
// VOID
// EFIAPI
// TriggerStackCookieInterrupt (
//   EFI_PHYSICAL_ADDRESS ExceptionAddress
//   );
//------------------------------------------------------------------------------
.global ASM_PFX(TriggerStackCookieInterrupt)
ASM_PFX(TriggerStackCookieInterrupt):
    AARCH64_BTI(c)
    svc STACK_CHECK_EXCEPTION_VECTOR
    ret
