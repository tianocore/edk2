##------------------------------------------------------------------------------
#
# SpeculationBarrier() for AArch64
#
# Copyright (c) 2019, Linaro Ltd. All rights reserved.
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##------------------------------------------------------------------------------

.text
.p2align 2

GCC_ASM_EXPORT(SpeculationBarrier)


#/**
#  Uses as a barrier to stop speculative execution.
#
#  Ensures that no later instruction will execute speculatively, until all prior
#  instructions have completed.
#
#**/
#VOID
#EFIAPI
#SpeculationBarrier (
#  VOID
#  );
#
ASM_PFX(SpeculationBarrier):
    dsb
    isb
    bx   lr
