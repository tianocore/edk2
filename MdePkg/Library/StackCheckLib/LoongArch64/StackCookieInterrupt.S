#------------------------------------------------------------------------------
#
# LoongArch64/StackCookieInterrupt.S
#
# Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <Library/BaseLib.h>
ASM_GLOBAL ASM_PFX(TriggerStackCookieInterrupt)

#/**
#  Calling `CpuBreakpoint` to create a breakpoint to the LoongArch64 exception handle and stop.
#**/

ASM_PFX(TriggerStackCookieInterrupt):
  bl    CpuBreakpoint
  jirl  $zero, $ra, 0
.end
