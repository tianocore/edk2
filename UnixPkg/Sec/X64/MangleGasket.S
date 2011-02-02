#------------------------------------------------------------------------------
#
# This template was generated from GasketEfiTemplate.c Unix x86_64 ABI
#
# The EFI_UNIX_THUNK_PROTOCOL member functions call these these generic assembly 
# routines.  
#
# Some OS X POSIX calls get name mangled in C code and we need to fill in a C global
# to get the correct binding (does not work from assembly). So we have 4 functions
# that do an indirect call, while the others call POSIX APIs directly
#
# movq  _gUnixRmDir@GOTPCREL(%rip), %rax 
#
#
# UNIX Arg passing: RCX, RDX,  R8,  R9
# EFI  Arg passing: RDI, RSI, RDX, RCX, R8, R9
#                   Callee allocates 32 bytes on stack to spill registers
# RSI, RDI calle-save on EFI, scatch on UNIX callign
#
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

//
// Gaskets are EFI ABI to UNIX ABI calls
// EFI ABI code will sub 40 (0x28) from %rsp before calling a function
//  This is the 32 (0x20) byte to spill registers and 8 bytes to align stack on 16 byte boundry.
//
  .text

// 32 byte shadow to spill rcx-r9, 8 bytes to align stack on 16 byte boundry
// Any call with 0 - 4 arguments allocates 40 bytes on the stack.
// For more than 4 args you always have to increase in quanta of 16 so 5 or 6 args is 56, 
// 7 or 8 args is 72, and 9 or 10 args is 88
#define EFI_STACK_SHADOW_SPACE          40  
#define EFI_STACK_SHADOW_SPACE_5_6      56  
#define EFI_STACK_SHADOW_SPACE_7_8      72 
#define EFI_STACK_SHADOW_SPACE_9_10     88 



  .text


ASM_GLOBAL ASM_PFX(Gasketrmdir)
ASM_PFX(Gasketrmdir):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 

  movq  ASM_PFX(gUnixRmDir)@GOTPCREL(%rip), %rax  // Get function name mangled by C
  movq  (%rax), %rax
  call  *%rax


  popq  %rdi          // restore state
  popq  %rsi
  popq  %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketopendir)
ASM_PFX(Gasketopendir):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 

  movq  ASM_PFX(gUnixOpenDir)@GOTPCREL(%rip), %rax  // Get function name mangled by C
  movq  (%rax), %rax
  call  *%rax
  
  popq  %rdi             // restore state
  popq  %rsi
  popq  %rbp
  ret



ASM_GLOBAL ASM_PFX(Gasketstat)
ASM_PFX(Gasketstat):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  movq  ASM_PFX(gUnixStat)@GOTPCREL(%rip), %rax   // Get function name mangled by C
  movq  (%rax), %rax
  call  *%rax
  

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret
  
  
ASM_GLOBAL ASM_PFX(Gasketstatfs)
ASM_PFX(Gasketstatfs):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  movq  ASM_PFX(gUnixStatFs)@GOTPCREL(%rip), %rax   // Get function name mangled by C
  movq  (%rax), %rax
  call  *%rax
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret
  
ASM_GLOBAL ASM_PFX(Gasketrewinddir)
ASM_PFX(Gasketrewinddir):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi
  
  movq    %rcx, %rdi    // Swizzle args 

  movq  ASM_PFX(gUnixRewinddir)@GOTPCREL(%rip), %rax   // Get function name mangled by C
  movq  (%rax), %rax
  call  *%rax
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret
  
ASM_GLOBAL ASM_PFX(Gasketreaddir)
ASM_PFX(Gasketreaddir):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi
  
  movq    %rcx, %rdi    // Swizzle args 

  movq  ASM_PFX(gUnixReaddir)@GOTPCREL(%rip), %rax   // Get function name mangled by C
  movq  (%rax), %rax
  call  *%rax
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret
  

ASM_GLOBAL ASM_PFX(GasketmsSleep)
ASM_PFX(GasketmsSleep):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call  ASM_PFX(msSleep)
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret
  
  
ASM_GLOBAL ASM_PFX(Gasketexit)
ASM_PFX(Gasketexit):
  movq    %rcx, %rdi    // Swizzle args 
  call  ASM_PFX(exit)           // Less to do as we will never return to EFI ABI world
LDEAD_LOOP:
  jmp  LDEAD_LOOP       // _exit should never return



ASM_GLOBAL ASM_PFX(GasketSetTimer)
ASM_PFX(GasketSetTimer):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call  ASM_PFX(SetTimer)
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret

  
ASM_GLOBAL ASM_PFX(GasketGetLocalTime)
ASM_PFX(GasketGetLocalTime):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call  ASM_PFX(GetLocalTime)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret

  

ASM_GLOBAL ASM_PFX(Gasketgmtime)
ASM_PFX(Gasketgmtime):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call  ASM_PFX(localtime)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret



ASM_GLOBAL ASM_PFX(GasketGetTimeZone)
ASM_PFX(GasketGetTimeZone):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call  ASM_PFX(GetTimeZone)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret

  
ASM_GLOBAL ASM_PFX(GasketGetDayLight)
ASM_PFX(GasketGetDayLight):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call  ASM_PFX(GetDayLight)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketpoll)
ASM_PFX(Gasketpoll):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(poll)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret



ASM_GLOBAL ASM_PFX(Gasketread)
ASM_PFX(Gasketread):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(read)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketwrite)
ASM_PFX(Gasketwrite):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(write)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketgetenv)
ASM_PFX(Gasketgetenv):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(getenv)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketopen)
ASM_PFX(Gasketopen):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(open)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketlseek)
ASM_PFX(Gasketlseek):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(lseek)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketftruncate)
ASM_PFX(Gasketftruncate):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(ftruncate)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketclose)
ASM_PFX(Gasketclose):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(close)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret



ASM_GLOBAL ASM_PFX(Gasketmkdir)
ASM_PFX(Gasketmkdir):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(mkdir)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketunlink)
ASM_PFX(Gasketunlink):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(unlink)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketGetErrno)
ASM_PFX(GasketGetErrno):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call  ASM_PFX(GetErrno)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(Gasketclosedir)
ASM_PFX(Gasketclosedir):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(closedir)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketrename)
ASM_PFX(Gasketrename):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(rename)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketmktime)
ASM_PFX(Gasketmktime):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(mktime)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketfsync)
ASM_PFX(Gasketfsync):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(fsync)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketchmod)
ASM_PFX(Gasketchmod):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(chmod)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketutime)
ASM_PFX(Gasketutime):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(utime)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gaskettcflush)
ASM_PFX(Gaskettcflush):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(tcflush)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUgaCreate)
ASM_PFX(GasketUgaCreate):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(UgaCreate)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketperror)
ASM_PFX(Gasketperror):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(perror)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketioctl)
ASM_PFX(Gasketioctl):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(UnixIoCtl1)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gasketfcntl)
ASM_PFX(Gasketfcntl):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(UnixFcntl1)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret



ASM_GLOBAL ASM_PFX(Gasketcfsetispeed)
ASM_PFX(Gasketcfsetispeed):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(cfsetispeed)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret



ASM_GLOBAL ASM_PFX(Gasketcfsetospeed)
ASM_PFX(Gasketcfsetospeed):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(cfsetospeed)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gaskettcgetattr)
ASM_PFX(Gaskettcgetattr):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(tcgetattr)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(Gaskettcsetattr)
ASM_PFX(Gaskettcsetattr):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(tcsetattr)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUnixPeCoffGetEntryPoint)
ASM_PFX(GasketUnixPeCoffGetEntryPoint):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(SecPeCoffGetEntryPoint)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret



ASM_GLOBAL ASM_PFX(GasketUnixPeCoffRelocateImageExtraAction)
ASM_PFX(GasketUnixPeCoffRelocateImageExtraAction):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(SecPeCoffRelocateImageExtraAction)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUnixPeCoffUnloadImageExtraAction)
ASM_PFX(GasketUnixPeCoffUnloadImageExtraAction):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(SecPeCoffLoaderUnloadImageExtraAction)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret





ASM_GLOBAL ASM_PFX(Gasketsocket)
ASM_PFX(Gasketsocket):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(socket)
  

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret



ASM_GLOBAL ASM_PFX(Gasketgetifaddrs)
ASM_PFX(Gasketgetifaddrs):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(getifaddrs)
  

  popq  %rbp
  popq  %rsi
  popq  %rbp
  ret



ASM_GLOBAL _Gasketfreeifaddrs
_Gasketfreeifaddrs:
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    _freeifaddrs
  

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret





ASM_GLOBAL ASM_PFX(GasketUgaClose)
ASM_PFX(GasketUgaClose):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(UgaClose)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret




ASM_GLOBAL ASM_PFX(GasketUgaSize)
ASM_PFX(GasketUgaSize):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(UgaSize)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUgaCheckKey)
ASM_PFX(GasketUgaCheckKey):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(UgaCheckKey)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUgaGetKey)
ASM_PFX(GasketUgaGetKey):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(UgaGetKey)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketUgaKeySetState)
ASM_PFX(GasketUgaKeySetState):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(UgaKeySetState)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUgaRegisterKeyNotify)
ASM_PFX(GasketUgaRegisterKeyNotify):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(UgaRegisterKeyNotify)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret





ASM_GLOBAL ASM_PFX(GasketUgaBlt)
ASM_PFX(GasketUgaBlt):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(UgaBlt)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUgaCheckPointer)
ASM_PFX(GasketUgaCheckPointer):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(UgaCheckPointer)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUgaGetPointerState)
ASM_PFX(GasketUgaGetPointerState):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(UgaGetPointerState)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketUnixEnableInterrupt)
ASM_PFX(GasketUnixEnableInterrupt):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(UnixEnableInterrupt)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketUnixDisableInterrupt)
ASM_PFX(GasketUnixDisableInterrupt):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(UnixDisableInterrupt)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

//
// UNIX ABI to EFI ABI call
//
// UINTN
// ReverseGasketUint64 (
//   void *Api,
//   UINTN Arg1
//   );
ASM_GLOBAL ASM_PFX(ReverseGasketUint64)
ASM_PFX(ReverseGasketUint64):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  movq    %rdi, %rax    // Swizzle args 
  movq    %rsi, %rcx     

  subq  $32, %rsp        // 32-byte shadow space
  call  *%rax
  addq  $32, %rsp

  popq   %rbp          
  ret

//
// UNIX ABI to EFI ABI call
//
// UINTN
// ReverseGasketUint64Uint64 (
//   void *Api,
//   UINTN Arg1
//   UINTN Arg2
//   );
ASM_GLOBAL ASM_PFX(ReverseGasketUint64Uint64)
ASM_PFX(ReverseGasketUint64Uint64):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  movq    %rdi, %rax    // Swizzle args 
  movq    %rsi, %rcx     

  subq  $32, %rsp        // 32-byte shadow space
  call  *%rax
  addq  $32, %rsp

  popq   %rbp          
  ret
  

// Sec PPI Callbacks

ASM_GLOBAL ASM_PFX(GasketSecUnixPeiLoadFile)
ASM_PFX(GasketSecUnixPeiLoadFile):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(SecUnixPeiLoadFile)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret



ASM_GLOBAL ASM_PFX(GasketSecUnixPeiAutoScan)
ASM_PFX(GasketSecUnixPeiAutoScan):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(SecUnixPeiAutoScan)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketSecUnixUnixThunkAddress)
ASM_PFX(GasketSecUnixUnixThunkAddress):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(SecUnixUnixThunkAddress)
  

  popq    %rdi
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecPeiReportStatusCode)
ASM_PFX(GasketSecPeiReportStatusCode):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx
  movq    $0,   %r8     // BugBug: This should come from the stack
  movq    $0,   %r9     //         But we can cheat since they are optional for bringup....

  call    ASM_PFX(SecPeiReportStatusCode)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecUnixFdAddress)
ASM_PFX(GasketSecUnixFdAddress):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(SecUnixFdAddress)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret




ASM_GLOBAL ASM_PFX(GasketSecTemporaryRamSupport)
ASM_PFX(GasketSecTemporaryRamSupport):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(SecTemporaryRamSupport)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret





