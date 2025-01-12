#------------------------------------------------------------------------------
#
# UefiCpu AArch64 architecture support library.
#
# Copyright 2024 Google LLC
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroLib.h>

// VOID
// EFIAPI
// InitializeFloatingPointUnits (
//   VOID
//   )

ASM_FUNC(InitializeFloatingPointUnits)
   ret

// UINTN
// ArchGetPhysicalAddressBits (
//   VOID
//   )
ASM_FUNC(ArchGetPhysicalAddressBits)
  // Read ID_AA64MMFR0_EL1.PARange (ID_AA64MMFR0_EL1[3:0])
  mrs x0, ID_AA64MMFR0_EL1
  and x0, x0, #0xF

  // Compare PARange values and return corresponding address size
  cmp x0, #0             // If PARange == 0
  b.eq return_32         // 32 bits
  cmp x0, #1             // If PARange == 1
  b.eq return_36         // 36 bits
  cmp x0, #2             // If PARange == 2
  b.eq return_40         // 40 bits
  cmp x0, #3             // If PARange == 3
  b.eq return_42         // 42 bits
  cmp x0, #4             // If PARange == 4
  b.eq return_44         // 44 bits
  cmp x0, #5             // If PARange == 5
  b.eq return_48         // 48 bits
  cmp x0, #6             // If PARange == 6
  b.eq return_52         // 52 bits

  // Returns 44 as default
  mov x0, #44
  ret

return_32:
  mov x0, #32
  ret
return_36:
  mov x0, #36
  ret
return_40:
  mov x0, #40
  ret
return_42:
  mov x0, #42
  ret
return_44:
  mov x0, #44
  ret
return_48:
  mov x0, #48
  ret
return_52:
  mov x0, #52
  ret
