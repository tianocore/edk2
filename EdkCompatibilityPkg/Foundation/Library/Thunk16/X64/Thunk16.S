#*****************************************************************************
#*
#*   Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
#*   This program and the accompanying materials                          
#*   are licensed and made available under the terms and conditions of the BSD License         
#*   which accompanies this distribution.  The full text of the license may be found at        
#*   http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                             
#*   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*   WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*   
#*   Module Name:
#*
#*    Thunk.S
#*  
#*   Abstract:
#*  
#*    Real mode thunk
#*  
#*****************************************************************************
#include <EfiBind.h>

#ifndef __APPLE__

    .data

.globl ASM_PFX(mCode16Size)

.data
mCode16Size:    .long   _Code16End - _Code16Addr


NullSegSel:     .quad   0
_16CsSegSel:    
                .word   -1
                .word   0
                .byte   0
                .byte   0x9b
                .byte   0x8f            #16-bit segment
                .byte   0
_16DsSegSel:
                .word   -1
                .word   0
                .byte   0
                .byte   0x93
                .byte   0x8f           #16-bit segment
                .byte   0

_16Gdtr:
                .word      _16Gdtr - NullSegSel - 1
                .long      NullSegSel
    .code: 

#IA32_REGS   STRUC   4t
#_EDI        DD      ?
#_ESI        DD      ?
#_EBP        DD      ?
#_ESP        DD      ?
#_EBX        DD      ?
#_EDX        DD      ?
#_ECX        DD      ?
#_EAX        DD      ?
#_DS         DW      ?
#_ES         DW      ?
#_FS         DW      ?
#_GS         DW      ?
#_RFLAGS     DQ      ?
#_EIP        DD      ?
#_CS         DW      ?
#_SS         DW      ?
#IA32_REGS   ENDS

#_STK16      STRUC   1t
#RetEip      DD      ?
#RetCs       DW      ?
#ThunkFlags  DW      ?
#SavedGdtr   FWORD   ?
#Resvd1      DW      ?
#SavedCr0    DD      ?
#SavedCr4    DD      ?
#_STK16      ENDS

ASM_PFX(Thunk16):
      push   %rbp
      push   %rbx
      push   %rsi
      push   %rdi
      push   %r12
      push   %r13
      push   %r14
      push   %r15
      pushq  %fs
      pushq  %gs
      movl   %ds,%r12d
      movl   %es,%r13d
      movl   %ss,%r14d
      mov    %rsp,%r15
      mov    %rcx,%rsi
      movzwq 0x36(%rsi),%r10            #movzx   r10, (IA32_REGS ptr [rsi])._SS
      xor    %rdi,%rdi
      mov    0xc(%rsi),%edi             #mov     edi, (IA32_REGS ptr [rsi])._ESP
      add    $0xffffffffffffffb0,%rdi   #add     rdi, - sizeof (IA32_REGS) - sizeof (_STK16)
      push   %rdi
      imul   $0x10,%r10,%rax
      add    %rax,%rdi
      pushq  $0xe                       #push    sizeof (IA32_REGS) / 4
      pop    %rcx
      rep movsl %ds:(%rsi),%es:(%rdi)
      #; copy eflags to stack frame
      mov    -16(%rsi), %rax
      mov    %rax, -80(%rsi)
      pop    %rbx                       #rbx <- 16-bit stack offset
      lea    Label,%eax                 #42 <_Thunk16+0x42>
      stos   %eax,%es:(%rdi)
      movl   %cs,%eax                   #return segment
      stos   %ax,%es:(%rdi)
      mov    %edx,%eax                  #THUNK Flags
      stos   %ax,%es:(%rdi)
      sgdt  0x58(%rsp)                  #save GDTR
      mov    0x58(%rsp),%rax
      stos   %rax,%es:(%rdi)
      mov    %cr0,%rax                  #save CR0
      mov    %eax,%esi                  #esi <- CR0 to set
      stos   %eax,%es:(%rdi)
      mov    %cr4,%rax                  #save CR4
      stos   %eax,%es:(%rdi)
      sidt   0x58(%rsp)                 #save IDTR
      and    $0x7ffffffe,%esi           #clear PE & PG bits
      mov    %r10,%rdi                  #rdi <- 16-bit stack segment
      shl    $0x10,%r8
      push   %r8                        #far jmp address
      lea    Label_16Bit,%eax           
      push   %rax
      movw   $0x8,0x4(%rsp)
      lgdt   _16Gdtr                    #bugbug: may not match.
      lret   
Label_16Bit:
      .byte  0x66
      movl   $0xc0000080,%ecx
      mov    %rsi,%cr0                  #disable PE & PG        
      rdmsr  
      and    $0xfe,%ah
      wrmsr                             #clear LME bit
      mov    %cr4,%rax
      and    $0xcf,%al                  #clear PAE & PSE
      mov    %rax,%cr4
      lret   

Label:
      xor    %rax,%rax
      movw   %ss,%ax
      shl    $0x4,%eax
      add    %esp,%eax
      mov    %r15,%rsp
      lidt   0x58(%rsp)
      movl   %r12d,%ds
      movl   %r13d,%es
      movl   %r14d,%ss
      popq   %gs
      popq   %fs
      pop    %r15
      pop    %r14
      pop    %r13
      pop    %r12
      pop    %rdi
      pop    %rsi
      pop    %rbx
      pop    %rbp
      retq   


    .p2align 4

_Code16Addr:
ASM_PFX(RealMode):
    movl   %edi,%ss
    mov    %bx,%sp                     #set up 16-bit stack
   .byte   0x2e 
   .byte   0x0f 
   .byte   0x01
   .byte   0x1e
   .word   _16Idtr - _Code16Addr       #lidt _16Idtr
   .byte   0x66
   .byte   0x61                        #popad
   .byte   0x1f                        #pop ds
   .byte   0x07                        #pop es
    popq   %fs
    popq   %gs
    sub    64, %esp
    .byte  0x66, 0x9d                  #popfd
    add    $0x4,%esp                   #skip high part of RFLAGS
   .byte   0x67                        #; test    (_STK16 ptr [esp + STACK_PARAM_SIZE + sizeof(IA32_REGS)]).ThunkFlags, 1
   .byte   0xf7    
   .byte   0x44
   .byte   0x24
   .byte   0x4e
   .byte   0x01
   .byte   0x00
    jz      1f
    pushfq                             #pushf, actually, when it's INT#
1:
   .byte   0x0e                        #push cs
   .byte   0x68                        #push /iw
   .word   FarCallRet - _Code16Addr
    jz     2f
   .byte   0x66
    ljmp   *70(%esp)
2:    
   .byte   0x66
    ljmp   *68(%esp)
FarCallRet: 
    add    64, %esp
   .byte   0x66
    push   $0x00                       #push a dword of zero
   .byte   0x66
    pushf                              #pushfd, actually
    pushq  %gs
    pushq  %fs
   .byte   0x06                        #push %es
   .byte   0x1e                        #push %ds
   .byte   0x66
   .byte   0x60
    cli
   .byte   0x66                        #sizeof (IA32_REGS) = 13 * 4 = 52
    lgdt   64(%esp)                    #lgdt    (_STK16 ptr [esp + sizeof (IA32_REGS)]).SavedGdtr
   .byte   0x66
    mov    76(%esp), %eax
    mov    %rax, %cr4
   .byte   0x66
    mov    $0xc0000080, %ecx
    rdmsr
    orb    $1, %ah
    wrmsr
   .byte   0x66
    mov    72(%esp), %eax
    mov    %rax, %cr0                   #restore CR0
   .byte   0x66
    ljmpl  *52(%esp)                    

_16Idtr:
   .word 0x3ff                          #FWORD   (1 SHL 10) - 1
   .byte 0x00

#endif
