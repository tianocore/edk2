#
#  Copyright (c) 2025, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

#include <AsmMacroLib.h>

#define  ICC_CR0_EL1            S3_1_C12_C0_1

#define  ICC_ICSR_EL1           S3_0_C12_C10_4

#define  ICC_PCR_EL1            S3_1_C12_C0_2

#define  ICC_PPI_ENABLER0_EL1   S3_0_C12_C10_6
#define  ICC_PPI_ENABLER1_EL1   S3_0_C12_C10_7

#define  ICC_PPI_HMR0_EL1       S3_0_C12_C10_0
#define  ICC_PPI_HMR1_EL1       S3_0_C12_C10_1

#define  GICR_CDIA              S1_0_C12_C3_0
#define  GIC_CDRCFG             S1_0_C12_C1_5
#define  GIC_CDDI               S1_0_C12_C2_0
#define  GIC_CDDIS              S1_0_C12_C1_0
#define  GIC_CDEN               S1_0_C12_C1_1
#define  GIC_CDEOI              #0, C12, C1, #7

#define IRS_CR0 0x100
#define IRS_IST_BASER 0x148

//UINT64
//EFIAPI
//ArmGicV5GetPpiEnabler0 (
//  VOID
//  );
ASM_FUNC(ArmGicV5GetPpiEnabler0)
        mrs     x0, ICC_PPI_ENABLER0_EL1
        ret

//UINT64
//EFIAPI
//ArmGicV5GetPpiEnabler1 (
//  VOID
//  );
ASM_FUNC(ArmGicV5GetPpiEnabler1)
        mrs     x0, ICC_PPI_ENABLER1_EL1
        ret

//VOID
//EFIAPI
//ArmGicV5SetPpiEnabler0 (
//  IN UINT64         InterruptMask
//  );
ASM_FUNC(ArmGicV5SetPpiEnabler0)
        msr     ICC_PPI_ENABLER0_EL1, x0
        ret

//VOID
//EFIAPI
//ArmGicV5SetPpiEnabler1 (
//  IN UINT64         InterruptMask
//  );
ASM_FUNC(ArmGicV5SetPpiEnabler1)
        msr     ICC_PPI_ENABLER1_EL1, x0
        ret

//UINT64
//EFIAPI
//ArmGicV5GetPPIHMR0 (
//  VOID
//  );
ASM_FUNC(ArmGicV5GetPPIHMR0)
        mrs     x0, ICC_PPI_HMR0_EL1
        ret

//UINT64
//EFIAPI
//ArmGicV5GetPPIHMR1 (
//  VOID
//  );
ASM_FUNC(ArmGicV5GetPPIHMR1)
        mrs     x0, ICC_PPI_HMR1_EL1
        ret

//VOID
//EFIAPI
//ArmGicV5SpiEnable (
//  IN UINT32         SpiId
//  );
ASM_FUNC(ArmGicV5SpiEnable)
        orr     x0, x0, #0x60000000
        msr     GIC_CDEN, x0
        ret

//VOID
//EFIAPI
//ArmGicV5SpiDisable (
//  IN UINT32         SpiId
//  );
ASM_FUNC(ArmGicV5SpiDisable)
        orr     x0, x0, #0x60000000
        msr     GIC_CDDIS, x0
        ret

//UINT64
//EFIAPI
//ArmGicV5ReadInterruptConfig (
//  IN UINT32         InterruptId
//  );
ASM_FUNC(ArmGicV5ReadInterruptConfig)
        msr     GIC_CDRCFG, x0
        isb
        mrs     x0, ICC_ICSR_EL1
        ret


//VOID
//EFIAPI
//ArmGicV5EnableInterruptInterface (
//  UINT64  IrsConfigFrameBase
//  );
ASM_FUNC(ArmGicV5EnableInterruptInterface)
        mov     x1, #1
        msr     ICC_CR0_EL1, x1
        mov     x1, #0
        msr     ICC_PCR_EL1, x1

        mov     x1, #0
        str     x1, [x0, #IRS_CR0]
        isb

        str     x1, [x0, #IRS_IST_BASER]
        isb

        mov     x1, #1
        str     x1, [x0, #IRS_CR0]
        isb

        ret

//VOID
//EFIAPI
//ArmGicV5DisableInterruptInterface (
//  UINT64  IrsConfigFrameBase
//  );
ASM_FUNC(ArmGicV5DisableInterruptInterface)
        mov     x1, #0
        str     x1, [x0, #IRS_CR0]
        isb

        mov     x1, #0
        msr     ICC_CR0_EL1, x1

        ret

//VOID
//EFIAPI
//ArmGicV5DeactivateInterrupt (
//  IN UINTN          InterruptId
//  );
ASM_FUNC(ArmGicV5DeactivateInterrupt)
        msr     GIC_CDDI, x0
        ret

//VOID
//EFIAPI
//ArmGicV5EndOfInterrupt (
//  VOID
//  );
ASM_FUNC(ArmGicV5EndOfInterrupt)
        sys     GIC_CDEOI
        ret

//UINTN
//EFIAPI
//ArmGicV5AcknowledgeInterrupt (
//  VOID
//  );
ASM_FUNC(ArmGicV5AcknowledgeInterrupt)
        mrs     x0, GICR_CDIA
        ret
