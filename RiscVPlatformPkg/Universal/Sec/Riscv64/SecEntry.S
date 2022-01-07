/*
  Copyright (c) 2021-2022 , Hewlett Packard Enterprise Development LP. All rights reserved.
  Copyright (c) 2019 Western Digital Corporation or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

 */

#include <IndustryStandard/RiscVOpensbi.h>
#include <Base.h>
#include <RiscVImpl.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>

#include <SecMain.h>

.macro  MOV_3R __d0, __s0, __d1, __s1, __d2, __s2
        add     \__d0, \__s0, zero
        add     \__d1, \__s1, zero
        add     \__d2, \__s2, zero
.endm

.text
.align 3

    .globl _start_warm

ASM_FUNC (_ModuleEntryPoint)
  /*
   * Jump to warm-boot if this is not the selected core booting,
   */
  csrr  a6, CSR_MHARTID
  li    a5, FixedPcdGet32 (PcdBootHartId)
  bne   a6, a5, _wait_for_boot_hart

  /*
   * Initial the hart count reported in DTB
   */
  li    a4, FixedPcdGet32 (PcdTemporaryRamBase)
  li    a5, FixedPcdGet32 (PcdTemporaryRamSize)

  /* Use Temp memory as the stack for calling to C code */
  add   sp, a4, a5
  /* Get the address of device tree and call generic fw_platform_init */
  call  GetDeviceTreeAddress /* a0 return the device tree address */
  beqz  a0, skip_fw_init
  add   a1, a0, 0            /* a1 is device tree */
  csrr  a0, CSR_MHARTID      /* a0 is boot hart ID */
  call  fw_platform_init
skip_fw_init:
  /* Preload HART details
   * s7 -> Total HART count from PCD or DTB
   * s8 -> HART Stack Size
   */
  la    a0, platform
#if __riscv_xlen == 64
  lwu   s7, SBI_PLATFORM_HART_COUNT_OFFSET(a0)
#else
  lw    s7, SBI_PLATFORM_HART_COUNT_OFFSET(a0)
#endif
  /*
   * This is the number of HARTs described in
   * DTB for this processor. We allocate the
   * scratch buffer according to this number.
   */
  la    a4, _physical_hart_count
  sd    s7, (a4)

  li    s8, FixedPcdGet32 (PcdOpenSbiStackSize)

  /*
   * Setup scratch space for all the HARTs
   *
   * Scratch buffer is on the top of stack buffer
   * of each hart.
   *
   * tp : Base address of scratch buffer.
   * s7 : HART Count
   * s8 : HART Stack Size
  */
  li    tp, FixedPcdGet32 (PcdScratchRamBase)
  mul   a5, s7, s8
  add   tp, tp, a5

  /* Keep a copy of tp */
  add   t3, tp, zero
  /* Counter */
  li    t2, 1
  /* hartid 0 is mandated by ISA */
  li    t1, 0
_scratch_init:
  add   tp, t3, zero
  mul   a5, s8, t1
  sub   tp, tp, a5
  li    a5, SBI_SCRATCH_SIZE
  sub   tp, tp, a5

  /* Initialize scratch space */

  /* Firmware range and size */
  li    a4, FixedPcdGet32 (PcdRootFirmwareDomainBaseAddress)
  li    a5, FixedPcdGet32 (PcdRootFirmwareDomainSize)
  sd    a4, SBI_SCRATCH_FW_START_OFFSET(tp)
  sd    a5, SBI_SCRATCH_FW_SIZE_OFFSET(tp)

  /*
   Note: fw_next_addr()uses a0, a1, and ra
  */
  call  fw_next_addr
  sd    a0, SBI_SCRATCH_NEXT_ADDR_OFFSET(tp) /* Save next address in scratch buffer*/
  li    a4, PRV_S
  sd    a4, SBI_SCRATCH_NEXT_MODE_OFFSET(tp) /* Save next mode in scratch buffer*/
  la    a4, _start_warm
  sd    a4, SBI_SCRATCH_WARMBOOT_ADDR_OFFSET(tp) /* Save warm boot address in scratch buffer*/
  la    a4, platform
  sd    a4, SBI_SCRATCH_PLATFORM_ADDR_OFFSET(tp) /* Save platfrom table in scratch buffer*/
  la    a4, _hartid_to_scratch
  sd    a4, SBI_SCRATCH_HARTID_TO_SCRATCH_OFFSET(tp) /* Save _hartid_to_scratch function in scratch buffer*/
  sd    zero, SBI_SCRATCH_TMP0_OFFSET(tp)
  /* Store trap-exit function address in scratch space */
  lla   a4, _trap_exit
  sd    a4, SBI_SCRATCH_TRAP_EXIT_OFFSET(tp)
  /* Clear tmp0 in scratch space */
  sd    zero, SBI_SCRATCH_TMP0_OFFSET(tp)
#ifdef FW_OPTIONS
  li    a4, FW_OPTIONS
  sd    a4, SBI_SCRATCH_OPTIONS_OFFSET(tp)
#else
  sd    zero, SBI_SCRATCH_OPTIONS_OFFSET(tp)
#endif
  add   t1, t1, t2
  /* Loop to next hart */
  blt   t1, s7, _scratch_init

  li    a4, FixedPcdGet32 (PcdTemporaryRamBase)
  li    a5, FixedPcdGet32 (PcdTemporaryRamSize)
  /* Use Temp memory as the stack for calling to C code */
  add   sp, a4, a5
  /* Zero out temporary memory */
  add   a5, a4, a5
1:
  li    a3, 0x0
  sd    a3, (a4)
  add   a4, a4, __SIZEOF_POINTER__
  blt   a4, a5, 1b

  /* Overwrite hart_index2id array of
     platform would like to use the bootable hart
     other than it defined in Device Tree */
  la    a0, platform
  call  Edk2PlatformHartIndex2Id

  /* Update boot hart flag */
  la    a4, _boot_hart_done
  li    a5, 1
  sd    a5, (a4)

  /* Wait for boot hart */
_wait_for_boot_hart:
  la    a4, _boot_hart_done
  ld    a5, (a4)

  /* Reduce the bus traffic so that boot hart may proceed faster */
  nop
  nop
  nop
  beqz  a5, _wait_for_boot_hart

_start_warm:
  li    ra, 0
  call  _reset_regs

  /* Disable and clear all interrupts */
  csrw  CSR_MIE, zero
  csrw  CSR_MIP, zero

  li    s7, FixedPcdGet32 (PcdBootableHartNumber)
  bnez  s7, 1f
  la    a4, platform
#if __riscv_xlen == 64
  lwu   s7, SBI_PLATFORM_HART_COUNT_OFFSET(a4)
#else
  lw    s7, SBI_PLATFORM_HART_COUNT_OFFSET(a4)
#endif
1:
  li    s8, FixedPcdGet32 (PcdOpenSbiStackSize)
  la    a4, platform

  /* s9 is hart_index2id array */
  REG_L s9, SBI_PLATFORM_HART_INDEX2ID_OFFSET(a4)

  /* s6 is this hart ID */
  csrr  s6, CSR_MHARTID

  /*
   * Convert hart ID to scratch buffer
   * Because the hert ID maybe not in sequentially
   */
  beqz  s9, 3f
  li    a4, 0
1:
#if __riscv_xlen == 64
  lwu   a5, (s9)
#else
  lw    a5, (s9)
#endif
  /* Branch if hart ID is matched */
  beq   a5, s6, 2f
  add   s9, s9, 4
  add   a4, a4, 1
  /* Loop to next index */
  blt   a4, s7, 1b
  li    a4, -1
2:
  add   s6, a4, zero
3:
  /* Jump to UninitializedHartWait for the non-bootable harts.
     Be aware that the stack and scratch is not set
     at this moment for this hart even the resource
     is preserved eariler for it.
  */
  bltu  s6, s7, 4f
  csrr  a0, CSR_MHARTID
  j _uninitialized_hart_wait
4:
  la    a5, _physical_hart_count
  ld    s7, (a5)
  /* Find the scratch space for this hart
   *
   * Scratch buffer is on the top of stack buffer
   * reserved for opensbi.
   *
   * tp: The base address of scratch buffer
   * s6: Index to scratch buffer fot this hart
   * s7: Total hart number
   * s8: Stack size for opebsbi.
  */
  li    tp, FixedPcdGet32 (PcdScratchRamBase)
  mul   a5, s7, s8
  add   tp, tp, a5
  mul   a5, s8, s6
  sub   tp, tp, a5
  li    a5, SBI_SCRATCH_SIZE
  sub   tp, tp, a5

  /* update the mscratch */
  csrw  CSR_MSCRATCH, tp

  /*make room for Hart specific Firmware Context*/
  li    a5, FIRMWARE_CONTEXT_HART_SPECIFIC_SIZE
  sub   tp, tp, a5

  /* Setup stack */
  add   sp, tp, zero

  /* Setup stack for the boot hart executing EFI to top of temporary ram*/
  csrr  a6, CSR_MHARTID
  li    a5, FixedPcdGet32 (PcdBootHartId)
  bne   a6, a5, 1f

  li    a4, FixedPcdGet32(PcdTemporaryRamBase)
  li    a5, FixedPcdGet32(PcdTemporaryRamSize)
  add   sp, a4, a5
1:

  /* Setup trap handler */
  la    a4, _trap_handler
  csrw  CSR_MTVEC, a4

  /* Make sure that mtvec is updated */
  1:
  csrr  a5, CSR_MTVEC
  bne   a4, a5, 1b

  /* Call library constructors before jup to SEC core */
  call  ProcessLibraryConstructorList

  /* Jump to SEC Core C
   * a0: HART ID
   * a1: Scratch pointer
  */
  csrr  a0, CSR_MHARTID
  csrr  a1, CSR_MSCRATCH
  call  SecCoreStartUpWithStack

  /* We do not expect to reach here hence just hang */
  j     _start_hang

  .align 3
  .section .data, "aw"
_boot_hart_done:
  RISCV_PTR 0
_physical_hart_count:
  RISCV_PTR 0

  .align 3
  .section .entry, "ax", %progbits
  .globl _hartid_to_scratch

  /*
   * a0 -> HART id (Obsoleted, keep this for the backward compatible)
   * a1 -> HART index (0-based) of this hart id
   */
_hartid_to_scratch:
  add   sp, sp, -(3 * __SIZEOF_POINTER__)
  sd    s0, (sp)
  sd    s1, (__SIZEOF_POINTER__)(sp)
  sd    s2, (__SIZEOF_POINTER__ * 2)(sp)

  /*
   * s0 -> HART Stack Size
   * s1 -> HART Stack End
   * s2 -> Total hart count
   */
  la    s2, platform
#if __riscv_xlen == 64
  lwu   s0, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(s2)
#else
  lw    s0, SBI_PLATFORM_HART_STACK_SIZE_OFFSET(s2)
#endif

  la    s1, _physical_hart_count /* total HART count */
  ld    s2, (s1)
  mul   s2, s2, s0
  li    s1, FixedPcdGet32 (PcdScratchRamBase)
  add   s1, s1, s2
  mul   s2, s0, a1
  sub   s1, s1, s2
  li    s2, SBI_SCRATCH_SIZE
  sub   a0, s1, s2
  ld    s0, (sp)
  ld    s1, (__SIZEOF_POINTER__)(sp)
  ld    s2, (__SIZEOF_POINTER__ * 2)(sp)
  add   sp, sp, (3 * __SIZEOF_POINTER__)
  ret

  .align 3
  .section .entry, "ax", %progbits
  .globl _start_hang
_start_hang:
  wfi
  j     _start_hang

  /*
   * Uninitialized hart comes here and wait
   * for the further process.
   * a0: hart ID.
  */
_uninitialized_hart_wait:
  wfi
  j     _uninitialized_hart_wait

.macro  TRAP_SAVE_AND_SETUP_SP_T0
  /* Swap TP and MSCRATCH */
  csrrw   tp, CSR_MSCRATCH, tp

  /* Save T0 in scratch space */
  REG_S   t0, SBI_SCRATCH_TMP0_OFFSET(tp)

  /*
   * Set T0 to appropriate exception stack
   *
   * Came_From_M_Mode = ((MSTATUS.MPP < PRV_M) ? 1 : 0) - 1;
   * Exception_Stack = TP ^ (Came_From_M_Mode & (SP ^ TP))
   *
   * Came_From_M_Mode = 0    ==>    Exception_Stack = TP
   * Came_From_M_Mode = -1   ==>    Exception_Stack = SP
   */
  csrr    t0, CSR_MSTATUS
  srl     t0, t0, MSTATUS_MPP_SHIFT
  and     t0, t0, PRV_M
  slti    t0, t0, PRV_M
  add     t0, t0, -1
  xor     sp, sp, tp
  and     t0, t0, sp
  xor     sp, sp, tp
  xor     t0, tp, t0

  /* Save original SP on exception stack */
  REG_S   sp, (SBI_TRAP_REGS_OFFSET(sp) - SBI_TRAP_REGS_SIZE)(t0)

  /* Set SP to exception stack and make room for trap registers */
  add     sp, t0, -(SBI_TRAP_REGS_SIZE)

  /* Restore T0 from scratch space */
  REG_L   t0, SBI_SCRATCH_TMP0_OFFSET(tp)

  /* Save T0 on stack */
  REG_S   t0, SBI_TRAP_REGS_OFFSET(t0)(sp)

  /* Swap TP and MSCRATCH */
  csrrw   tp, CSR_MSCRATCH, tp
.endm

.macro  TRAP_SAVE_MEPC_MSTATUS have_mstatush
  /* Save MEPC and MSTATUS CSRs */
  csrr    t0, CSR_MEPC
  REG_S   t0, SBI_TRAP_REGS_OFFSET(mepc)(sp)
  csrr    t0, CSR_MSTATUS
  REG_S   t0, SBI_TRAP_REGS_OFFSET(mstatus)(sp)
.if \have_mstatush
  csrr    t0, CSR_MSTATUSH
  REG_S   t0, SBI_TRAP_REGS_OFFSET(mstatusH)(sp)
.else
  REG_S   zero, SBI_TRAP_REGS_OFFSET(mstatusH)(sp)
.endif
.endm

.macro  TRAP_SAVE_GENERAL_REGS_EXCEPT_SP_T0
  /* Save all general regisers except SP and T0 */
  REG_S   zero, SBI_TRAP_REGS_OFFSET(zero)(sp)
  REG_S   ra, SBI_TRAP_REGS_OFFSET(ra)(sp)
  REG_S   gp, SBI_TRAP_REGS_OFFSET(gp)(sp)
  REG_S   tp, SBI_TRAP_REGS_OFFSET(tp)(sp)
  REG_S   t1, SBI_TRAP_REGS_OFFSET(t1)(sp)
  REG_S   t2, SBI_TRAP_REGS_OFFSET(t2)(sp)
  REG_S   s0, SBI_TRAP_REGS_OFFSET(s0)(sp)
  REG_S   s1, SBI_TRAP_REGS_OFFSET(s1)(sp)
  REG_S   a0, SBI_TRAP_REGS_OFFSET(a0)(sp)
  REG_S   a1, SBI_TRAP_REGS_OFFSET(a1)(sp)
  REG_S   a2, SBI_TRAP_REGS_OFFSET(a2)(sp)
  REG_S   a3, SBI_TRAP_REGS_OFFSET(a3)(sp)
  REG_S   a4, SBI_TRAP_REGS_OFFSET(a4)(sp)
  REG_S   a5, SBI_TRAP_REGS_OFFSET(a5)(sp)
  REG_S   a6, SBI_TRAP_REGS_OFFSET(a6)(sp)
  REG_S   a7, SBI_TRAP_REGS_OFFSET(a7)(sp)
  REG_S   s2, SBI_TRAP_REGS_OFFSET(s2)(sp)
  REG_S   s3, SBI_TRAP_REGS_OFFSET(s3)(sp)
  REG_S   s4, SBI_TRAP_REGS_OFFSET(s4)(sp)
  REG_S   s5, SBI_TRAP_REGS_OFFSET(s5)(sp)
  REG_S   s6, SBI_TRAP_REGS_OFFSET(s6)(sp)
  REG_S   s7, SBI_TRAP_REGS_OFFSET(s7)(sp)
  REG_S   s8, SBI_TRAP_REGS_OFFSET(s8)(sp)
  REG_S   s9, SBI_TRAP_REGS_OFFSET(s9)(sp)
  REG_S   s10, SBI_TRAP_REGS_OFFSET(s10)(sp)
  REG_S   s11, SBI_TRAP_REGS_OFFSET(s11)(sp)
  REG_S   t3, SBI_TRAP_REGS_OFFSET(t3)(sp)
  REG_S   t4, SBI_TRAP_REGS_OFFSET(t4)(sp)
  REG_S   t5, SBI_TRAP_REGS_OFFSET(t5)(sp)
  REG_S   t6, SBI_TRAP_REGS_OFFSET(t6)(sp)
.endm

.macro  TRAP_CALL_C_ROUTINE
  /* Call C routine */
  add     a0, sp, zero
  call    sbi_trap_handler
.endm

.macro  TRAP_RESTORE_GENERAL_REGS_EXCEPT_A0_T0
  /* Restore all general regisers except A0 and T0 */
  REG_L   ra, SBI_TRAP_REGS_OFFSET(ra)(a0)
  REG_L   sp, SBI_TRAP_REGS_OFFSET(sp)(a0)
  REG_L   gp, SBI_TRAP_REGS_OFFSET(gp)(a0)
  REG_L   tp, SBI_TRAP_REGS_OFFSET(tp)(a0)
  REG_L   t1, SBI_TRAP_REGS_OFFSET(t1)(a0)
  REG_L   t2, SBI_TRAP_REGS_OFFSET(t2)(a0)
  REG_L   s0, SBI_TRAP_REGS_OFFSET(s0)(a0)
  REG_L   s1, SBI_TRAP_REGS_OFFSET(s1)(a0)
  REG_L   a1, SBI_TRAP_REGS_OFFSET(a1)(a0)
  REG_L   a2, SBI_TRAP_REGS_OFFSET(a2)(a0)
  REG_L   a3, SBI_TRAP_REGS_OFFSET(a3)(a0)
  REG_L   a4, SBI_TRAP_REGS_OFFSET(a4)(a0)
  REG_L   a5, SBI_TRAP_REGS_OFFSET(a5)(a0)
  REG_L   a6, SBI_TRAP_REGS_OFFSET(a6)(a0)
  REG_L   a7, SBI_TRAP_REGS_OFFSET(a7)(a0)
  REG_L   s2, SBI_TRAP_REGS_OFFSET(s2)(a0)
  REG_L   s3, SBI_TRAP_REGS_OFFSET(s3)(a0)
  REG_L   s4, SBI_TRAP_REGS_OFFSET(s4)(a0)
  REG_L   s5, SBI_TRAP_REGS_OFFSET(s5)(a0)
  REG_L   s6, SBI_TRAP_REGS_OFFSET(s6)(a0)
  REG_L   s7, SBI_TRAP_REGS_OFFSET(s7)(a0)
  REG_L   s8, SBI_TRAP_REGS_OFFSET(s8)(a0)
  REG_L   s9, SBI_TRAP_REGS_OFFSET(s9)(a0)
  REG_L   s10, SBI_TRAP_REGS_OFFSET(s10)(a0)
  REG_L   s11, SBI_TRAP_REGS_OFFSET(s11)(a0)
  REG_L   t3, SBI_TRAP_REGS_OFFSET(t3)(a0)
  REG_L   t4, SBI_TRAP_REGS_OFFSET(t4)(a0)
  REG_L   t5, SBI_TRAP_REGS_OFFSET(t5)(a0)
  REG_L   t6, SBI_TRAP_REGS_OFFSET(t6)(a0)
.endm

.macro  TRAP_RESTORE_MEPC_MSTATUS have_mstatush
  /* Restore MEPC and MSTATUS CSRs */
  REG_L   t0, SBI_TRAP_REGS_OFFSET(mepc)(a0)
  csrw    CSR_MEPC, t0
  REG_L   t0, SBI_TRAP_REGS_OFFSET(mstatus)(a0)
  csrw    CSR_MSTATUS, t0
.if \have_mstatush
  REG_L   t0, SBI_TRAP_REGS_OFFSET(mstatusH)(a0)
  csrw    CSR_MSTATUSH, t0
.endif
.endm

.macro TRAP_RESTORE_A0_T0
  /* Restore T0 */
  REG_L   t0, SBI_TRAP_REGS_OFFSET(t0)(a0)

  /* Restore A0 */
  REG_L   a0, SBI_TRAP_REGS_OFFSET(a0)(a0)
.endm

  .section .entry, "ax", %progbits
  .align 3
  .globl _trap_handler
  .globl _trap_exit
_trap_handler:
  TRAP_SAVE_AND_SETUP_SP_T0

  TRAP_SAVE_MEPC_MSTATUS 0

  TRAP_SAVE_GENERAL_REGS_EXCEPT_SP_T0

  TRAP_CALL_C_ROUTINE

_trap_exit:
  TRAP_RESTORE_GENERAL_REGS_EXCEPT_A0_T0

  TRAP_RESTORE_MEPC_MSTATUS 0

  TRAP_RESTORE_A0_T0

  mret

  .align 3
  .section .entry, "ax", %progbits
  .globl _reset_regs
_reset_regs:

  /* flush the instruction cache */
  fence.i

  /* Reset all registers except ra, a0,a1 */
  li    sp, 0
  li    gp, 0
  li    tp, 0
  li    t0, 0
  li    t1, 0
  li    t2, 0
  li    s0, 0
  li    s1, 0
  li    a2, 0
  li    a3, 0
  li    a4, 0
  li    a5, 0
  li    a6, 0
  li    a7, 0
  li    s2, 0
  li    s3, 0
  li    s4, 0
  li    s5, 0
  li    s6, 0
  li    s7, 0
  li    s8, 0
  li    s9, 0
  li    s10, 0
  li    s11, 0
  li    t3, 0
  li    t4, 0
  li    t5, 0
  li    t6, 0
  csrw  CSR_MSCRATCH, 0
  ret

  .align 3
  .section .entry, "ax", %progbits
  .global fw_next_addr
fw_next_addr:
  /* We return next address in 'a0' */
   la   a0, _jump_addr
   ld   a0, (a0)
   ret

  .align 3
  .section .entry, "ax", %progbits
_jump_addr:
RISCV_PTR SecCoreStartUpWithStack
