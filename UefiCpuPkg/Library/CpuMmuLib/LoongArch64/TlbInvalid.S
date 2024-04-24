#------------------------------------------------------------------------------
#
# Invalid TLB operation function
#
# Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#-----------------------------------------------------------------------------

#include <Register/LoongArch64/Csr.h>

ASM_GLOBAL ASM_PFX(InvalidTlb)

#
# Invalid corresponding TLB entries are based on the address given
# @param a0 The address corresponding to the invalid page table entry
# @retval  none
#
ASM_PFX(InvalidTlb):
    invtlb  INVTLB_ADDR_GTRUE_OR_ASID, $zero, $a0
    jirl    $zero, $ra, 0

    .end
