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


.globl _Gasketrmdir
_Gasketrmdir:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	movq	_gUnixRmDir@GOTPCREL(%rip), %rax  // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax
    
	pushq	  %rdi          // restore state
	pushq	  %rsi
	ret


.globl _Gasketopendir
_Gasketopendir:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	movq	_gUnixOpenDir@GOTPCREL(%rip), %rax  // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax
	
	popq  %rdi             // restore state
	popq  %rsi
  ret



.globl _Gasketstat
_Gasketstat:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	movq	_gUnixStat@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax
	
	popq  %rdi            // restore state
	popq  %rsi
  ret
	
	
.globl _Gasketstatfs
_Gasketstatfs:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	movq	_gUnixStatFs@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax
	
	popq  %rdi            // restore state
	popq  %rsi
  ret
	
.globl _Gasketrewinddir
_Gasketrewinddir:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	movq	_gUnixRewinddir@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax
	
	popq  %rdi            // restore state
	popq  %rsi
  ret
	
.globl _Gasketreaddir
_Gasketreaddir:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	movq	_gUnixReaddir@GOTPCREL(%rip), %rax   // Get function name mangled by C
	movq	(%rax), %rax
	call	*%rax
	
	popq  %rdi            // restore state
	popq  %rsi
  ret
	

.globl _GasketmsSleep
_GasketmsSleep:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	_msSleep
	
	popq  %rdi            // restore state
	popq  %rsi
  ret
	
	
.globl _Gasketexit
_Gasketexit:
  movq    %rcx, %rdi    // Swizzle args 
	call	_exit           // Less to do as we will never return to EFI ABI world
LDEAD_LOOP:
  jmp  LDEAD_LOOP       // _exit should never return



.globl _GasketSetTimer
_GasketSetTimer:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	_SetTimer
	
	popq  %rdi            // restore state
	popq  %rsi
  ret

	
.globl _GasketGetLocalTime
_GasketGetLocalTime:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	_GetLocalTime
	
	popq  %rdi            // restore state
	popq  %rsi
  ret

	

.globl _Gasketgmtime
_Gasketgmtime:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	_localtime
	
	popq  %rdi            // restore state
	popq  %rsi
  ret



.globl _GasketGetTimeZone
_GasketGetTimeZone:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	_GetTimeZone
	
	popq  %rdi            // restore state
	popq  %rsi
  ret

	
.globl _GasketGetDayLight
_GasketGetDayLight:
LFB26:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	_GetDayLight
	
	popq  %rdi            // restore state
	popq  %rsi
  ret


.globl _Gasketpoll
_Gasketpoll:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _poll
	
	popq  %rdi            // restore state
	popq  %rsi
  ret



.globl _Gasketread
_Gasketread:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _read
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketwrite
_Gasketwrite:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _write
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketgetenv
_Gasketgetenv:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _getenv
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketopen
_Gasketopen:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _open
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketlseek
_Gasketlseek:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _lseek
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketftruncate
_Gasketftruncate:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _ftruncate
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketclose
_Gasketclose:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _close
	
	popq    %rdi          // restore state
	popq    %rsi
  ret



.globl _Gasketmkdir
_Gasketmkdir:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _mkdir
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketunlink
_Gasketunlink:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _unlink
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketGetErrno
_GasketGetErrno:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	_GetErrno
	
	popq    %rdi          // restore state
	popq    %rsi
  ret

.globl _Gasketclosedir
_Gasketclosedir:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _closedir
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketrename
_Gasketrename:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _rename
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketmktime
_Gasketmktime:
LFB42:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _mktime
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketfsync
_Gasketfsync:
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _fsync
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketchmod
_Gasketchmod:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _chmod
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketutime
_Gasketutime:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _utime
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gaskettcflush
_Gaskettcflush:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _tcflush
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUgaCreate
_GasketUgaCreate:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _UgaCreate
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketperror
_Gasketperror:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _perror
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketioctl
_Gasketioctl:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _UnixIoCtl1
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gasketfcntl
_Gasketfcntl:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _UnixFcntl1
	
	popq    %rdi          // restore state
	popq    %rsi
  ret



.globl _Gasketcfsetispeed
_Gasketcfsetispeed:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _cfsetispeed
	
	popq    %rdi          // restore state
	popq    %rsi
  ret



.globl _Gasketcfsetospeed
_Gasketcfsetospeed:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _cfsetospeed
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gaskettcgetattr
_Gaskettcgetattr:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _tcgetattr
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _Gaskettcsetattr
_Gaskettcsetattr:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _tcsetattr
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUnixPeCoffGetEntryPoint
_GasketUnixPeCoffGetEntryPoint:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _SecPeCoffGetEntryPoint
	
	popq    %rdi          // restore state
	popq    %rsi
  ret



.globl _GasketUnixPeCoffRelocateImageExtraAction
_GasketUnixPeCoffRelocateImageExtraAction:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _SecPeCoffRelocateImageExtraAction
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUnixPeCoffUnloadImageExtraAction
_GasketUnixPeCoffUnloadImageExtraAction:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _SecPeCoffLoaderUnloadImageExtraAction
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUgaClose
_GasketUgaClose:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _UgaClose
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUgaSize
_GasketUgaSize:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _UgaSize
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUgaCheckKey
_GasketUgaCheckKey:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 

	call	  _UgaCheckKey
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUgaGetKey
_GasketUgaGetKey:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

	call	  _UgaGetKey
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketUgaBlt
_GasketUgaBlt:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  _UgaBlt
	
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
.globl _ReverseGasketUint64
_ReverseGasketUint64:
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

.globl _GasketSecUnixPeiLoadFile
_GasketSecUnixPeiLoadFile:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  _SecUnixPeiLoadFile
	
	popq    %rdi          // restore state
	popq    %rsi
  ret



.globl _GasketSecUnixPeiAutoScan
_GasketSecUnixPeiAutoScan:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

	call	  _SecUnixPeiAutoScan
	
	popq    %rdi          // restore state
	popq    %rsi
  ret

.globl _GasketSecUnixUnixThunkAddress
_GasketSecUnixUnixThunkAddress:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

	call	  _SecUnixUnixThunkAddress
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketSecPeiReportStatusCode
_GasketSecPeiReportStatusCode:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx
  movq    $0,   %r8     // BugBug: This should come from the stack
  movq    $0,   %r9     //         But we can cheat since they are optional for bringup....

	call	  _SecPeiReportStatusCode
	
	popq    %rdi          // restore state
	popq    %rsi
  ret


.globl _GasketSecUnixFdAddress
_GasketSecUnixFdAddress:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  _SecUnixFdAddress
	
	popq    %rdi          // restore state
	popq    %rsi
  ret




.globl _GasketSecTemporaryRamSupport
_GasketSecTemporaryRamSupport:
	pushq	  %rsi          // %rsi & %rdi are volatie in Unix and callee-save in EFI ABI
	pushq	  %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

	call	  _SecTemporaryRamSupport
	
	popq    %rdi          // restore state
	popq    %rsi
  ret





