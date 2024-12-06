#
#  Copyright (c) 2024, NewFW Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

#include <AsmMacroLib.h>

//UINTN
//ArmGicV3GetControlRegister(
//  VOID
//  );
ASM_FUNC(ArmGicV3GetControlRegister)
        mrs     x0, icc_ctlr_el1
        ret

//VOID
//ArmGicV3SetControlRegister(
//  IN UINTN          Value
//  );
ASM_FUNC(ArmGicV3SetControlRegister)
        msr     icc_ctlr_el1, x0
        ret

