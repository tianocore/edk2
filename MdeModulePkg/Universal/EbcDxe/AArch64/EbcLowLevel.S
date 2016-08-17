///** @file
//
//  This code provides low level routines that support the Virtual Machine
//  for option ROMs.
//
//  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>
//  Copyright (c) 2015, The Linux Foundation. All rights reserved.<BR>
//  Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
      stp  x19, x20, [sp, #-16]!
      stp  x29, x30, [sp, #-16]!

      mov  x19, x0
      mov  x20, sp
      sub  x2, x2, x1   // Length = NewStackPointer-FramePtr
      sub  sp, sp, x2
      sub  sp, sp, #64  // Make sure there is room for at least 8 args in the new stack
      mov  x0, sp

      bl   CopyMem      // Sp, NewStackPointer, Length

      ldp  x0, x1, [sp], #16
      ldp  x2, x3, [sp], #16
      ldp  x4, x5, [sp], #16
      ldp  x6, x7, [sp], #16

      blr  x19

      mov  sp,  x20
      ldp  x29, x30, [sp], #16
      ldp  x19, x20, [sp], #16

      ret

//****************************************************************************
// EbcLLEbcInterpret
//
// This function is called by the thunk code to handle an Native to EBC call
// This can handle up to 16 arguments (1-8 on in x0-x7, 9-16 are on the stack)
// x16 contains the Entry point that will be the first argument when
// EBCInterpret is called.
//
//****************************************************************************
ASM_PFX(EbcLLEbcInterpret):
    stp  x29, x30, [sp, #-16]!

    // copy the current arguments 9-16 from old location and add arg 7 to stack
    // keeping 16 byte stack alignment
    sub sp, sp, #80
    str x7, [sp]
    ldr x11, [sp, #96]
    str x11, [sp, #8]
    ldr x11, [sp, #104]
    str x11, [sp, #16]
    ldr x11, [sp, #112]
    str x11, [sp, #24]
    ldr x11, [sp, #120]
    str x11, [sp, #32]
    ldr x11, [sp, #128]
    str x11, [sp, #40]
    ldr x11, [sp, #136]
    str x11, [sp, #48]
    ldr x11, [sp, #144]
    str x11, [sp, #56]
    ldr x11, [sp, #152]
    str x11, [sp, #64]

    // Shift arguments and add entry point and as argument 1
    mov x7, x6
    mov x6, x5
    mov x5, x4
    mov x4, x3
    mov x3, x2
    mov x2, x1
    mov x1, x0
    mov x0, x16

    // call C-code
    bl ASM_PFX(EbcInterpret)
    add sp, sp, #80

    ldp  x29, x30, [sp], #16

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
    stp  x29, x30, [sp, #-16]!
    // build new parameter calling convention
    mov  x2, x1
    mov  x1, x0
    mov  x0, x16

    // call C-code
    bl ASM_PFX(ExecuteEbcImageEntryPoint)
    ldp  x29, x30, [sp], #16
    ret

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
