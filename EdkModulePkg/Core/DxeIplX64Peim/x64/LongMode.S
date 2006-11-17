#------------------------------------------------------------------------------
#*
#*   Copyright (c) 2006, Intel Corporation                                                         
#*   All rights reserved. This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*    LongMode.S
#*  
#*   Abstract:
#*
#*    Transition from 32-bit protected mode EFI environment into x64 
#*    64-bit bit long mode.
#*  
#*   This file is not fully ported or operational.
#*  
#------------------------------------------------------------------------------

.686p: 
#.MODEL flat

#
# Create the exception handler code in IA32 C code
#

.code: 
.stack: 
.MMX: 
.XMM: 

.global _LoadGo64Gdt;
_LoadGo64Gdt:
    pushl   %ebp              # C prolog
    pushl   %edi
    movl    %esp, %ebp
    #
    # Disable interrupts
    #
    cli
    #
    # Reload the selectors
    # Note:
    #      Make the Selectors 64-bit ready
    #
    movl    gdtr, %edi          # Load GDT register
    movw    %cs, %ax            # Get the selector data from our code image          
    mov     %ax, %es
# FIXME MISMATCH: "    lgdt    FWORD PTR es:[edi]  "

    .byte   0x67
    .byte   0xea              # Far Jump Offset:Selector to reload CS
# FIXME MISMATCH: "    dd      OFFSET DataSelectorRld"
# FIXME MISMATCH: "    dw      LINEAR_CODE_SEL   "
DataSelectorRld:
    movw    SYS_DATA_SEL, %ax # Update the Base for the new selectors, too
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss

    popl    %edi
    popl    %ebp
    ret
#_LoadGo64Gdt ENDP


# VOID
# ActivateLongMode (
#   IN  EFI_PHYSICAL_ADDRESS  PageTables,  
#   IN  EFI_PHYSICAL_ADDRESS  HobStart,
#   IN  EFI_PHYSICAL_ADDRESS  Stack,
#   IN  EFI_PHYSICAL_ADDRESS  PpisNeededByDxeIplEntryPoint,
#   IN  EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint
#   )
#
# Input:  [ebp][0h]  = Original ebp
#         [ebp][4h]  = Return address
#         [ebp][8h]  = PageTables
#         [ebp][10h] = HobStart
#         [ebp][18h] = Stack
#         [ebp][20h] = CodeEntryPoint1 <--- Call this first (for each call, pass HOB pointer)
#         [ebp][28h] = CodeEntryPoint2 <--- Call this second
#
#
.global _ActivateLongMode;
_ActivateLongMode:
    pushl   %ebp              # C prolog
    movl    %esp, %ebp

    #
    # Use CPUID to determine if the processor supports long mode.
    #
    movl    $0x80000000, %eax # Extended-function code 8000000h.
    cpuid                   # Is largest extended function
    cmpl    $0x80000000, %eax # any function > 80000000h?
    jbe     no_long_mode    # If not, no long mode.
    movl    $0x80000001, %eax # Extended-function code 8000001h.
    cpuid                   # Now EDX = extended-features flags.
    btl     $29, %edx       # Test if long mode is supported.
    jnc     no_long_mode    # Exit if not supported.

    #
    # Enable the 64-bit page-translation-table entries by
    # setting CR4.PAE=1 (this is _required_ before activating
    # long mode). Paging is not enabled until after long mode
    # is enabled.
    #
    movl %cr4, %eax
    btsl $5, %eax
    movl %eax, %cr4

    #
    # Get the long-mode page tables, and initialize the
    # 64-bit CR3 (page-table base address) to point to the base
    # of the PML4 page table. The PML4 page table must be located
    # below 4 Gbytes because only 32 bits of CR3 are loaded when
    # the processor is not in 64-bit mode.
    #
    movl 0x8(%ebp), %eax    # Get Page Tables
    movl %eax, %cr3          # Initialize CR3 with PML4 base.

    #
    # Enable long mode (set EFER.LME=1).
    #
    movl $0xc0000080, %ecx # EFER MSR number.
    rdmsr               # Read EFER.
    btsl $8, %eax       # Set LME=1.
    wrmsr               # Write EFER.

    #
    # Enable paging to activate long mode (set CR0.PG=1)
    #


    movl %cr0, %eax # Read CR0.
    btsl $31, %eax # Set PG=1.
    movl %eax, %cr0 # Write CR0.
    jmp   go_to_long_mode
go_to_long_mode: 

    #
    # This is the next instruction after enabling paging.  Jump to long mode
    #
    .byte   0x67
    .byte   0xea              # Far Jump Offset:Selector to reload CS
#FIXME MISMATCH: "    dd      OFFSET in_long_mode"
#FIXME MISMATCH: "    dw      SYS_CODE64_SEL    "
in_long_mode:
    movw    SYS_DATA64_SEL, %ax
    movw    %ax, %es
    movw    %ax, %ss
    movw    %ax, %ds
    jmp     .


    #
    # We're in long mode, so marshall the arguments to call the
    # passed in function pointers
    # Recall
    #         [ebp][10h] = HobStart
    #         [ebp][18h] = Stack
    #         [ebp][20h] = PpisNeededByDxeIplEntryPoint <--- Call this first (for each call, pass HOB pointer)
    #         [ebp][28h] = DxeCoreEntryPoint            <--- Call this second
    #
    .byte 0x48
    movl 0x18(%ebp), %ebx     # Setup the stack
    .byte 0x48
    movl %ebx, %esp           # On a new stack now


## 00000905  FF D0                  call rax

    .byte 0x48
    movl 0x10(%ebp), %ecx     # Pass Hob Start in RCX
    .byte 0x48
    movl 0x28(%ebp), %eax     # Get the function pointer for 
                              # DxeCoreEntryPoint into EAX

## 00000905  FF D0                  call rax
    .byte 0xff
    .byte 0xd0

    #
    # WE SHOULD NEVER GET HERE!!!!!!!!!!!!!
    #
no_long_mode: 
    jmp   no_long_mode
#_ActivateLongMode ENDP

        .align 16

gdtr: #FIXME MISMATCH: "gdtr    dw _GDT_END - _GDT_BASE - 1   "
#FIXME MISMATCH: "        dd OFFSET _GDT_BASE          "

#-----------------------------------------------------------------------------;
#   global descriptor table (GDT)
#-----------------------------------------------------------------------------;

        .align 16

.global _GDT_BASE
_GDT_BASE: 
# null descriptor
.equ                NULL_SEL, .-_GDT_BASE     # Selector [0]
        .word 0         # limit 15:0
        .word 0         # base 15:0
        .byte 0         # base 23:16
        .byte 0         # type
        .byte 0         # limit 19:16, flags
        .byte 0         # base 31:24

# linear data segment descriptor
.equ            LINEAR_SEL, .-_GDT_BASE         # Selector [0x8]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# linear code segment descriptor
.equ            LINEAR_CODE_SEL, .-_GDT_BASE         # Selector [0x10]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9F      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# system data segment descriptor
.equ            SYS_DATA_SEL, .-_GDT_BASE         # Selector [0x18]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x93      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# system code segment descriptor
.equ            SYS_CODE_SEL, .-_GDT_BASE         # Selector [0x20]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # present, ring 0, data, expand-up, writable
        .byte 0xCF              # page-granular, 32-bit
        .byte 0

# spare segment descriptor
.equ        SPARE3_SEL, .-_GDT_BASE             # Selector [0x28]
        .word 0         # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0         # present, ring 0, data, expand-up, writable
        .byte 0         # page-granular, 32-bit
        .byte 0

#
# system data segment descriptor
#
.equ              SYS_DATA64_SEL, .-_GDT_BASE           # Selector [0x30]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x92      # P | DPL [1..2] | 1   | 1   | C | R | A
        .byte 0xCF      # G | D   | L    | AVL | Segment [19..16]
        .byte 0

#
# system code segment descriptor
#
.equ              SYS_CODE64_SEL, .-_GDT_BASE           # Selector [0x38]
        .word 0xFFFF    # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0x9A      # P | DPL [1..2] | 1   | 1   | C | R | A
        .byte 0xAF      # G | D   | L    | AVL | Segment [19..16]
        .byte 0

# spare segment descriptor
.equ        SPARE4_SEL, .-_GDT_BASE             # Selector [0x40]
        .word 0         # limit 0xFFFFF
        .word 0         # base 0
        .byte 0
        .byte 0         # present, ring 0, data, expand-up, writable
        .byte 0         # page-granular, 32-bit
        .byte 0

_GDT_END: 



