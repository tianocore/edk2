//
//  Copyright (c) 2014, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials are licensed and made available
//  under the terms and conditions of the BSD License which accompanies this
//  distribution. The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

// For the moment we assume this will run in SVC mode on ARMv7

    EXPORT  ArmGicV3GetControlSystemRegisterEnable
    EXPORT  ArmGicV3SetControlSystemRegisterEnable
    EXPORT  ArmGicV3EnableInterruptInterface
    EXPORT  ArmGicV3DisableInterruptInterface
    EXPORT  ArmGicV3EndOfInterrupt
    EXPORT  ArmGicV3AcknowledgeInterrupt
    EXPORT  ArmGicV3SetPriorityMask
    EXPORT  ArmGicV3SetBinaryPointer

    AREA ArmGicV3, CODE, READONLY

//UINT32
//EFIAPI
//ArmGicGetControlSystemRegisterEnable (
//  VOID
//  );
ArmGicV3GetControlSystemRegisterEnable
        mrc     p15, 0, r0, c12, c12, 5 // ICC_SRE
        bx      lr

//VOID
//EFIAPI
//ArmGicSetControlSystemRegisterEnable (
//  IN UINT32         ControlSystemRegisterEnable
//  );
ArmGicV3SetControlSystemRegisterEnable
        mcr     p15, 0, r0, c12, c12, 5 // ICC_SRE
        isb
        bx      lr

//VOID
//ArmGicV3EnableInterruptInterface (
//  VOID
//  );
ArmGicV3EnableInterruptInterface
        mov     r0, #1
        mcr     p15, 0, r0, c12, c12, 7 // ICC_IGRPEN1
        bx      lr

//VOID
//ArmGicV3DisableInterruptInterface (
//  VOID
//  );
ArmGicV3DisableInterruptInterface
        mov     r0, #0
        mcr     p15, 0, r0, c12, c12, 7 // ICC_IGRPEN1
        bx      lr

//VOID
//ArmGicV3EndOfInterrupt (
//  IN UINTN InterruptId
//  );
ArmGicV3EndOfInterrupt
        mcr     p15, 0, r0, c12, c12, 1 //ICC_EOIR1
        bx      lr

//UINTN
//ArmGicV3AcknowledgeInterrupt (
//  VOID
//  );
ArmGicV3AcknowledgeInterrupt
        mrc     p15, 0, r0, c12, c8, 0 //ICC_IAR1
        bx      lr

//VOID
//ArmGicV3SetPriorityMask (
//  IN UINTN                  Priority
//  );
ArmGicV3SetPriorityMask
        mcr     p15, 0, r0, c4, c6, 0 //ICC_PMR
        bx      lr

//VOID
//ArmGicV3SetBinaryPointer (
//  IN UINTN                  BinaryPoint
//  );
ArmGicV3SetBinaryPointer
        mcr     p15, 0, r0, c12, c12, 3 //ICC_BPR1
        bx      lr

    END
