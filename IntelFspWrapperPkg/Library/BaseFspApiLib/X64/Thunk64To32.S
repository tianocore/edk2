#
# Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
# Module Name:
#
#    Thunk64To32.asm
#
# Abstract:
#
#   This is the assembly code to transition from long mode to compatibility mode to execute 32-bit code and then
#   transit back to long mode.
#
#-------------------------------------------------------------------------------

#----------------------------------------------------------------------------
# Procedure:    AsmExecute32BitCode
#
# Input:        None
#
# Output:       None
#
# Prototype:    UINT32
#               AsmExecute32BitCode (
#                 IN UINT64           Function,
#                 IN UINT64           Param1,
#                 IN UINT64           Param2,
#                 IN IA32_DESCRIPTOR  *InternalGdtr
#                 );
#
#
# Description:  A thunk function to execute 32-bit code in long mode.
#
#----------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(AsmExecute32BitCode)
ASM_PFX(AsmExecute32BitCode):
    #
    # save IFLAG and disable it
    #
    pushfq
    cli

    #
    # save orignal GDTR and CS
    #
    movl    %ds, %eax
    push    %rax
    movl    %cs, %eax
    push    %rax
    subq    $0x10, %rsp
    sgdt    (%rsp)
    #
    # load internal GDT
    #
    lgdt    (%r9)
    #
    # Save general purpose register and rflag register
    #
    pushfq
    push    %rdi
    push    %rsi
    push    %rbp
    push    %rbx

    #
    # save CR3
    #
    movq    %cr3, %rax
    movq    %rax, %rbp

    #
    # Prepare the CS and return address for the transition from 32-bit to 64-bit mode
    #
    movq    $0x10, %rax              # load long mode selector
    shl     $32, %rax
    lea     ReloadCS(%rip), %r9   #Assume the ReloadCS is under 4G
    orq     %r9, %rax
    push    %rax
    #
    # Save parameters for 32-bit function call
    #
    movq    %r8, %rax
    shl     $32, %rax
    orq     %rdx, %rax
    push    %rax
    #
    # save the 32-bit function entry and the return address into stack which will be
    # retrieve in compatibility mode.
    #
    lea     ReturnBack(%rip), %rax   #Assume the ReloadCS is under 4G
    shl     $32, %rax
    orq     %rcx, %rax
    push    %rax

    #
    # let rax save DS
    #
    movq    $0x18, %rax

    #
    # Change to Compatible Segment
    #
    movq    $8, %rcx               # load compatible mode selector
    shl     $32, %rcx
    lea     Compatible(%rip), %rdx # assume address < 4G
    orq     %rdx, %rcx
    push    %rcx
    .byte   0xcb                # retf

Compatible:
    # reload DS/ES/SS to make sure they are correct referred to current GDT
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss

    #
    # Disable paging
    #
    movq    %cr0, %rcx
    btc     $31, %ecx
    movq    %rcx, %cr0
    #
    # Clear EFER.LME
    #
    movl    $0xC0000080, %ecx
    rdmsr
    btc     $8, %eax
    wrmsr

# Now we are in protected mode
    #
    # Call 32-bit function. Assume the function entry address and parameter value is less than 4G
    #
    pop    %rax                 # Here is the function entry
    #
    # Now the parameter is at the bottom of the stack,  then call in to IA32 function.
    #
    jmp   *%rax
ReturnBack:
    movl  %eax, %ebx            # save return status
    pop   %rcx                  # drop param1
    pop   %rcx                  # drop param2

    #
    # restore CR4
    #
    movq    %cr4, %rax
    bts     $5, %eax
    movq    %rax, %cr4

    #
    # restore CR3
    #
    movl    %ebp, %eax
    movq    %rax, %cr3

    #
    # Set EFER.LME to re-enable ia32-e
    #
    movl    $0xC0000080, %ecx
    rdmsr
    bts     $8, %eax
    wrmsr
    #
    # Enable paging
    #
    movq    %cr0, %rax
    bts     $31, %eax
    mov     %rax, %cr0
# Now we are in compatible mode

    #
    # Reload cs register
    #
    .byte   0xcb                # retf
ReloadCS:
    #
    # Now we're in Long Mode
    #
    #
    # Restore C register and eax hold the return status from 32-bit function.
    # Note: Do not touch rax from now which hold the return value from IA32 function
    #
    movl    %ebx, %eax # put return status to EAX
    pop     %rbx
    pop     %rbp
    pop     %rsi
    pop     %rdi
    popfq
    #
    # Switch to orignal GDT and CS. here rsp is pointer to the orignal GDT descriptor.
    #
    lgdt    (%rsp)
    #
    # drop GDT descriptor in stack
    #
    addq    $0x10, %rsp
    #
    # switch to orignal CS and GDTR
    #
    pop     %r9                 # get  CS
    shl     $32, %r9            # rcx[32..47] <- Cs
    lea     ReturnToLongMode(%rip), %rcx
    orq     %r9, %rcx
    push    %rcx
    .byte   0xcb                # retf
ReturnToLongMode:
    #
    # Reload original DS/ES/SS
    #
    pop     %rcx
    movl    %ecx, %ds
    movl    %ecx, %es
    movl    %ecx, %ss

    #
    # Restore IFLAG
    #
    popfq

    ret

