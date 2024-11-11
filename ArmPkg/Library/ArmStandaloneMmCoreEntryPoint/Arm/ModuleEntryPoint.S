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

#include <Arm/AsmMacroLib.h>
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
// If FF-A can use, return TRUE. otherwise return FALSE.
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
  MOV32 (r0, ARM_FID_FFA_VERSION)

  // Set r1 as request version.
  MOV32 (r1, ARM_FFA_CREATE_VERSION (
               ARM_FFA_MAJOR_VERSION,
               ARM_FFA_MINOR_VERSION))

  svc #0

  // Set r4 as ARM_FFA_RET_NOT_SUPPORTED (-1)
  mvn r4, #0x00

  cmp r0, r4
  movne r0, #0x01
  moveq r0, #0x00
  mov r4, #0x00
  bx lr

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
  MOV32 (r2, FixedPcdGet32 (PcdStMmStackSize))

  // r1 = stmm_stack top
  mov r1, r0

  // r12 = Compute and save the stack base
  add r12, r1, r2

  // r2 = Count of pages of stmm_stack
  lsr r2, r2, #EFI_PAGE_SHIFT

  // r3 = Memory permission
  MOV32 (r3,
    ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST (
      ARM_FFA_SET_MEM_ATTR_DATA_PERM_RW,
      ARM_FFA_SET_MEM_ATTR_CODE_PERM_XN))

  MOV32 (r0, ARM_FID_FFA_MEM_PERM_SET)

  // Call FFA_MEM_PERM_SET to set stmm_stack with write permission
  // See [3], Section 2.9 FFA_MEM_PERM_SET
  svc #0

  // Check FFA_MEM_PERM_SET operation is success.
  MOV32 (r5, ARM_FID_FFA_SUCCESS_AARCH32)
  cmp r0, r5

  // Set return value as base address of stack.
  mov r0, r12
  bne .Lout_set_stack_perm_ffa
  // If failed, set return value as zero.
  mov r0, #0x00

.Lout_set_stack_perm_ffa:
  // Initialise SP with temp stack
  mov r5, #0x00
  mov r12, #0x00
  bx lr

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
  MOV32 (r2, FixedPcdGet32 (PcdStMmStackSize))

  // r1 = stmm_stack top
  mov r1, r0

  // r12 = Compute and save the stack base
  add r12, r1, r2

  // r2 = Count of pages of stmm_stack
  lsr r2, r2, #EFI_PAGE_SHIFT

  // r3 = Memory permission
  MOV32 (r3,
    ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST (
      ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_RW,
      ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_XN))

  MOV32 (r0, ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES)

  // Call SPM_MM_SP_SET_MEM_ATTRIBUTES to set stmm_stack with write permission
  // See [1], Section 4.16.5.5.1 MM_SP_MEMORY_ATTRIBUTES_SET_AARCH64.
  svc #0

  MOV32 (r5, ARM_SPM_MM_RET_SUCCESS)
  cmp r0, r5

  // Set return value as base address of stack.
  mov r0, r12
  beq .Lout_set_stack_perm
  // If failed, set return value as zero.
  mov r0, #0x00

.Lout_set_stack_perm:
  mov r5, #0x00
  mov r12, #0x00
  bx lr

//
// Entry point of StandaloneMm
//
ASM_FUNC(_ModuleEntryPoint)
  // Stash boot information registers from the SPMC
  mov r8, r0
  mov r9, r1
  mov r10, r2
  mov r11, r3

  bl CheckFfaSupport
  mov r1, r0

  // Get StandaloneMm Stack top address and save in x0
  LDRL(r0, stmm_stack)

  // Set stack permission
  cmp r1, #0x01
  beq .Lset_stack_perm_ffa
  bne .Lset_stack_perm_spm

  // If SetStackPermission* failed, x0 is #0x00.
  // Otherwise, x0 is base address of stack.
.Lset_stmm_sp:
  cmp r0, #0x00
  bne .Lerror

  mov sp, r0

  // Restore boot information registers from the SPMC
  mov r3, r11
  mov r2, r10
  mov r1, r9
  mov r0, r8

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
