#------------------------------------------------------------------------------
#*
#*   Copyright 2006, Intel Corporation                                                         
#*   All rights reserved. This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*    CpuInterrupt.S
#*  
#*   Abstract:
#*
#------------------------------------------------------------------------------

#PUBLIC SystemTimerHandler
#PUBLIC SystemExceptionHandler
#EXTERNDEF mExceptionCodeSize:DWORD

#EXTERN TimerHandler: NEAR
#EXTERN ExceptionHandler: NEAR
#EXTERN mTimerVector: DWORD

	.data
	.globl ASM_PFX(mExceptionCodeSize)
ASM_PFX(mExceptionCodeSize): .long  9

	.text
	.globl ASM_PFX(InitDescriptor)

ASM_PFX(InitDescriptor):
        movl    $GDT_BASE,%eax              # EAX=PHYSICAL address of gdt
        movl    %eax, gdtr + 2              # Put address of gdt into the gdtr
        lgdt    gdtr
        movl    $IDT_BASE,%eax              # EAX=PHYSICAL address of idt
        movl    %eax, idtr + 2              # Put address of idt into the idtr
        lidt    idtr
        ret

# VOID
# InstallInterruptHandler (
#     UINTN Vector,
#     VOID  (*Handler)(VOID)
#     )
	.globl ASM_PFX(InstallInterruptHandler)
ASM_PFX(InstallInterruptHandler):
#  Vector:DWORD @ 4(%esp)
#  Handler:DWORD @ 8(%esp)

        push    %edi
        pushf                               # save eflags
        cli                                 # turn off interrupts
        subl    $6,%esp                     # open some space on the stack
        movl    %esp,%edi
        sidt    (%edi)                      # get fword address of IDT
        movl    2(%edi), %edi               # move offset of IDT into EDI
        addl    $6,%esp                     # correct stack
        movl    12(%esp),%eax               # Get vector number
        shl     $3,%eax                     # multiply by 8 to get offset
        addl    %eax,%edi                   # add to IDT base to get entry
        movl    16(%esp),%eax               # load new address into IDT entry
        movw    %ax,(%edi)                  # write bits 15..0 of offset
        shrl    $16,%eax                    # use ax to copy 31..16 to descriptors
        movw    %ax,6(%edi)                 # write bits 31..16 of offset
        popf                                # restore flags (possible enabling interrupts)
        pop     %edi
        ret

	.macro JmpCommonIdtEntry
    # jmp     commonIdtEntry - this must be hand coded to keep the assembler from
    #                          using a 8 bit reletive jump when the entries are
    #                          within 255 bytes of the common entry.  This must
    #                          be done to maintain the consistency of the size
    #                          of entry points...
    .byte     0xe9                        # jmp 16 bit reletive 
    .long     commonIdtEntry - . - 4      #  offset to jump to
	.endm

    .align 2
	.globl ASM_PFX(SystemExceptionHandler)
ASM_PFX(SystemExceptionHandler):
INT0:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x0 
    JmpCommonIdtEntry
#    db      0e9h                        # jmp 16 bit reletive 
#    dd      commonIdtEntry - $ - 4      #  offset to jump to
    
INT1:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x1 
    JmpCommonIdtEntry
    
INT2:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x2 
    JmpCommonIdtEntry
    
INT3:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x3 
    JmpCommonIdtEntry
    
INT4:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x4 
    JmpCommonIdtEntry
    
INT5:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x5 
    JmpCommonIdtEntry
    
INT6:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x6 
    JmpCommonIdtEntry
    
INT7:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x7 
    JmpCommonIdtEntry
    
INT8:
#   Double fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $0x8 
    JmpCommonIdtEntry
    
INT9:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $0x9 
    JmpCommonIdtEntry
    
INT10:
#   Invalid TSS causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $10
    JmpCommonIdtEntry
    
INT11:
#   Segment Not Present causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $11
    JmpCommonIdtEntry
    
INT12:
#   Stack fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $12
    JmpCommonIdtEntry
    
INT13:
#   GP fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $13
    JmpCommonIdtEntry
    
INT14:
#   Page fault causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $14
    JmpCommonIdtEntry
    
INT15:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $15
    JmpCommonIdtEntry
    
INT16:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $16
    JmpCommonIdtEntry
    
INT17:
#   Alignment check causes an error code to be pushed so no phony push necessary
    nop
    nop
    pushl    $17
    JmpCommonIdtEntry
    
INT18:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $18
    JmpCommonIdtEntry
    
INT19:
    pushl    $0x0      # push error code place holder on the stack
    pushl    $19
    JmpCommonIdtEntry

INTUnknown:
	.rept (32 - 20)
    pushl    $0x0      # push error code place holder on the stack
#    push    xxh     # push vector number
    .byte    0x6a
    .byte    ( . - INTUnknown - 3 ) / 9 + 20 # vector number
    JmpCommonIdtEntry
	.endr

	.globl ASM_PFX(SystemTimerHandler)
ASM_PFX(SystemTimerHandler):
    pushl    $0
    pushl    $mTimerVector
    JmpCommonIdtEntry

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
  push %ebp
  movl %esp,%ebp

  #
  # Align stack to make sure that EFI_FX_SAVE_STATE_IA32 of EFI_SYSTEM_CONTEXT_IA32
  # is 16-byte aligned
  #
  andl    $0xfffffff0,%esp
  subl    $12,%esp

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax#
  push    %eax
  push    %ecx
  push    %edx
  push    %ebx
  leal    6*4(%ebp),%ecx
  push    %ecx                          # ESP
  push    (%ebp)                        # EBP
  push    %esi
  push    %edi

## UINT32  Gs, Fs, Es, Ds, Cs, Ss#
  movw  %ss,%ax
  push  %eax
  movzx 4*4(%ebp),%eax
  push %eax
  movw  %ds,%ax
  push %eax
  movw  %es,%ax
  push %eax
  movw %fs,%ax
  push %eax
  movw %gs,%ax
  push %eax

## UINT32  Eip#
  pushl   3*4(%ebp)

## UINT32  Gdtr[2], Idtr[2]#
  subl  $8,%esp
  sidt (%esp)
  subl  $8,%esp
  sgdt (%esp)

## UINT32  Ldtr, Tr#
  xorl  %eax, %eax
  str  %ax
  push %eax
  sldt %eax
  push %eax

## UINT32  EFlags#
  pushl  5*4(%ebp)

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4#
  mov  %cr4,%eax
  orl  $0x208,%eax
  mov  %eax,%cr4
  push %eax
  mov  %cr3,%eax
  push %eax
  mov  %cr2,%eax
  push %eax
  xor  %eax, %eax
  push %eax
  mov  %cr0,%eax
  push %eax

## UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7#
  mov     %dr7,%eax
  push    %eax
## clear Dr7 while executing debugger itself
  xor     %eax, %eax
  mov     %eax,%dr7

  mov     %dr6,%eax
  push    %eax
## insure all status bits in dr6 are clear...
  xor     %eax, %eax
  mov     %eax,%dr6

  mov     %dr3,%eax
  push    %eax
  mov     %dr2,%eax
  push    %eax
  mov     %dr1,%eax
  push    %eax
  mov     %dr0,%eax
  push    %eax

## FX_SAVE_STATE_IA32 FxSaveState;
  sub     $512,%esp
  mov     %esp,%edi
  fxsave (%edi)

## UINT32  ExceptionData;
  pushl    2*4(%ebp)

## Prepare parameter and call
  mov     %esp,%edx
  push    %edx
  mov     1*4(%ebp),%eax
  push    %eax
  cmpl    $32,%eax
  jb      CallException
  call    TimerHandler
  jmp     ExceptionDone
CallException:
  call    ExceptionHandler
ExceptionDone:
  addl    $8,%esp

  cli
## UINT32  ExceptionData;
  addl     $4,%esp

## FX_SAVE_STATE_IA32 FxSaveState;
  mov      %esp,%esi
  fxrstor  (%esi)
  addl     $512,%esp

#; UINT32  Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
  pop     %eax
  mov     %eax,%dr0
  pop     %eax
  mov     %eax,%dr1
  pop     %eax
  mov     %eax,%dr2
  pop     %eax
  mov     %eax,%dr3
## skip restore of dr6.  We cleared dr6 during the context save.
  addl    $4,%esp
  pop     %eax
  mov     %eax,%dr7

## UINT32  Cr0, Cr1, Cr2, Cr3, Cr4;
  pop     %eax
  mov     %eax,%cr0
  addl    $4,%esp    # not for Cr1
  pop     %eax
  mov     %eax,%cr2
  pop     %eax
  mov     %eax,%cr3
  pop     %eax
  mov     %eax,%cr4

## UINT32  EFlags;
  popl     5*4(%ebp)

## UINT32  Ldtr, Tr;
## UINT32  Gdtr[2], Idtr[2];
## Best not let anyone mess with these particular registers...
  addl     $24,%esp

## UINT32  Eip;
  popl     3*4(%ebp)

## UINT32  Gs, Fs, Es, Ds, Cs, Ss;
## NOTE - modified segment registers could hang the debugger...  We
##        could attempt to insulate ourselves against this possibility,
##        but that poses risks as well.
##
  pop     %gs
  pop     %fs
  pop     %es
  pop     %ds
  popl    4*4(%ebp)
  pop     %ss

## UINT32  Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
  pop     %edi
  pop     %esi
  addl    $4,%esp   # not for ebp
  addl    $4,%esp   # not for esp
  pop     %ebx
  pop     %edx
  pop     %ecx
  pop     %eax

  mov     %ebp,%esp
  pop     %ebp
  addl    $8,%esp
  iret


#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
# data
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.data
        .align 4

gdtr:	.short GDT_END - GDT_BASE - 1   # GDT limit
        .long 0                         # (GDT base gets set above)
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#   global descriptor table (GDT)
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        .align 4

	.globl GDT_BASE
GDT_BASE:
# null descriptor
NULL_SEL            = .-GDT_BASE
        .short 0            # limit 15:0
        .short 0            # base 15:0
        .byte 0            # base 23:16
        .byte 0            # type
        .byte 0            # limit 19:16, flags
        .byte 0            # base 31:24

# linear data segment descriptor
LINEAR_SEL      = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0x092         # present, ring 0, data, expand-up, writable
        .byte 0x0CF                 # page-granular, 32-bit
        .byte 0

# linear code segment descriptor
LINEAR_CODE_SEL = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0x09A         # present, ring 0, data, expand-up, writable
        .byte 0x0CF                 # page-granular, 32-bit
        .byte 0

# system data segment descriptor
SYS_DATA_SEL    = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0x092         # present, ring 0, data, expand-up, writable
        .byte 0x0CF                 # page-granular, 32-bit
        .byte 0

# system code segment descriptor
SYS_CODE_SEL    = .-GDT_BASE
        .short 0x0FFFF       # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0x09A         # present, ring 0, data, expand-up, writable
        .byte 0x0CF                 # page-granular, 32-bit
        .byte 0

# spare segment descriptor
SPARE3_SEL  = .-GDT_BASE
        .short 0            # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0            # present, ring 0, data, expand-up, writable
        .byte 0            # page-granular, 32-bit
        .byte 0

# spare segment descriptor
SPARE4_SEL  = .-GDT_BASE
        .short 0            # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0            # present, ring 0, data, expand-up, writable
        .byte 0            # page-granular, 32-bit
        .byte 0

# spare segment descriptor
SPARE5_SEL  = .-GDT_BASE
        .short 0            # limit 0xFFFFF
        .short 0            # base 0
        .byte 0
        .byte 0            # present, ring 0, data, expand-up, writable
        .byte 0            # page-granular, 32-bit
        .byte 0

GDT_END:

        .align 4



idtr:	.short IDT_END - IDT_BASE - 1   # IDT limit
        .long 0                        # (IDT base gets set above)
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#   interrupt descriptor table (IDT)
#
#   Note: The hardware IRQ's specified in this table are the normal PC/AT IRQ
#       mappings.  This implementation only uses the system timer and all other
#       IRQs will remain masked.  The descriptors for vectors 33+ are provided
#       for convenience.
#;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#idt_tag .byte "IDT",0     
        .align 4

.globl IDT_BASE
IDT_BASE:
# divide by zero (INT 0)
DIV_ZERO_SEL        = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# debug exception (INT 1)
DEBUG_EXCEPT_SEL    = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# NMI (INT 2)
NMI_SEL             = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# soft breakpoint (INT 3)
BREAKPOINT_SEL      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# overflow (INT 4)
OVERFLOW_SEL        = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# bounds check (INT 5)
BOUNDS_CHECK_SEL    = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# invalid opcode (INT 6)
INVALID_OPCODE_SEL  = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# device not available (INT 7)
DEV_NOT_AVAIL_SEL   = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# double fault (INT 8)
DOUBLE_FAULT_SEL    = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# Coprocessor segment overrun - reserved (INT 9)
RSVD_INTR_SEL1      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# invalid TSS (INT 0ah)
INVALID_TSS_SEL     = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# segment not present (INT 0bh)
SEG_NOT_PRESENT_SEL = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# stack fault (INT 0ch)
STACK_FAULT_SEL     = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# general protection (INT 0dh)
GP_FAULT_SEL        = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# page fault (INT 0eh)
PAGE_FAULT_SEL      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# Intel reserved - do not use (INT 0fh)
RSVD_INTR_SEL2      = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# floating point error (INT 0x10)
FLT_POINT_ERR_SEL   = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # type = 386 interrupt gate, present
        .short 0            # offset 31:16

# alignment check (INT 0x11)
ALIGNMENT_CHECK_SEL = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# machine check (INT 0x12)
MACHINE_CHECK_SEL   = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# SIMD floating-point exception (INT 0x13)
SIMD_EXCEPTION_SEL  = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

	.rept  (32 - 20)
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16
	.endr

# 72 unspecified descriptors
	.rept 72 * 8
        .byte 0
	.endr
        
# IRQ 0 (System timer) - (INT 0x68)
IRQ0_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 1 (8042 Keyboard controller) - (INT 0x69)
IRQ1_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# Reserved - IRQ 2 redirect (IRQ 2) - DO NOT USE!!! - (INT 6ah)
IRQ2_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 3 (COM 2) - (INT 6bh)
IRQ3_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 4 (COM 1) - (INT 6ch)
IRQ4_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 5 (LPT 2) - (INT 6dh)
IRQ5_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 6 (Floppy controller) - (INT 6eh)
IRQ6_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 7 (LPT 1) - (INT 6fh)
IRQ7_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 8 (RTC Alarm) - (INT 0x70)
IRQ8_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 9 - (INT 0x71)
IRQ9_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 10 - (INT 0x72)
IRQ10_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 11 - (INT 0x73)
IRQ11_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 12 (PS/2 mouse) - (INT 0x74)
IRQ12_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 13 (Floating point error) - (INT 0x75)
IRQ13_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 14 (Secondary IDE) - (INT 0x76)
IRQ14_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

# IRQ 15 (Primary IDE) - (INT 0x77)
IRQ15_SEL            = .-IDT_BASE
        .short 0            # offset 15:0
        .short SYS_CODE_SEL # selector 15:0
        .byte 0            # 0 for interrupt gate
        .byte 0x0e | 0x80   # (10001110)type = 386 interrupt gate, present
        .short 0            # offset 31:16

	.rept 1 * 8
        .byte 0
	.endr

IDT_END:

