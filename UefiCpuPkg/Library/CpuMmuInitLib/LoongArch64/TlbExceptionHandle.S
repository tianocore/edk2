#------------------------------------------------------------------------------
#
# TLB refill exception handler
#
# Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#-----------------------------------------------------------------------------

#include <Register/LoongArch64/Csr.h>

ASM_GLOBAL ASM_PFX(HandleTlbRefillStart)
ASM_GLOBAL ASM_PFX(HandleTlbRefillEnd)

#
#  Refill the page table.
#  @param  VOID
#  @retval  VOID
#
ASM_PFX(HandleTlbRefillStart):
  csrwr   $t0, LOONGARCH_CSR_TLBRSAVE
  csrrd   $t0, LOONGARCH_CSR_PWCTL1
  srli.d  $t0, $t0, 18
  andi    $t0, $t0, 0x3F
  bnez    $t0, Level5
  csrrd   $t0, LOONGARCH_CSR_PWCTL1
  srli.d  $t0, $t0, 6
  andi    $t0, $t0, 0x3F
  bnez    $t0, Level4
  csrrd   $t0, LOONGARCH_CSR_PGD
  b       Level3
Level5:
  csrrd   $t0, LOONGARCH_CSR_PGD
  lddir   $t0, $t0, 4   #Put pud BaseAddress into T0
  lddir   $t0, $t0, 3   #Put pud BaseAddress into T0
  b       Level3
Level4:
  csrrd   $t0, LOONGARCH_CSR_PGD
  lddir   $t0, $t0, 3   #Put pud BaseAddress into T0
Level3:
  lddir   $t0, $t0, 2   #Put pmd BaseAddress into T0
  lddir   $t0, $t0, 1   #Put pte BaseAddress into T0
  ldpte   $t0, 0
  ldpte   $t0, 1
  tlbfill   // refill hi, lo0, lo1
  csrrd   $t0, LOONGARCH_CSR_TLBRSAVE
  ertn
ASM_PFX(HandleTlbRefillEnd):

    .end
