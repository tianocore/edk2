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
# movq	_gUnixRmDir@GOTPCREL(%rip), %rax
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
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	movq	ASM_PFX(gUnixRmDir)@GOTPCREL(%rip), %rax  // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax

	pushq	  %rdi          // restore state
	pushq	  %rsi
	ret


ASM_GLOBAL ASM_PFX(Gasketopendir)
ASM_PFX(Gasketopendir):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	movq	ASM_PFX(gUnixOpenDir)@GOTPCREL(%rip), %rax  // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax

	popq  %rdi             // restore state
	popq  %rsi
  ret



ASM_GLOBAL ASM_PFX(Gasketstat)
ASM_PFX(Gasketstat):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	movq	ASM_PFX(gUnixStat)@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax

	popq  %rdi            // restore state
	popq  %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketstatfs)
ASM_PFX(Gasketstatfs):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	movq	ASM_PFX(gUnixStatFs)@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax

	popq  %rdi            // restore state
	popq  %rsi
  ret

ASM_GLOBAL ASM_PFX(Gasketrewinddir)
ASM_PFX(Gasketrewinddir):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	movq	ASM_PFX(gUnixRewinddir)@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax

	popq  %rdi            // restore state
	popq  %rsi
  ret

ASM_GLOBAL ASM_PFX(Gasketreaddir)
ASM_PFX(Gasketreaddir):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	movq	ASM_PFX(gUnixReaddir)@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax

	popq  %rdi            // restore state
	popq  %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketmsSleep)
ASM_PFX(GasketmsSleep):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	ASM_PFX(msSleep)

	popq  %rdi            // restore state
	popq  %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketexit)
ASM_PFX(Gasketexit):
  movq    %rcx, %rdi    // Swizzle args
	call	ASM_PFX(exit)           // Less to do as we will never return to EFI ABI world
LDEAD_LOOP:
  jmp  LDEAD_LOOP       // _exit should never return



ASM_GLOBAL ASM_PFX(GasketSetTimer)
ASM_PFX(GasketSetTimer):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	ASM_PFX(SetTimer)

	popq  %rdi            // restore state
	popq  %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketGetLocalTime)
ASM_PFX(GasketGetLocalTime):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	ASM_PFX(GetLocalTime)

	popq  %rdi            // restore state
	popq  %rsi
  ret



ASM_GLOBAL ASM_PFX(Gasketgmtime)
ASM_PFX(Gasketgmtime):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	ASM_PFX(localtime)

	popq  %rdi            // restore state
	popq  %rsi
  ret



ASM_GLOBAL ASM_PFX(GasketGetTimeZone)
ASM_PFX(GasketGetTimeZone):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	ASM_PFX(GetTimeZone)

	popq  %rdi            // restore state
	popq  %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketGetDayLight)
ASM_PFX(GasketGetDayLight):
LFB26:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	ASM_PFX(GetDayLight)

	popq  %rdi            // restore state
	popq  %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketpoll)
ASM_PFX(Gasketpoll):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(poll)

	popq  %rdi            // restore state
	popq  %rsi
  ret



ASM_GLOBAL ASM_PFX(Gasketread)
ASM_PFX(Gasketread):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(read)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketwrite)
ASM_PFX(Gasketwrite):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(write)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketgetenv)
ASM_PFX(Gasketgetenv):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(getenv)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketopen)
ASM_PFX(Gasketopen):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(open)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketlseek)
ASM_PFX(Gasketlseek):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(lseek)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketftruncate)
ASM_PFX(Gasketftruncate):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(ftruncate)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketclose)
ASM_PFX(Gasketclose):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(close)

	popq    %rdi          // restore state
	popq    %rsi
  ret



ASM_GLOBAL ASM_PFX(Gasketmkdir)
ASM_PFX(Gasketmkdir):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(mkdir)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketunlink)
ASM_PFX(Gasketunlink):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(unlink)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketGetErrno)
ASM_PFX(GasketGetErrno):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	ASM_PFX(GetErrno)

	popq    %rdi          // restore state
	popq    %rsi
  ret

ASM_GLOBAL ASM_PFX(Gasketclosedir)
ASM_PFX(Gasketclosedir):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(closedir)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketrename)
ASM_PFX(Gasketrename):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(rename)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketmktime)
ASM_PFX(Gasketmktime):
LFB42:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(mktime)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketfsync)
ASM_PFX(Gasketfsync):
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(fsync)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketchmod)
ASM_PFX(Gasketchmod):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(chmod)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketutime)
ASM_PFX(Gasketutime):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(utime)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gaskettcflush)
ASM_PFX(Gaskettcflush):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(tcflush)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUgaCreate)
ASM_PFX(GasketUgaCreate):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(UgaCreate)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketperror)
ASM_PFX(Gasketperror):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(perror)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketioctl)
ASM_PFX(Gasketioctl):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(UnixIoCtl1)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gasketfcntl)
ASM_PFX(Gasketfcntl):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(UnixFcntl1)

	popq    %rdi          // restore state
	popq    %rsi
  ret



ASM_GLOBAL ASM_PFX(Gasketcfsetispeed)
ASM_PFX(Gasketcfsetispeed):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(cfsetispeed)

	popq    %rdi          // restore state
	popq    %rsi
  ret



ASM_GLOBAL ASM_PFX(Gasketcfsetospeed)
ASM_PFX(Gasketcfsetospeed):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(cfsetospeed)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gaskettcgetattr)
ASM_PFX(Gaskettcgetattr):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(tcgetattr)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(Gaskettcsetattr)
ASM_PFX(Gaskettcsetattr):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(tcsetattr)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUnixPeCoffGetEntryPoint)
ASM_PFX(GasketUnixPeCoffGetEntryPoint):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(SecPeCoffGetEntryPoint)

	popq    %rdi          // restore state
	popq    %rsi
  ret



ASM_GLOBAL ASM_PFX(GasketUnixPeCoffRelocateImageExtraAction)
ASM_PFX(GasketUnixPeCoffRelocateImageExtraAction):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(SecPeCoffRelocateImageExtraAction)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUnixPeCoffUnloadImageExtraAction)
ASM_PFX(GasketUnixPeCoffUnloadImageExtraAction):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(SecPeCoffLoaderUnloadImageExtraAction)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUgaClose)
ASM_PFX(GasketUgaClose):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(UgaClose)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUgaSize)
ASM_PFX(GasketUgaSize):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(UgaSize)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUgaCheckKey)
ASM_PFX(GasketUgaCheckKey):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args

	call	  ASM_PFX(UgaCheckKey)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUgaGetKey)
ASM_PFX(GasketUgaGetKey):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi

	call	  ASM_PFX(UgaGetKey)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketUgaBlt)
ASM_PFX(GasketUgaBlt):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  ASM_PFX(UgaBlt)

	popq    %rdi          // restore state
	popq    %rsi
  ret


//
// UNIX ABI to EFI ABI call
//
// UINTN
// ReverseGasketUint64 (
//   void *Api,
//   UINTN Arg1,
//   UINTN Arg2,
//   UINTN Arg3
//   );
ASM_GLOBAL ASM_PFX(ReverseGasketUint64)
ASM_PFX(ReverseGasketUint64):
  movq    %rdi, %rax    // Swizzle args
  movq    %rsi, %r9
//  movq    %rdx, %rdx
  movq    %rcx, %r8
  movq    %r9,  %rcx

	subq  $40, %rsp              // 32-byte shadow space plus alignment pad
	call	*%rax
  addq  $40, %rsp

	ret


// Sec PPI Callbacks

ASM_GLOBAL ASM_PFX(GasketSecUnixPeiLoadFile)
ASM_PFX(GasketSecUnixPeiLoadFile):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  ASM_PFX(SecUnixPeiLoadFile)

	popq    %rdi          // restore state
	popq    %rsi
  ret



ASM_GLOBAL ASM_PFX(GasketSecUnixPeiAutoScan)
ASM_PFX(GasketSecUnixPeiAutoScan):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx

	call	  ASM_PFX(SecUnixPeiAutoScan)

	popq    %rdi          // restore state
	popq    %rsi
  ret

ASM_GLOBAL ASM_PFX(GasketSecUnixUnixThunkAddress)
ASM_PFX(GasketSecUnixUnixThunkAddress):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	  ASM_PFX(SecUnixUnixThunkAddress)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketSecPeiReportStatusCode)
ASM_PFX(GasketSecPeiReportStatusCode):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx
  movq    %r9,  %rcx
  movq    $0,   %r8     // BugBug: This should come from the stack
  movq    $0,   %r9     //         But we can cheat since they are optional for bringup....

	call	  ASM_PFX(SecPeiReportStatusCode)

	popq    %rdi          // restore state
	popq    %rsi
  ret


ASM_GLOBAL ASM_PFX(GasketSecUnixFdAddress)
ASM_PFX(GasketSecUnixFdAddress):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  ASM_PFX(SecUnixFdAddress)

	popq    %rdi          // restore state
	popq    %rsi
  ret




ASM_GLOBAL ASM_PFX(GasketSecTemporaryRamSupport)
ASM_PFX(GasketSecTemporaryRamSupport):
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args
  movq    %rdx, %rsi
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  ASM_PFX(SecTemporaryRamSupport)

	popq    %rdi          // restore state
	popq    %rsi
  ret





