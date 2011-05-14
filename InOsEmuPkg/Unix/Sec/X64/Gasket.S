#------------------------------------------------------------------------------
#
# Manage differenced between UNIX ABI and EFI/Windows ABI
#
# EFI  Arg passing: RCX, RDX, R8,  R9
#                   Callee allocates 32 bytes on stack to spill registers
# UNIX Arg passing: RDI, RSI, RDX, RCX, R8, R9
# RSI, RDI calle-save on EFI, scatch on UNIX callign
#
# Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
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



  .text
  
//
// EMU_THUNK_PROTOCOL gaskets (EFIAPI to UNIX ABI)
//




ASM_GLOBAL ASM_PFX(GasketSecWriteStdErr)
ASM_PFX(GasketSecWriteStdErr):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call  ASM_PFX(SecWriteStdErr)
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecSetTimer)
ASM_PFX(GasketSecSetTimer):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call  ASM_PFX(SecSetTimer)
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecEnableInterrupt)
ASM_PFX(GasketSecEnableInterrupt):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(SecEnableInterrupt)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecDisableInterrupt)
ASM_PFX(GasketSecDisableInterrupt):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(SecDisableInterrupt)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketQueryPerformanceFrequency)
ASM_PFX(GasketQueryPerformanceFrequency):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(QueryPerformanceFrequency)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketQueryPerformanceCounter)
ASM_PFX(GasketQueryPerformanceCounter):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(QueryPerformanceCounter)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecSleep)
ASM_PFX(GasketSecSleep):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq    %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq    %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call  ASM_PFX(SecSleep)
  
  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret
  
  
ASM_GLOBAL ASM_PFX(GasketSecExit)
ASM_PFX(GasketSecExit):
  pushq   %rbp                 // stack frame is for the debugger
  movq    %rsp, %rbp

  movq    %rcx, %rdi           // Swizzle args 
  call    ASM_PFX(SecExit)     // Less to do as we will never return to EFI ABI world
LDEAD_LOOP:
  jmp  LDEAD_LOOP              // _exit should never return

  
ASM_GLOBAL ASM_PFX(GasketSecGetTime)
ASM_PFX(GasketSecGetTime):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call  ASM_PFX(SecGetTime)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketSecSetTime)
ASM_PFX(GasketSecSetTime):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call  ASM_PFX(SecSetTime)

  popq  %rdi            // restore state
  popq  %rsi
  popq  %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketSecGetNextProtocol)
ASM_PFX(GasketSecGetNextProtocol):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(SecGetNextProtocol)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

// PPIs produced by SEC

ASM_GLOBAL ASM_PFX(GasketSecPeCoffGetEntryPoint)
ASM_PFX(GasketSecPeCoffGetEntryPoint):
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
 
ASM_GLOBAL ASM_PFX(GasketSecPeCoffRelocateImageExtraAction)
ASM_PFX(GasketSecPeCoffRelocateImageExtraAction):
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
 
ASM_GLOBAL ASM_PFX(GasketSecPeCoffUnloadImageExtraAction)
ASM_PFX(GasketSecPeCoffUnloadImageExtraAction):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(SecPeCoffUnloadImageExtraAction)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret
 

ASM_GLOBAL ASM_PFX(GasketSecEmuThunkAddress)
ASM_PFX(GasketSecEmuThunkAddress):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  call    ASM_PFX(SecEmuThunkAddress)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

//
// Gasket functions for EFI_EMU_UGA_IO_PROTOCOL
//

ASM_GLOBAL ASM_PFX(GasketX11Size)
ASM_PFX(GasketX11Size):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(X11Size)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketX11CheckKey)
ASM_PFX(GasketX11CheckKey):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(X11CheckKey)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketX11GetKey)
ASM_PFX(GasketX11GetKey):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(X11GetKey)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketX11KeySetState)
ASM_PFX(GasketX11KeySetState):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(X11KeySetState)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

  
ASM_GLOBAL ASM_PFX(GasketX11RegisterKeyNotify)
ASM_PFX(GasketX11RegisterKeyNotify):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(X11RegisterKeyNotify)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

  
ASM_GLOBAL ASM_PFX(GasketX11Blt)
ASM_PFX(GasketX11Blt):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(X11Blt)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketX11CheckPointer)
ASM_PFX(GasketX11CheckPointer):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(X11CheckPointer)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketX11GetPointerState)
ASM_PFX(GasketX11GetPointerState):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(X11GetPointerState)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketX11GraphicsWindowOpen)
ASM_PFX(GasketX11GraphicsWindowOpen):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(X11GraphicsWindowOpen)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

 
ASM_GLOBAL ASM_PFX(GasketX11GraphicsWindowClose)
ASM_PFX(GasketX11GraphicsWindowClose):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %r9,  %rcx

  call    ASM_PFX(X11GraphicsWindowClose)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


// Pthreads

ASM_GLOBAL ASM_PFX(GasketPthreadMutexLock)
ASM_PFX(GasketPthreadMutexLock):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadMutexLock)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadMutexUnLock)
ASM_PFX(GasketPthreadMutexUnLock):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadMutexUnLock)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketPthreadMutexTryLock)
ASM_PFX(GasketPthreadMutexTryLock):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadMutexTryLock)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

ASM_GLOBAL ASM_PFX(GasketPthreadMutexInit)
ASM_PFX(GasketPthreadMutexInit):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi


  call    ASM_PFX(PthreadMutexInit)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


 
ASM_GLOBAL ASM_PFX(GasketPthreadMutexDestroy)
ASM_PFX(GasketPthreadMutexDestroy):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadMutexDestroy)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadCreate)
ASM_PFX(GasketPthreadCreate):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(PthreadCreate)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

                          
ASM_GLOBAL ASM_PFX(GasketPthreadExit)
ASM_PFX(GasketPthreadExit):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadExit)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


                            
ASM_GLOBAL ASM_PFX(GasketPthreadSelf)
ASM_PFX(GasketPthreadSelf):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi


  call    ASM_PFX(PthreadSelf)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadOpen)
ASM_PFX(GasketPthreadOpen):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadOpen)
  
  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadClose)
ASM_PFX(GasketPthreadClose):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PthreadClose)
  
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


// EmuIoThunk SimpleFileSystem

ASM_GLOBAL ASM_PFX(GasketPosixOpenVolume)
ASM_PFX(GasketPosixOpenVolume):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(PosixOpenVolume)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileOpen)
ASM_PFX(GasketPosixFileOpen):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx
  movq    0x30(%rbp), %r8 
 
  call    ASM_PFX(PosixFileOpen)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileCLose)
ASM_PFX(GasketPosixFileCLose):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PosixFileCLose)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

  
ASM_GLOBAL ASM_PFX(GasketPosixFileDelete)
ASM_PFX(GasketPosixFileDelete):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PosixFileDelete)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileRead)
ASM_PFX(GasketPosixFileRead):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(PosixFileRead)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret

  
ASM_GLOBAL ASM_PFX(GasketPosixFileWrite)
ASM_PFX(GasketPosixFileWrite):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx

  call    ASM_PFX(PosixFileWrite)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSetPossition)
ASM_PFX(GasketPosixFileSetPossition):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(PosixFileSetPossition)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileGetPossition)
ASM_PFX(GasketPosixFileGetPossition):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     

  call    ASM_PFX(PosixFileGetPossition)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileGetInfo)
ASM_PFX(GasketPosixFileGetInfo):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(PosixFileGetInfo)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSetInfo)
ASM_PFX(GasketPosixFileSetInfo):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 
  movq    %rdx, %rsi     
  movq    %r8,  %rdx
  movq    %r9,  %rcx

  call    ASM_PFX(PosixFileSetInfo)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileFlush)
ASM_PFX(GasketPosixFileFlush):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PosixFileFlush)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSystmeThunkOpen)
ASM_PFX(GasketPosixFileSystmeThunkOpen):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PosixFileSystmeThunkOpen)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSystmeThunkClose)
ASM_PFX(GasketPosixFileSystmeThunkClose):
  pushq   %rbp            // stack frame is for the debugger
  movq    %rsp, %rbp

  pushq   %rsi          // %rsi & %rdi are volatile in Unix and callee-save in EFI ABI
  pushq   %rdi

  movq    %rcx, %rdi    // Swizzle args 

  call    ASM_PFX(PosixFileSystmeThunkClose)

  popq    %rdi          // restore state
  popq    %rsi
  popq    %rbp
  ret







