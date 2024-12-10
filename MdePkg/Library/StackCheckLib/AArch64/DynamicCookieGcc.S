#------------------------------------------------------------------------------
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# Module Name:
#
#   DynamicCookie.S
#
# Abstract:
#
#   Generates random number through the RNDR instruction on a 64-bit AARCH64 platform
#   to store a random value in the GCC __stack_check_guard stack cookie.
#   The first byte is 0'd to prevent string copy functions from clobbering
#   the stack cookie.
#
# Notes:
#
# If RNDR fails, the build time static stack cookie value will be used instead.
#
#------------------------------------------------------------------------------

.text
.p2align 2

GCC_ASM_IMPORT(__stack_chk_guard)
GCC_ASM_IMPORT(_CModuleEntryPoint)
GCC_ASM_EXPORT(_ModuleEntryPoint)

#define RNDR  S3_3_C2_C4_0

#------------------------------------------------------------------------------
# VOID
# EFIAPI
# _ModuleEntryPoint (
#   Parameters are passed through.
# )
#------------------------------------------------------------------------------
ASM_PFX(_ModuleEntryPoint):
  AARCH64_BTI(c)
  stp x29, x30, [sp, #-16]!         // Save frame pointer and link register
  stp x19, x20, [sp, #-16]!         // Save x19 and x20
  stp x21, x22, [sp, #-16]!         // Save x21 and x22
  stp x23, x24, [sp, #-16]!         // Save x23 and x24
  stp x25, x26, [sp, #-16]!         // Save x25 and x26
  stp x27, x28, [sp, #-16]!         // Save x27 and x28
  stp x0, x1, [sp, #-16]!           // Save x0 and x1
  stp x2, x3, [sp, #-16]!           // Save x2 and x3
  stp x4, x5, [sp, #-16]!           // Save x4 and x5
  stp x6, x7, [sp, #-16]!           // Save x6 and x7

  mrs x0, ID_AA64ISAR0_EL1          // Read the AArch64 Instruction Set Attribute Register 0
  ubfx x0, x0, #60, #4              // Extract the RNDR bit field (bits 60-63)
  cbz x0, c_entry                   // If RNDR is not supported, jump to c_entry

  mov x0, #0                        // Initialize x0 to 0
  mrs x0, RNDR                      // Generate a random number
  b.eq c_entry                      // RNDR sets NZCV to 0b0100 on failure
                                    // So if the zero flag is set, use the static stack guard

  adrp x1, ASM_PFX(__stack_chk_guard)          // Load the page address of __stack_chk_guard
  add x1, x1, :lo12:ASM_PFX(__stack_chk_guard) // Add the offset within the page

  and x0, x0, #0x00FFFFFFFFFFFFFF   // Zero the first byte of the random value
  str x0, [x1]                      // Store the random value in __stack_chk_guard

c_entry:
  ldp x6, x7, [sp], #16             // Restore x6 and x7
  ldp x4, x5, [sp], #16             // Restore x4 and x5
  ldp x2, x3, [sp], #16             // Restore x2 and x3
  ldp x0, x1, [sp], #16             // Restore x0 and x1
  ldp x27, x28, [sp], #16           // Restore x27 and x28
  ldp x25, x26, [sp], #16           // Restore x25 and x26
  ldp x23, x24, [sp], #16           // Restore x23 and x24
  ldp x21, x22, [sp], #16           // Restore x21 and x22
  ldp x19, x20, [sp], #16           // Restore x19 and x20
  ldp x29, x30, [sp], #16           // Restore frame pointer and link register
  b ASM_PFX(_CModuleEntryPoint)     // Jump to the C module entry point
