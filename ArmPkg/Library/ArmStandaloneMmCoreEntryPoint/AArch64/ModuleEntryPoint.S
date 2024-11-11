#------------------------------------------------------------------------------
#
# Entrypoint of StandaloneMm.
#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# @par Reference(s):
#   - [1] SPM based on the MM interface.
#        (https://trustedfirmware-a.readthedocs.io/en/latest/components/
#         secure-partition-manager-mm.html)
#   - [2] Arm Firmware Framework for Armv8-A, DEN0077, version 1.2
#        (https://developer.arm.com/documentation/den0077/latest/)
#   - [3] FF-A Memory Management Protocol for Armv8-A, DEN0140, version 1.2
#        (https://developer.arm.com/documentation/den0140/latest/)
#
#------------------------------------------------------------------------------

#include <AArch64/AsmMacroLib.h>
#include <IndustryStandard/ArmMmSvc.h>
#include <IndustryStandard/ArmFfaSvc.h>
#include <Uefi/UefiBaseType.h>

.data
.section .data.stmm_stack, "aw"
.align 12
// Define a data section to be used for setting up the
// stack for StandaloneMm
stmm_stack:
.zero FixedPcdGet32 (PcdStMmStackSize)

.text
//
// Check whether it is possible to use FF-A.
// If FF-A is supported return TRUE, otherwise return FALSE.
//
// BOOLEAN
// EFIAPI
// CheckFfaSupport (
//   VOID
//   )
//
ASM_FUNC(CheckFfaSupport)
  //
  // Try to check FF-A support via FFA_VERSION
  // See [2], Section 13.2 FFA_VERSION
  //
  MOV32 (x0, ARM_FID_FFA_VERSION)

  // Set x1 as request version.
  MOV32 (x1, ARM_FFA_CREATE_VERSION (
               ARM_FFA_MAJOR_VERSION,
               ARM_FFA_MINOR_VERSION))

  svc #0

  MOV64 (x9, ARM_FFA_RET_NOT_SUPPORTED)
  cmp x0, x9
  cset x0, ne
  mov x9, xzr
  ret

//
// Set write memory permission on StandaloneMm stack area via FF-A request.
// If success, return StMmStackBaseAddr. otherwise return 0.
//
// UINTN
// EFIAPI
// SetStackPermissionFfa (
//   IN UINTN StMmStackTopAddr
//   )
//
ASM_FUNC(SetStackPermissionFfa)
  //
  // Try to set write permission on stmm_stack with FF-A request
  // See [3], Section 2.9 FFA_MEM_PERM_SET
  //
  MOV32 (x2, FixedPcdGet32 (PcdStMmStackSize))

  // x1 = stmm_stack top
  mov x1, x0

  // x12 = Compute and save the stack base
  add x12, x1, x2

  // x2 = Count of pages of stmm_stack
  lsr x2, x2, #EFI_PAGE_SHIFT

  // x3 = Memory permission
  MOV32 (x3,
    ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
      ARM_FFA_SET_MEM_ATTR_DATA_PERM_RW,
      ARM_FFA_SET_MEM_ATTR_CODE_PERM_XN))

  MOV32 (x0, ARM_FID_FFA_MEM_PERM_SET)

  // Call FFA_MEM_PERM_SET to set stmm_stack with write permission
  // See [3], Section 2.9 FFA_MEM_PERM_SET
  svc #0

  // Check FFA_MEM_PERM_SET operation is success.
  mov x10, #0x00
  MOV32 (x11, ARM_FID_FFA_SUCCESS_AARCH64)
  cmp x0, x11
  cinc x10, x10, eq
  MOV32 (x11, ARM_FID_FFA_SUCCESS_AARCH32)
  cmp x0, x11
  cinc x10, x10, eq
  cmp x10, #0x00

  // Set return value as base address of stack.
  mov x0, x12
  b.ne .Lout_set_stack_perm_ffa
  // If failed, set return value as zero.
  mov x0, #0x00

.Lout_set_stack_perm_ffa:
  mov x9, xzr
  mov x10, xzr
  mov x11, xzr
  mov x12, xzr
  ret

//
// Set write memory permission on StandaloneMm stack area via SpmMm.
// If success, return StMmStackTopAddr. otherwise return 0.
//
// UINTN
// EFIAPI
// SetStackPermissionSpmMm (
//   IN UINTN StMmStackTopAddr
//   )
//
ASM_FUNC(SetStackPermissionSpmMm)
  //
  // Try to set write permission on stmm_stack with SPM_MM request
  // See [1], Section 4.16.5.5.1 MM_SP_MEMORY_ATTRIBUTES_SET_AARCH64.
  //
  MOV32 (x2, FixedPcdGet32 (PcdStMmStackSize))

  // x1 = stmm_stack top
  mov x1, x0

  // x12 = Compute and save the stack base
  add x12, x1, x2

  // x2 = Count of pages of stmm_stack
  lsr x2, x2, #EFI_PAGE_SHIFT

  // x3 = Memory permission
  MOV32 (x3,
    ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST (
      ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_RW,
      ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_XN))

  MOV32 (x0, ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES)

  // Call SPM_MM_SP_SET_MEM_ATTRIBUTES to set stmm_stack with write permission
  // See [1], Section 4.16.5.5.1 MM_SP_MEMORY_ATTRIBUTES_SET_AARCH64.
  svc #0

  cmp x0, #ARM_SPM_MM_RET_SUCCESS

  // Set return value as base address of stack.
  mov x0, x12
  b.eq .Lout_set_stack_perm
  // If failed, set return value as zero.
  mov x0, #0x00

.Lout_set_stack_perm:
  mov x9, xzr
  mov x12, xzr
  ret

//
// Entry point of StandaloneMm
//
ASM_FUNC(_ModuleEntryPoint)
  // Stash boot information registers from the SPMC
  mov x19, x0
  mov x20, x1
  mov x21, x2
  mov x22, x3
  mov x23, x4

  bl CheckFfaSupport
  mov x1, x0

  // Get StandaloneMm Stack top address and save in x0
  adrp x4, stmm_stack
  mov x0, x4

  // Set stack permission
  cmp x1, #0x01
  b.eq .Lset_stack_perm_ffa
  b.ne .Lset_stack_perm_spm

  // If SetStackPermission* failed, x0 is #0x00.
  // Otherwise, x0 is base address of stack.
.Lset_stmm_sp:
  cmp x0, #0x00
  b.eq .Lerror

  mov sp, x0

  // Restore boot information registers from the SPMC
  mov x0, x19
  mov x1, x20
  mov x2, x21
  mov x3, x22
  mov x4, x23
  mov x19, xzr
  mov x20, xzr
  mov x21, xzr
  mov x22, xzr
  mov x23, xzr

  // Invoke the C entrypoint
  b     CEntryPoint

.Lerror:
  b .

.Lset_stack_perm_ffa:
  bl SetStackPermissionFfa
  b .Lset_stmm_sp

.Lset_stack_perm_spm:
  bl SetStackPermissionSpmMm
  b .Lset_stmm_sp
