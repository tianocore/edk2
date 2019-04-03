//
//  Copyright (c) 2014, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

// For the moment we assume this will run in SVC mode on ARMv7


    INCLUDE AsmMacroExport.inc

//UINT32
//EFIAPI
//ArmGicGetControlSystemRegisterEnable (
//  VOID
//  );
 RVCT_ASM_EXPORT ArmGicV3GetControlSystemRegisterEnable
        mrc     p15, 0, r0, c12, c12, 5 // ICC_SRE
        bx      lr

//VOID
//EFIAPI
//ArmGicSetControlSystemRegisterEnable (
//  IN UINT32         ControlSystemRegisterEnable
//  );
 RVCT_ASM_EXPORT ArmGicV3SetControlSystemRegisterEnable
        mcr     p15, 0, r0, c12, c12, 5 // ICC_SRE
        isb
        bx      lr

//VOID
//ArmGicV3EnableInterruptInterface (
//  VOID
//  );
 RVCT_ASM_EXPORT ArmGicV3EnableInterruptInterface
        mov     r0, #1
        mcr     p15, 0, r0, c12, c12, 7 // ICC_IGRPEN1
        bx      lr

//VOID
//ArmGicV3DisableInterruptInterface (
//  VOID
//  );
 RVCT_ASM_EXPORT ArmGicV3DisableInterruptInterface
        mov     r0, #0
        mcr     p15, 0, r0, c12, c12, 7 // ICC_IGRPEN1
        bx      lr

//VOID
//ArmGicV3EndOfInterrupt (
//  IN UINTN InterruptId
//  );
 RVCT_ASM_EXPORT ArmGicV3EndOfInterrupt
        mcr     p15, 0, r0, c12, c12, 1 //ICC_EOIR1
        bx      lr

//UINTN
//ArmGicV3AcknowledgeInterrupt (
//  VOID
//  );
 RVCT_ASM_EXPORT ArmGicV3AcknowledgeInterrupt
        mrc     p15, 0, r0, c12, c12, 0 //ICC_IAR1
        bx      lr

//VOID
//ArmGicV3SetPriorityMask (
//  IN UINTN                  Priority
//  );
 RVCT_ASM_EXPORT ArmGicV3SetPriorityMask
        mcr     p15, 0, r0, c4, c6, 0 //ICC_PMR
        bx      lr

//VOID
//ArmGicV3SetBinaryPointer (
//  IN UINTN                  BinaryPoint
//  );
 RVCT_ASM_EXPORT ArmGicV3SetBinaryPointer
        mcr     p15, 0, r0, c12, c12, 3 //ICC_BPR1
        bx      lr

    END
