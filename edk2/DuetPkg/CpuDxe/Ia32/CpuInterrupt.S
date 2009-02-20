/** @file
 *
 *   Copyright 2006, Intel Corporation                                                         
 *   All rights reserved. This program and the accompanying materials                          
 *   are licensed and made available under the terms and conditions of the BSD License         
 *   which accompanies this distribution.  The full text of the license may be found at        
 *   http://opensource.org/licenses/bsd-license.php                                            
 *                                                                                             
 *   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
 *   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
 *   
 *    CpuInterrupt.S
 *  
 *   Abstract:
 *
 **/

.globl ASM_PFX(SystemTimerHandler)
.globl ASM_PFX(SystemExceptionHandler)
.globl ASM_PFX(mExceptionCodeSize)
.globl ASM_PFX(InitDescriptor)
.globl ASM_PFX(InstallInterruptHandler)

ASM_PFX(mExceptionCodeSize):  .long  9

/**
 * VOID
 * InitDescriptor (
 * VOID
 * )
 **/
ASM_PFX(InitDescriptor):
        lgdt    gdtr
        lidt    idtr
        ret

/**
 * VOID
 * InstallInterruptHandler (
 *     UINTN Vector,
 *     VOID  (*Handler)(VOID)
 *     )
 **/
ASM_PFX(InstallInterruptHandler):
        movl    %esp, %ebp
        pushl   %edi
        pushfl                              # save eflags
        cli                                 # turn off interrupts
        subl    $6, %esp                    # open some space on the stack
        movl    %esp, %edi
        sidt    %es:(%edi)                  # get fword address of IDT
        movl    %es:2(%edi), %edi            # move offset of IDT into EDI
        addl    $6, %esp                    # correct stack
        mov     4(%ebp), %eax               # Get vector number
        shll    $3, %eax                    # multiply by 8 to get offset
        addl    %eax, %edi                  # add to IDT base to get entry
        movl    8(%ebp), %eax               # load new address into IDT entry
        movw    %ax, %es:(%edi)             # write bits 15..0 of offset
        shrl    $16, %eax                   # use ax to copy 31..16 to descriptors
        movw    %ax, %es:6(%edi)            # write bits 31..16 of offset
        popfl                               # restore flags (possible enabling interrupts)
        pop     %edi
        ret

.macro JmpCommonIdtEntry  errno, vector
    /* jmp     commonIdtEntry - this must be hand coded to keep the assembler from
     *                          using a 8 bit reletive jump when the entries are
     *                          within 255 bytes of the common entry.  This must
     *                          be done to maintain the consistency of the size
     *                          of entry points...
     */
    pushl \errno
    pushl \vector
    #.byte   0e9h                         # jmp 16 bit reletive 
    #.long   commonIdtEntry - $ - $4      #  offset to jump to
    jmpl *commonIdtEntry
.endm

.align 0x02
ASM_PFX(SystemExceptionHandler):
INT0:
    JmpCommonIdtEntry  errno=0,vector=0
INT1:
    JmpCommonIdtEntry  errno=0,vector=1
INT2:
    JmpCommonIdtEntry  errno=0,vector=2
INT3:
    JmpCommonIdtEntry  errno=0,vector=3
INT4:
    JmpCommonIdtEntry  errno=0,vector=4
INT5:
    JmpCommonIdtEntry  errno=0,vector=5
INT6:
    JmpCommonIdtEntry  errno=0,vector=6
INT7:
    JmpCommonIdtEntry  errno=0,vector=7
INT8:
#   Double fault causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $8
    jmpl *commonIdtEntry
INT9:
    JmpCommonIdtEntry  errno=0,vector=9
INT10:
#   Invalid TSS causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $10
    jmpl *commonIdtEntry
INT11:
#   Segment Not Present causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $11
    jmpl *commonIdtEntry    
INT12:
#   Stack fault causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $12
    jmpl *commonIdtEntry
INT13:
#   GP fault causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $13
    jmpl *commonIdtEntry
INT14:
#   Page fault causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $14
    jmpl *commonIdtEntry
INT15:
    JmpCommonIdtEntry  errno=0,vector=15
INT16:
    JmpCommonIdtEntry  errno=0,vector=16
INT17:
#   Alignment check causes an error code to be pushed so no phony pushl necessary
    nop
    nop
    pushl    $17
    jmpl *commonIdtEntry
INT18:
    JmpCommonIdtEntry  errno=0,vector=18
INT19:
    JmpCommonIdtEntry  errno=0,vector=19
INTUnknown:
    JmpCommonIdtEntry  errno=0,vector=20
    JmpCommonIdtEntry  errno=0,vector=21
    JmpCommonIdtEntry  errno=0,vector=22
    JmpCommonIdtEntry  errno=0,vector=23
    JmpCommonIdtEntry  errno=0,vector=24
    JmpCommonIdtEntry  errno=0,vector=25
    JmpCommonIdtEntry  errno=0,vector=26
    JmpCommonIdtEntry  errno=0,vector=27
    JmpCommonIdtEntry  errno=0,vector=28
    JmpCommonIdtEntry  errno=0,vector=29
    JmpCommonIdtEntry  errno=0,vector=30
    JmpCommonIdtEntry  errno=0,vector=31

ASM_PFX(SystemTimerHandler):
    JmpCommonIdtEntry  errno=0,vector=ASM_PFX(mTimerVector)

commonIdtEntry:
# +---------------------+
# +    EFlags           +
# +---------------------+
# +    CS               +
# +---------------------+
# +    EIP              +
# +---------------------+
# +    Error Code       +
# +---------------------+
# +    Vector Number    +
# +---------------------+
# +    EBP              +
# +---------------------+ <-- EBP

  cli
  pushl %ebp
  movl  %esp, %ebp

  #
  # Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
  # is 16-byte aligned
  #
  andl    $0xfffffff0, %esp
  subl    $12, %esp

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax#
  pushl    %eax
  pushl    %ecx
  pushl    %edx
  pushl    %ebx
  leal     24(%ebp), %ecx
  pushl    %ecx                # ESP
  pushl    (%ebp)              # EBP
  pushl    %esi
  pushl    %edi

## UINT32  Gs, Fs, Es, Ds, Cs, Ss#
  movl     %ss, %eax
  pushl    %eax
  movzwl   16(%ebp), %eax
  pushl    %eax
  movl     %ds, %eax
  pushl    %eax
  movl     %es, %eax
  pushl    %eax
  movl     %fs, %eax
  pushl    %eax
  movl     %gs, %eax
  pushl    %eax

## UINT32  Eip#
  pushl    12(%ebp)

## UINT32  Gdtr[2], Idtr[2]#
  subl     $8, %esp
  sidt     (%esp)
  subl     $8, %esp
  sgdt     (%esp)

## UINT32  Ldtr, Tr#
  xorl     %eax, %eax
  strw     %ax
  pushl    %eax
  sldt     %ax
  pushl    %eax

## UINT32  EFlags#
  pushl   5*4(%ebp)

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4#
  movl %cr4, %eax
  orl  $0x208, %eax
  movl %eax, %cr4
  pushl %eax
  movl %cr3, %eax
  pushl %eax
  movl %cr2, %eax
  pushl %eax
  xorl %eax, %eax
  pushl %eax
  movl %cr0, %eax
  pushl %eax

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7#
  movl    %dr7, %eax
  pushl   %eax
## clear Dr7 while executing debugger itself
  xorl    %eax, %eax
  movl    %eax, %dr7

  movl    %dr6, %eax
  pushl   %eax
## insure all status bits in dr6 are clear...
  xorl    %eax, %eax
  movl    %eax, %dr6

  movl    %dr3, %eax
  pushl   %eax
  movl    %dr2, %eax
  pushl   %eax
  movl    %dr1, %eax
  pushl   %eax
  movl    %dr0, %eax
  pushl   %eax

## FX_SAVE_STATE_IA32 FxSaveState#
  subl $512, %esp
  movl %esp, %edi
  #.byte 0f, 0ae, 00000111y #fxsave [edi]
  fxsave (%edi)
  
## UINT32  ExceptionData#
  pushl    2*4(%ebp)

## Prepare parameter and call
  movl    %esp, %edx
  pushl   %edx
  movl    1*4(%ebp), %eax
  pushl   %eax
  cmpl    $32, %eax
  jb      CallException
  call    ASM_PFX(TimerHandler)
  jmp     ExceptionDone
CallException:
  call    ASM_PFX(ExceptionHandler)
ExceptionDone:
  addl    $8, %esp

  cli
## UINT32  ExceptionData#
  addl    $4, %esp

## FX_SAVE_STATE_IA32 FxSaveState#
  movl %esp, %esi
  #db 0fh, 0aeh, 00001110y # fxrstor [esi]
  fxrstor (%esi)
  addl $512, %esp

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7#
  popl    %eax
  movl    %eax, %dr0
  popl    %eax
  movl    %eax, %dr1
  popl    %eax
  movl    %eax, %dr2
  popl    %eax
  movl    %eax, %dr3
## skip restore of dr6.  We cleared dr6 during the context save.
  addl    $4, %esp
  popl    %eax
  movl    %eax, %dr7

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4#
  popl    %eax
  movl    %eax, %cr0
  addl    $4, %esp  # not for Cr1
  popl    %eax
  movl    %eax, %cr2
  popl    %eax
  movl    %eax, %cr3
  popl    %eax
  movl    %eax, %cr4

## UINT32  EFlags#
  popl    5*4(%ebp)

## UINT32  Ldtr, Tr#
## UINT32  Gdtr[2], Idtr[2]#
## Best not let anyone mess with these particular registers...
  addl    $24, %esp

## UINT32  Eip#
  popl    3*4(%ebp)

## UINT32  Gs, Fs, Es, Ds, Cs, Ss#
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##
  popl    %gs
  popl    %fs
  popl    %es
  popl    %ds
  popl    4*4(%ebp)
  popl    %ss

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax#
  popl    %edi
  popl    %esi
  addl    $4, %esp # not for ebp
  addl    $4, %esp # not for esp
  popl    %ebx
  popl    %edx
  popl    %ecx
  popl    %eax

  movl    %ebp, %esp
  popl    %ebp
  addl    $8, %esp
  iretl


##############################################################################
# data
##############################################################################

.align 2, 0x0
gdtr:
    .word 8*8 - 1
    .long GDT_BASE
    
##############################################################################
#   global descriptor table (GDT)
##############################################################################


.align 2, 0x90
GDT_BASE:
    .quad 0x0                   // null descriptor
    .quad 0x00cf92000000ffff    // linear data segment descriptor
    .quad 0x00cf9a000000ffff    // linear code segment descriptor
    .quad 0x00cf92000000ffff    // system data segment descriptor
    .quad 0x00cf9a000000ffff    // system code segment descriptor
    .quad 0x0                   // spare segment descriptor
    .quad 0x0
    .quad 0x0

.align 0x02

idtr:
    .word IDT_END - IDT_BASE - 1   # IDT limit
    .long IDT_BASE
    
##############################################################################
#   interrupt descriptor table (IDT)
#
#   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
#       mappings.  This implementation only uses the system timer and all other
#       IRQs will remain masked.  The descriptors for vectors 33+ are provided
#       for convenience.
##############################################################################

.align 0x02

IDT_BASE:
     .skip 256 * 16

/**
# divide by zero (INT 0)
DIV_ZERO_SEL        equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# debug exception (INT 1)
DEBUG_EXCEPT_SEL    equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# NMI (INT 2)
NMI_SEL             equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# soft breakpoint (INT 3)
BREAKPOINT_SEL      equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# overflow (INT 4)
OVERFLOW_SEL        equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# bounds check (INT 5)
BOUNDS_CHECK_SEL    equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# invalid opcode (INT 6)
INVALID_OPCODE_SEL  equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# device not available (INT 7)
DEV_NOT_AVAIL_SEL   equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# double fault (INT 8)
DOUBLE_FAULT_SEL    equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1      equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# invalid TSS (INT 0ah)
INVALID_TSS_SEL     equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# stack fault (INT 0ch)
STACK_FAULT_SEL     equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# general protection (INT 0dh)
GP_FAULT_SEL        equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# page fault (INT 0eh)
PAGE_FAULT_SEL      equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2      equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# floating point error (INT 10h)
FLT_POINT_ERR_SEL   equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # type = 386 interrupt gate, present
        dw 0            # offset 31:16

# alignment check (INT 11h)
ALIGNMENT_CHECK_SEL equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# machine check (INT 12h)
MACHINE_CHECK_SEL   equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# SIMD floating-point exception (INT 13h)
SIMD_EXCEPTION_SEL  equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

REPEAT  (32 - 20)
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16
ENDM

# 72 unspecified descriptors
        db (72 * 8) dup(0)
        
# IRQ 0 (System timer) - (INT 68h)
IRQ0_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 1 (8042 Keyboard controller) - (INT 69h)
IRQ1_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 8 (RTC Alarm) - (INT 70h)
IRQ8_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 9 - (INT 71h)
IRQ9_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 10 - (INT 72h)
IRQ10_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 11 - (INT 73h)
IRQ11_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 12 (PS/2 mouse) - (INT 74h)
IRQ12_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 13 (Floating point error) - (INT 75h)
IRQ13_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 14 (Secondary IDE) - (INT 76h)
IRQ14_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

# IRQ 15 (Primary IDE) - (INT 77h)
IRQ15_SEL            equ $-IDT_BASE
        dw 0            # offset 15:0
        dw SYS_CODE_SEL # selector 15:0
        db 0            # 0 for interrupt gate
        db 0eh OR 80h   # (10001110)type = 386 interrupt gate, present
        dw 0            # offset 31:16

        db (1 * 8) dup(0)

**/
IDT_END:
