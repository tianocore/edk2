//
//  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

    EXPORT  return_from_exception
    EXPORT  enter_monitor_mode
    EXPORT  copy_cpsr_into_spsr
    EXPORT  set_non_secure_mode

    AREA   Helper, CODE, READONLY

// r0: Monitor World EntryPoint
// r1: MpId
// r2: SecBootMode
// r3: Secure Monitor mode stack
enter_monitor_mode FUNCTION
    cmp     r3, #0                      // If a Secure Monitor stack base has not been defined then use the Secure stack
    moveq   r3, sp

    mrs     r4, cpsr                    // Save current mode (SVC) in r4
    bic     r5, r4, #0x1f               // Clear all mode bits
    orr     r5, r5, #0x16               // Set bits for Monitor mode
    msr     cpsr_cxsf, r5               // We are now in Monitor Mode

    mov     sp, r3                      // Set the stack of the Monitor Mode

    mov     lr, r0                      // Use the pass entrypoint as lr

    msr     spsr_cxsf, r4               // Use saved mode for the MOVS jump to the kernel

    mov     r4, r0                      // Swap EntryPoint and MpId registers
    mov     r0, r1
    mov     r1, r2
    mov     r2, r3

    bx      r4
    ENDFUNC

// We cannot use the instruction 'movs pc, lr' because the caller can be written either in ARM or Thumb2 assembler.
// When we will jump into this function, we will set the CPSR flag to ARM assembler. By copying directly 'lr' into
// 'pc'; we will not change the CPSR flag and it will crash.
// The way to fix this limitation is to do the movs into the ARM assmbler code and then do a 'bx'.
return_from_exception
    adr     lr, returned_exception
    movs    pc, lr
returned_exception                           // We are now in non-secure state
    bx      r0

// Save the current Program Status Register (PSR) into the Saved PSR
copy_cpsr_into_spsr
    mrs     r0, cpsr
    msr     spsr_cxsf, r0
    bx      lr

// Set the Non Secure Mode
set_non_secure_mode
    push    { r1 }
    and     r0, r0, #0x1f     // Keep only the mode bits
    mrs     r1, spsr          // Read the spsr
    bic     r1, r1, #0x1f     // Clear all mode bits
    orr     r1, r1, r0
    msr     spsr_cxsf, r1     // write back spsr (may have caused a mode switch)
    isb
    pop     { r1 }
    bx      lr                // return (hopefully thumb-safe!)

dead
    B       dead

    END
