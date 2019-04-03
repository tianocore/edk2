///** @file
//
//  This code provides low level routines that support the Virtual Machine
//  for option ROMs.
//
//  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>
//  Copyright (c) 2015, The Linux Foundation. All rights reserved.<BR>
//  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//**/

ASM_GLOBAL ASM_PFX(EbcLLCALLEXNative)
ASM_GLOBAL ASM_PFX(EbcLLEbcInterpret)
ASM_GLOBAL ASM_PFX(EbcLLExecuteEbcImageEntryPoint)

ASM_GLOBAL ASM_PFX(mEbcInstructionBufferTemplate)

//****************************************************************************
// EbcLLCALLEX
//
// This function is called to execute an EBC CALLEX instruction.
// This instruction requires that we thunk out to external native
// code. For AArch64, we copy the VM stack into the main stack and then pop
// the first 8 arguments off according to the AArch64 Procedure Call Standard
// On return, we restore the stack pointer to its original location.
//
//****************************************************************************
// UINTN EbcLLCALLEXNative(UINTN FuncAddr, UINTN NewStackPointer, VOID *FramePtr)
ASM_PFX(EbcLLCALLEXNative):
    mov     x8, x0                 // Preserve x0
    mov     x9, x1                 // Preserve x1

    //
    // If the EBC stack frame is smaller than or equal to 64 bytes, we know there
    // are no stacked arguments #9 and beyond that we need to copy to the native
    // stack. In this case, we can perform a tail call which is much more
    // efficient, since there is no need to touch the native stack at all.
    //
    sub     x3, x2, x1              // Length = NewStackPointer - FramePtr
    cmp     x3, #64
    b.gt    1f

    //
    // While probably harmless in practice, we should not access the VM stack
    // outside of the interval [NewStackPointer, FramePtr), which means we
    // should not blindly fill all 8 argument registers with VM stack data.
    // So instead, calculate how many argument registers we can fill based on
    // the size of the VM stack frame, and skip the remaining ones.
    //
    adr     x0, 0f                  // Take address of 'br' instruction below
    bic     x3, x3, #7              // Ensure correct alignment
    sub     x0, x0, x3, lsr #1      // Subtract 4 bytes for each arg to unstack
    br      x0                      // Skip remaining argument registers

    ldr     x7, [x9, #56]           // Call with 8 arguments
    ldr     x6, [x9, #48]           //  |
    ldr     x5, [x9, #40]           //  |
    ldr     x4, [x9, #32]           //  |
    ldr     x3, [x9, #24]           //  |
    ldr     x2, [x9, #16]           //  |
    ldr     x1, [x9, #8]            //  V
    ldr     x0, [x9]                // Call with 1 argument

0:  br      x8                      // Call with no arguments

    //
    // More than 64 bytes: we need to build the full native stack frame and copy
    // the part of the VM stack exceeding 64 bytes (which may contain stacked
    // arguments) to the native stack
    //
1:  stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    //
    // Ensure that the stack pointer remains 16 byte aligned,
    // even if the size of the VM stack frame is not a multiple of 16
    //
    add     x1, x1, #64             // Skip over [potential] reg params
    tbz     x3, #3, 2f              // Multiple of 16?
    ldr     x4, [x2, #-8]!          // No? Then push one word
    str     x4, [sp, #-16]!         // ... but use two slots
    b       3f

2:  ldp     x4, x5, [x2, #-16]!
    stp     x4, x5, [sp, #-16]!
3:  cmp     x2, x1
    b.gt    2b

    ldp     x0, x1, [x9]
    ldp     x2, x3, [x9, #16]
    ldp     x4, x5, [x9, #32]
    ldp     x6, x7, [x9, #48]

    blr     x8

    mov     sp, x29
    ldp     x29, x30, [sp], #16
    ret

//****************************************************************************
// EbcLLEbcInterpret
//
// This function is called by the thunk code to handle an Native to EBC call
// This can handle up to 16 arguments (1-8 on in x0-x7, 9-16 are on the stack)
// x16 contains the Entry point that will be the first stacked argument when
// EBCInterpret is called.
//
//****************************************************************************
ASM_PFX(EbcLLEbcInterpret):
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    // push the entry point and the address of args #9 - #16 onto the stack
    add     x17, sp, #16
    stp     x16, x17, [sp, #-16]!

    // call C-code
    bl      ASM_PFX(EbcInterpret)

    add     sp, sp, #16
    ldp     x29, x30, [sp], #16
    ret

//****************************************************************************
// EbcLLExecuteEbcImageEntryPoint
//
// This function is called by the thunk code to handle the image entry point
// x16 contains the Entry point that will be the third argument when
// ExecuteEbcImageEntryPoint is called.
//
//****************************************************************************
ASM_PFX(EbcLLExecuteEbcImageEntryPoint):
    mov     x2, x16

    // tail call to C code
    b       ASM_PFX(ExecuteEbcImageEntryPoint)

//****************************************************************************
// mEbcInstructionBufferTemplate
//****************************************************************************
    .section    ".rodata", "a"
    .align      3
ASM_PFX(mEbcInstructionBufferTemplate):
    adr     x17, 0f
    ldp     x16, x17, [x17]
    br      x17

    //
    // Add a magic code here to help the VM recognize the thunk.
    //
    hlt     #0xEBC

0:  .quad   0   // EBC_ENTRYPOINT_SIGNATURE
    .quad   0   // EBC_LL_EBC_ENTRYPOINT_SIGNATURE
