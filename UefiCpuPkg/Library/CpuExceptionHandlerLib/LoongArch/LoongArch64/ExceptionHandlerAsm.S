#------------------------------------------------------------------------------
#
# LoongArch64 ASM exception handler
#
# Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Register/LoongArch64/Csr.h>

#define RSIZE                 8           // 64 bit mode register size
#define GP_REG_CONTEXT_SIZE   32 * RSIZE  // General-purpose registers size
#define FP_REG_CONTEXT_SIZE   34 * RSIZE  // Floating-point registers size
#define CSR_REG_CONTEXT_SIZE  9  * RSIZE  // CSR registers size

ASM_GLOBAL ASM_PFX(ExceptionEntry)
ASM_GLOBAL ASM_PFX(ExceptionEntryStart)
ASM_GLOBAL ASM_PFX(ExceptionEntryEnd)

ASM_PFX(ExceptionEntry):
  move    $s0, $a0
  bl      GetExceptionType        // Exception type stored in register a0
  move    $a1, $s0                // SystemContxt
  bl      CommonExceptionHandler

PopContext:
  //
  // Not sure if interrupts are turned on during the exception handler, anyway disable interrupts here.
  // It will be turned on when the instruction 'ertn' is executed.
  //
  bl      DisableInterrupts

  move    $a0, $s0                // Restore a0 parameter through s0(EFI_SYSTEM_CONTEXT)
  bl      GetExceptionType        // Get current exception type, and stored in register a0

  // Check whether the FPE is changed during interrupt handler, if ture restore it.
  ld.d    $t1, $sp, (LOONGARCH_CSR_EUEN * RSIZE + GP_REG_CONTEXT_SIZE)
  csrrd   $t0, LOONGARCH_CSR_EUEN        // Current EUEN
  andi    $t0, $t0, CSR_EUEN_FPEN
  andi    $t1, $t1, CSR_EUEN_FPEN
  li.d    $t2, EXCEPT_LOONGARCH_INT
  bne     $a0, $t2, PopRegs
  beq     $t0, $t1, PopRegs
  beqz    $t1, CloseFP
  bl      EnableFloatingPointUnits
  b       PopRegs

CloseFP:
  bl      DisableFloatingPointUnits

PopRegs:
  //
  // Pop CSR reigsters
  //
  addi.d  $sp, $sp, GP_REG_CONTEXT_SIZE

  ld.d    $t0, $sp, LOONGARCH_CSR_CRMD * RSIZE
  csrwr   $t0, LOONGARCH_CSR_CRMD
  ld.d    $t0, $sp, LOONGARCH_CSR_PRMD * RSIZE
  csrwr   $t0, LOONGARCH_CSR_PRMD
  ld.d    $t0, $sp, LOONGARCH_CSR_ECFG * RSIZE
  csrwr   $t0, LOONGARCH_CSR_ECFG
  ld.d    $t0, $sp, LOONGARCH_CSR_ERA * RSIZE
  csrwr   $t0, LOONGARCH_CSR_ERA

  addi.d  $sp, $sp, CSR_REG_CONTEXT_SIZE  // Fource change the stack pointer befor pop the FP registers.

  beqz    $t1, PopGP                      // If the FPE not set, only pop the GP registers.

  //
  // Pop FP registers
  //
  fld.d  $fa0, $sp, 0 * RSIZE
  fld.d  $fa1, $sp, 1 * RSIZE
  fld.d  $fa2, $sp, 2 * RSIZE
  fld.d  $fa3, $sp, 3 * RSIZE
  fld.d  $fa4, $sp, 4 * RSIZE
  fld.d  $fa5, $sp, 5 * RSIZE
  fld.d  $fa6, $sp, 6 * RSIZE
  fld.d  $fa7, $sp, 7 * RSIZE
  fld.d  $ft0, $sp, 8 * RSIZE
  fld.d  $ft1, $sp, 9 * RSIZE
  fld.d  $ft2, $sp, 10 * RSIZE
  fld.d  $ft3, $sp, 11 * RSIZE
  fld.d  $ft4, $sp, 12 * RSIZE
  fld.d  $ft5, $sp, 13 * RSIZE
  fld.d  $ft6, $sp, 14 * RSIZE
  fld.d  $ft7, $sp, 15 * RSIZE
  fld.d  $ft8, $sp, 16 * RSIZE
  fld.d  $ft9, $sp, 17 * RSIZE
  fld.d  $ft10, $sp, 18 * RSIZE
  fld.d  $ft11, $sp, 19 * RSIZE
  fld.d  $ft12, $sp, 20 * RSIZE
  fld.d  $ft13, $sp, 21 * RSIZE
  fld.d  $ft14, $sp, 22 * RSIZE
  fld.d  $ft15, $sp, 23 * RSIZE
  fld.d  $fs0, $sp, 24 * RSIZE
  fld.d  $fs1, $sp, 25 * RSIZE
  fld.d  $fs2, $sp, 26 * RSIZE
  fld.d  $fs3, $sp, 27 * RSIZE
  fld.d  $fs4, $sp, 28 * RSIZE
  fld.d  $fs5, $sp, 29 * RSIZE
  fld.d  $fs6, $sp, 30 * RSIZE
  fld.d  $fs7, $sp, 31 * RSIZE

  ld.d        $t0, $sp, 32 * RSIZE
  movgr2fcsr  $r0, $t0             // Pop the fcsr0 register.

  //
  // Pop the fcc0-fcc7 registers.
  //
  ld.d        $t0, $sp, 33 * RSIZE
  bstrpick.d  $t1, $t0, 7, 0
  movgr2cf    $fcc0, $t1
  bstrpick.d  $t1, $t0, 15, 8
  movgr2cf    $fcc1, $t1
  bstrpick.d  $t1, $t0, 23, 16
  movgr2cf    $fcc2, $t1
  bstrpick.d  $t1, $t0, 31, 24
  movgr2cf    $fcc3, $t1
  bstrpick.d  $t1, $t0, 39, 32
  movgr2cf    $fcc4, $t1
  bstrpick.d  $t1, $t0, 47, 40
  movgr2cf    $fcc5, $t1
  bstrpick.d  $t1, $t0, 55, 48
  movgr2cf    $fcc6, $t1
  bstrpick.d  $t1, $t0, 63, 56
  movgr2cf    $fcc7, $t1

PopGP:
  //
  // Pop GP registers
  //
  addi.d  $sp, $sp, -(GP_REG_CONTEXT_SIZE + CSR_REG_CONTEXT_SIZE)
  ld.d    $ra, $sp, 1 * RSIZE
  ld.d    $tp, $sp, 2 * RSIZE
  ld.d    $a0, $sp, 4 * RSIZE
  ld.d    $a1, $sp, 5 * RSIZE
  ld.d    $a2, $sp, 6 * RSIZE
  ld.d    $a3, $sp, 7 * RSIZE
  ld.d    $a4, $sp, 8 * RSIZE
  ld.d    $a5, $sp, 9 * RSIZE
  ld.d    $a6, $sp, 10 * RSIZE
  ld.d    $a7, $sp, 11 * RSIZE
  ld.d    $t0, $sp, 12 * RSIZE
  ld.d    $t1, $sp, 13 * RSIZE
  ld.d    $t2, $sp, 14 * RSIZE
  ld.d    $t3, $sp, 15 * RSIZE
  ld.d    $t4, $sp, 16 * RSIZE
  ld.d    $t5, $sp, 17 * RSIZE
  ld.d    $t6, $sp, 18 * RSIZE
  ld.d    $t7, $sp, 19 * RSIZE
  ld.d    $t8, $sp, 20 * RSIZE
  ld.d    $r21, $sp, 21 * RSIZE
  ld.d    $fp, $sp, 22 * RSIZE
  ld.d    $s0, $sp, 23 * RSIZE
  ld.d    $s1, $sp, 24 * RSIZE
  ld.d    $s2, $sp, 25 * RSIZE
  ld.d    $s3, $sp, 26 * RSIZE
  ld.d    $s4, $sp, 27 * RSIZE
  ld.d    $s5, $sp, 28 * RSIZE
  ld.d    $s6, $sp, 29 * RSIZE
  ld.d    $s7, $sp, 30 * RSIZE
  ld.d    $s8, $sp, 31 * RSIZE
  ld.d    $sp, $sp, 3 * RSIZE

  ertn // Returen from exception.
//
// End of ExceptionEntry
//

ASM_PFX(ExceptionEntryStart):
  //
  // Store the old stack pointer in preparation for pushing the exception context onto the new stack.
  //
  csrwr   $sp, LOONGARCH_CSR_KS0

  csrrd   $sp, LOONGARCH_CSR_KS0

  //
  // Push GP registers
  //
  addi.d  $sp, $sp, -(GP_REG_CONTEXT_SIZE + FP_REG_CONTEXT_SIZE + CSR_REG_CONTEXT_SIZE)
  st.d    $zero, $sp, 0 * RSIZE
  st.d    $ra, $sp, 1 * RSIZE
  st.d    $tp, $sp, 2 * RSIZE
  st.d    $a0, $sp, 4 * RSIZE
  st.d    $a1, $sp, 5 * RSIZE
  st.d    $a2, $sp, 6 * RSIZE
  st.d    $a3, $sp, 7 * RSIZE
  st.d    $a4, $sp, 8 * RSIZE
  st.d    $a5, $sp, 9 * RSIZE
  st.d    $a6, $sp, 10 * RSIZE
  st.d    $a7, $sp, 11 * RSIZE
  st.d    $t0, $sp, 12 * RSIZE
  st.d    $t1, $sp, 13 * RSIZE
  st.d    $t2, $sp, 14 * RSIZE
  st.d    $t3, $sp, 15 * RSIZE
  st.d    $t4, $sp, 16 * RSIZE
  st.d    $t5, $sp, 17 * RSIZE
  st.d    $t6, $sp, 18 * RSIZE
  st.d    $t7, $sp, 19 * RSIZE
  st.d    $t8, $sp, 20 * RSIZE
  st.d    $r21, $sp, 21 * RSIZE
  st.d    $fp, $sp, 22 * RSIZE
  st.d    $s0, $sp, 23 * RSIZE
  st.d    $s1, $sp, 24 * RSIZE
  st.d    $s2, $sp, 25 * RSIZE
  st.d    $s3, $sp, 26 * RSIZE
  st.d    $s4, $sp, 27 * RSIZE
  st.d    $s5, $sp, 28 * RSIZE
  st.d    $s6, $sp, 29 * RSIZE
  st.d    $s7, $sp, 30 * RSIZE
  st.d    $s8, $sp, 31 * RSIZE
  csrrd   $t0, LOONGARCH_CSR_KS0  // Read the old stack pointer.
  st.d    $t0, $sp, 3 * RSIZE

  //
  // Push CSR registers
  //
  addi.d  $sp, $sp, GP_REG_CONTEXT_SIZE

  csrrd   $t0, LOONGARCH_CSR_CRMD
  st.d    $t0, $sp, LOONGARCH_CSR_CRMD * RSIZE
  csrrd   $t0, LOONGARCH_CSR_PRMD
  st.d    $t0, $sp, LOONGARCH_CSR_PRMD * RSIZE
  csrrd   $t0, LOONGARCH_CSR_EUEN
  st.d    $t0, $sp, LOONGARCH_CSR_EUEN * RSIZE
  csrrd   $t0, LOONGARCH_CSR_MISC
  st.d    $t0, $sp, LOONGARCH_CSR_MISC * RSIZE
  csrrd   $t0, LOONGARCH_CSR_ECFG
  st.d    $t0, $sp, LOONGARCH_CSR_ECFG * RSIZE
  csrrd   $t0, LOONGARCH_CSR_ESTAT
  st.d    $t0, $sp, LOONGARCH_CSR_ESTAT * RSIZE
  csrrd   $t0, LOONGARCH_CSR_ERA
  st.d    $t0, $sp, LOONGARCH_CSR_ERA * RSIZE
  csrrd   $t0, LOONGARCH_CSR_BADV
  st.d    $t0, $sp, LOONGARCH_CSR_BADV * RSIZE
  csrrd   $t0, LOONGARCH_CSR_BADI
  st.d    $t0, $sp, LOONGARCH_CSR_BADI * RSIZE

  //
  // Push FP registers
  //
  addi.d  $sp, $sp, CSR_REG_CONTEXT_SIZE

  csrrd   $t0, LOONGARCH_CSR_EUEN
  andi    $t0, $t0, CSR_EUEN_FPEN
  beqz    $t0, PushRegDone

  fst.d  $fa0, $sp, 0 * RSIZE
  fst.d  $fa1, $sp, 1 * RSIZE
  fst.d  $fa2, $sp, 2 * RSIZE
  fst.d  $fa3, $sp, 3 * RSIZE
  fst.d  $fa4, $sp, 4 * RSIZE
  fst.d  $fa5, $sp, 5 * RSIZE
  fst.d  $fa6, $sp, 6 * RSIZE
  fst.d  $fa7, $sp, 7 * RSIZE
  fst.d  $ft0, $sp, 8 * RSIZE
  fst.d  $ft1, $sp, 9 * RSIZE
  fst.d  $ft2, $sp, 10 * RSIZE
  fst.d  $ft3, $sp, 11 * RSIZE
  fst.d  $ft4, $sp, 12 * RSIZE
  fst.d  $ft5, $sp, 13 * RSIZE
  fst.d  $ft6, $sp, 14 * RSIZE
  fst.d  $ft7, $sp, 15 * RSIZE
  fst.d  $ft8, $sp, 16 * RSIZE
  fst.d  $ft9, $sp, 17 * RSIZE
  fst.d  $ft10, $sp, 18 * RSIZE
  fst.d  $ft11, $sp, 19 * RSIZE
  fst.d  $ft12, $sp, 20 * RSIZE
  fst.d  $ft13, $sp, 21 * RSIZE
  fst.d  $ft14, $sp, 22 * RSIZE
  fst.d  $ft15, $sp, 23 * RSIZE
  fst.d  $fs0, $sp, 24 * RSIZE
  fst.d  $fs1, $sp, 25 * RSIZE
  fst.d  $fs2, $sp, 26 * RSIZE
  fst.d  $fs3, $sp, 27 * RSIZE
  fst.d  $fs4, $sp, 28 * RSIZE
  fst.d  $fs5, $sp, 29 * RSIZE
  fst.d  $fs6, $sp, 30 * RSIZE
  fst.d  $fs7, $sp, 31 * RSIZE

  movfcsr2gr  $t3, $r0
  st.d        $t3, $sp, 32 * RSIZE  // Push the FCSR0 register.

  //
  // Push the fcc0-fcc7 registers.
  //
  movcf2gr    $t3, $fcc0
  or          $t2, $t3, $zero
  movcf2gr    $t3, $fcc1
  bstrins.d   $t2, $t3, 0xf, 0x8
  movcf2gr    $t3, $fcc2
  bstrins.d   $t2, $t3, 0x17, 0x10
  movcf2gr    $t3, $fcc3
  bstrins.d   $t2, $t3, 0x1f, 0x18
  movcf2gr    $t3, $fcc4
  bstrins.d   $t2, $t3, 0x27, 0x20
  movcf2gr    $t3, $fcc5
  bstrins.d   $t2, $t3, 0x2f, 0x28
  movcf2gr    $t3, $fcc6
  bstrins.d   $t2, $t3, 0x37, 0x30
  movcf2gr    $t3, $fcc7
  bstrins.d   $t2, $t3, 0x3f, 0x38
  st.d        $t2, $sp, 33 * RSIZE
  //
  // Push exception context down
  //

PushRegDone:
  //
  // Process IPI only when mailbox3 is NULL and mailbox0 is no-NULL.
  //
  li.d      $t0, LOONGARCH_IOCSR_MBUF0
  iocsrrd.d $a0, $t0
  beqz      $a0, EntryConmmonHanlder

  li.d      $t0, LOONGARCH_IOCSR_MBUF3
  iocsrrd.d $t1, $t0
  bnez      $t1, EntryConmmonHanlder

  csrrd     $t0, LOONGARCH_CSR_ESTAT
  srli.d    $t0, $t0, 12
  andi      $t0, $t0, 0x1
  beqz      $t0, EntryConmmonHanlder

  //
  // Clean up current processor mailbox 0 and mailbox 3.
  //
  li.d      $t0, LOONGARCH_IOCSR_MBUF0
  iocsrwr.d $zero, $t0
  li.d      $t0, LOONGARCH_IOCSR_MBUF3
  iocsrwr.d $zero, $t0

  //
  // Clear IPI interrupt.
  //
  li.d      $t0, LOONGARCH_IOCSR_IPI_STATUS
  iocsrrd.w $t1, $t0
  li.d      $t0, LOONGARCH_IOCSR_IPI_CLEAR
  iocsrwr.w $t1, $t0

  //
  // Only kernel stage BSP calls IPI without parameters. Clean up the PIE and make sure
  // global interrupts are turned off for the current processor when jumping to the kernel.
  //
  csrwr     $a0, LOONGARCH_CSR_ERA         // Update ERA
  li.w      $t0, BIT2                      // IE
  csrxchg   $zero, $t0, LOONGARCH_CSR_PRMD // Clean PIE

  //
  // Return this exception and jump to kernel using ERA.
  //
  ertn

EntryConmmonHanlder:
  addi.d  $sp, $sp, -(GP_REG_CONTEXT_SIZE + CSR_REG_CONTEXT_SIZE)
  move    $a0, $sp
  la.abs  $ra, ExceptionEntry
  jirl    $zero, $ra, 0
ASM_PFX(ExceptionEntryEnd):
.end
