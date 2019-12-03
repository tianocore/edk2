#------------------------------------------------------------------------------
#
# Manage differenced between UNIX ABI and EFI/Windows ABI
#
# For IA-32 the only difference is Mac OS X requires a 16-byte aligned stack.
# For Linux this stack adjustment is a no-op, but we may as well make the
# the code common.
#
# Copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------



  .text

//
// EMU_THUNK_PROTOCOL gaskets (EFIAPI to UNIX ABI)
//


ASM_GLOBAL ASM_PFX(GasketSecWriteStdErr)
ASM_PFX(GasketSecWriteStdErr):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecWriteStdErr)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecConfigStdIn)
ASM_PFX(GasketSecConfigStdIn):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecConfigStdIn)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecWriteStdOut)
ASM_PFX(GasketSecWriteStdOut):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecWriteStdOut)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecReadStdIn)
ASM_PFX(GasketSecReadStdIn):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecReadStdIn)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecPollStdIn)
ASM_PFX(GasketSecPollStdIn):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecPollStdIn)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecMalloc)
ASM_PFX(GasketSecMalloc):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecMalloc)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecValloc)
ASM_PFX(GasketSecValloc):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecValloc)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecFree)
ASM_PFX(GasketSecFree):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecFree)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecSetTimer)
ASM_PFX(GasketSecSetTimer):
  pushl %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  8(%ebp), %eax
  movl  12(%ebp), %edx
  movl  %edx, 4(%esp)
  movl  %eax, (%esp)

  call  ASM_PFX(SecSetTimer)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecEnableInterrupt)
ASM_PFX(GasketSecEnableInterrupt):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(SecEnableInterrupt)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecDisableInterrupt)
ASM_PFX(GasketSecDisableInterrupt):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(SecDisableInterrupt)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketQueryPerformanceFrequency)
ASM_PFX(GasketQueryPerformanceFrequency):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(QueryPerformanceFrequency)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketQueryPerformanceCounter)
ASM_PFX(GasketQueryPerformanceCounter):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(QueryPerformanceCounter)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecSleep)
ASM_PFX(GasketSecSleep):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  12(%ebp), %ecx
  movl  %ecx, 4(%esp)
  movl  %eax, (%esp)

  call  ASM_PFX(SecSleep)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecCpuSleep)
ASM_PFX(GasketSecCpuSleep):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call  ASM_PFX(SecCpuSleep)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecExit)
ASM_PFX(GasketSecExit):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecExit)     // Less to do as we will never return to EFI ABI world
LDEAD_LOOP:
  jmp  LDEAD_LOOP              // _exit should never return


ASM_GLOBAL ASM_PFX(GasketSecGetTime)
ASM_PFX(GasketSecGetTime):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecGetTime)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecSetTime)
ASM_PFX(GasketSecSetTime):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call  ASM_PFX(SecSetTime)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecGetNextProtocol)
ASM_PFX(GasketSecGetNextProtocol):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecGetNextProtocol)

  leave
  ret

// PPIs produced by SEC

ASM_GLOBAL ASM_PFX(GasketSecPeCoffGetEntryPoint)
ASM_PFX(GasketSecPeCoffGetEntryPoint):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecPeCoffGetEntryPoint)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecPeCoffRelocateImageExtraAction)
ASM_PFX(GasketSecPeCoffRelocateImageExtraAction):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecPeCoffRelocateImageExtraAction)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketSecPeCoffUnloadImageExtraAction)
ASM_PFX(GasketSecPeCoffUnloadImageExtraAction):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecPeCoffUnloadImageExtraAction)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecEmuThunkAddress)
ASM_PFX(GasketSecEmuThunkAddress):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(SecEmuThunkAddress)

  leave
  ret

//
// Gasket functions for EFI_EMU_UGA_IO_PROTOCOL
//

ASM_GLOBAL ASM_PFX(GasketX11Size)
ASM_PFX(GasketX11Size):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11Size)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11CheckKey)
ASM_PFX(GasketX11CheckKey):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11CheckKey)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketX11GetKey)
ASM_PFX(GasketX11GetKey):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11GetKey)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11KeySetState)
ASM_PFX(GasketX11KeySetState):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11KeySetState)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11RegisterKeyNotify)
ASM_PFX(GasketX11RegisterKeyNotify):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11RegisterKeyNotify)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11Blt)
ASM_PFX(GasketX11Blt):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11Blt)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11CheckPointer)
ASM_PFX(GasketX11CheckPointer):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11CheckPointer)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11GetPointerState)
ASM_PFX(GasketX11GetPointerState):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11GetPointerState)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11GraphicsWindowOpen)
ASM_PFX(GasketX11GraphicsWindowOpen):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11GraphicsWindowOpen)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketX11GraphicsWindowClose)
ASM_PFX(GasketX11GraphicsWindowClose):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(X11GraphicsWindowClose)

  leave
  ret


// Pthreads

ASM_GLOBAL ASM_PFX(GasketPthreadMutexLock)
ASM_PFX(GasketPthreadMutexLock):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadMutexLock)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadMutexUnLock)
ASM_PFX(GasketPthreadMutexUnLock):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadMutexUnLock)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketPthreadMutexTryLock)
ASM_PFX(GasketPthreadMutexTryLock):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadMutexTryLock)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketPthreadMutexInit)
ASM_PFX(GasketPthreadMutexInit):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(PthreadMutexInit)

  leave
  ret



ASM_GLOBAL ASM_PFX(GasketPthreadMutexDestroy)
ASM_PFX(GasketPthreadMutexDestroy):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadMutexDestroy)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadCreate)
ASM_PFX(GasketPthreadCreate):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadCreate)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadExit)
ASM_PFX(GasketPthreadExit):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadExit)

  leave
  ret



ASM_GLOBAL ASM_PFX(GasketPthreadSelf)
ASM_PFX(GasketPthreadSelf):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call

  call    ASM_PFX(PthreadSelf)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadOpen)
ASM_PFX(GasketPthreadOpen):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadOpen)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPthreadClose)
ASM_PFX(GasketPthreadClose):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PthreadClose)

  leave
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
  pushl  %ebp
  movl  %esp, %ebp
  subl  $8, %esp
  movl  16(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  calll  *8(%ebp)
  addl  $8, %esp
  popl  %ebp
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
  pushl  %ebp
  movl  %esp, %ebp
  subl  $24, %esp
  movl  24(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  12(%ebp), %eax
  movl  %eax, (%esp)
  calll  *8(%ebp)
  addl  $24, %esp
  popl  %ebp
  ret


ASM_GLOBAL ASM_PFX(GasketSecUnixPeiAutoScan)
ASM_PFX(GasketSecUnixPeiAutoScan):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecUnixPeiAutoScan)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSecUnixFdAddress)
ASM_PFX(GasketSecUnixFdAddress):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(SecUnixFdAddress)

  leave
  ret


// EmuIoThunk SimpleFileSystem

ASM_GLOBAL ASM_PFX(GasketPosixOpenVolume)
ASM_PFX(GasketPosixOpenVolume):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixOpenVolume)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileOpen)
ASM_PFX(GasketPosixFileOpen):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $56, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  28(%ebp), %eax
  movl  32(%ebp), %ecx
  movl  %ecx, 24(%esp)
  movl  %eax, 20(%esp)
  movl  20(%ebp), %eax
  movl  24(%ebp), %ecx
  movl  %ecx, 16(%esp)
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileOpen)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileCLose)
ASM_PFX(GasketPosixFileCLose):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileCLose)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileDelete)
ASM_PFX(GasketPosixFileDelete):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileDelete)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileRead)
ASM_PFX(GasketPosixFileRead):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileRead)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileWrite)
ASM_PFX(GasketPosixFileWrite):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileWrite)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSetPossition)
ASM_PFX(GasketPosixFileSetPossition):
  pushl %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  16(%ebp), %ecx
  movl  %ecx, 8(%esp)
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileSetPossition)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileGetPossition)
ASM_PFX(GasketPosixFileGetPossition):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileGetPossition)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileGetInfo)
ASM_PFX(GasketPosixFileGetInfo):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileGetInfo)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSetInfo)
ASM_PFX(GasketPosixFileSetInfo):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileSetInfo)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileFlush)
ASM_PFX(GasketPosixFileFlush):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileFlush)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSystmeThunkOpen)
ASM_PFX(GasketPosixFileSystmeThunkOpen):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileSystmeThunkOpen)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketPosixFileSystmeThunkClose)
ASM_PFX(GasketPosixFileSystmeThunkClose):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(PosixFileSystmeThunkClose)

  leave
  ret

ASM_GLOBAL ASM_PFX(GasketEmuBlockIoReset)
ASM_PFX(GasketEmuBlockIoReset):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuBlockIoReset)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketEmuBlockIoReadBlocks)
ASM_PFX(GasketEmuBlockIoReadBlocks):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $56, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  32(%ebp), %eax
  movl  %eax, 24(%esp)
  movl  28(%ebp), %eax
  movl  %eax, 20(%esp)
  movl  24(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  16(%ebp), %eax
  movl  20(%ebp), %edx
  movl  %edx, 12(%esp)
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuBlockIoReadBlocks)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketEmuBlockIoWriteBlocks)
ASM_PFX(GasketEmuBlockIoWriteBlocks):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $56, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  32(%ebp), %eax
  movl  %eax, 24(%esp)
  movl  28(%ebp), %eax
  movl  %eax, 20(%esp)
  movl  24(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  16(%ebp), %eax
  movl  20(%ebp), %edx
  movl  %edx, 12(%esp)
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuBlockIoWriteBlocks)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketEmuBlockIoFlushBlocks)
ASM_PFX(GasketEmuBlockIoFlushBlocks):  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)


  call    ASM_PFX(EmuBlockIoFlushBlocks)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketEmuBlockIoCreateMapping)
ASM_PFX(GasketEmuBlockIoCreateMapping):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuBlockIoCreateMapping)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketBlockIoThunkOpen)
ASM_PFX(GasketBlockIoThunkOpen):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuBlockIoThunkOpen)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketBlockIoThunkClose)
ASM_PFX(GasketBlockIoThunkClose):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuBlockIoThunkClose)

  leave
  ret



ASM_GLOBAL ASM_PFX(GasketSnpCreateMapping)
ASM_PFX(GasketSnpCreateMapping):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpCreateMapping)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpStart)
ASM_PFX(GasketSnpStart):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpStart)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpStop)
ASM_PFX(GasketSnpStop):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpStop)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpInitialize)
ASM_PFX(GasketSnpInitialize):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpInitialize)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpReset)
ASM_PFX(GasketSnpReset):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpReset)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpShutdown)
ASM_PFX(GasketSnpShutdown):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpShutdown)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpReceiveFilters)
ASM_PFX(GasketSnpReceiveFilters):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  28(%ebp), %eax
  movl  %eax, 20(%esp)
  movl  24(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpReceiveFilters)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpStationAddress)
ASM_PFX(GasketSnpStationAddress):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  leave
  ret



ASM_GLOBAL ASM_PFX(GasketSnpStatistics)
ASM_PFX(GasketSnpStatistics):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpStatistics)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpMCastIpToMac)
ASM_PFX(GasketSnpMCastIpToMac):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpMCastIpToMac)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpNvData)
ASM_PFX(GasketSnpNvData):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  24(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpNvData)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpGetStatus)
ASM_PFX(GasketSnpGetStatus):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $40, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpGetStatus)

  leave
  ret



ASM_GLOBAL ASM_PFX(GasketSnpTransmit)
ASM_PFX(GasketSnpTransmit):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $56, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  32(%ebp), %eax
  movl  %eax, 24(%esp)
  movl  28(%ebp), %eax
  movl  %eax, 20(%esp)
  movl  24(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpTransmit)

  leave
  ret



ASM_GLOBAL ASM_PFX(GasketSnpReceive)
ASM_PFX(GasketSnpReceive):
  pushl  %ebp
  movl  %esp, %ebp
  subl  $56, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  32(%ebp), %eax
  movl  %eax, 24(%esp)
  movl  28(%ebp), %eax
  movl  %eax, 20(%esp)
  movl  24(%ebp), %eax
  movl  %eax, 16(%esp)
  movl  20(%ebp), %eax
  movl  %eax, 12(%esp)
  movl  16(%ebp), %eax
  movl  %eax, 8(%esp)
  movl  12(%ebp), %eax
  movl  %eax, 4(%esp)
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpReceive)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpThunkOpen)
ASM_PFX(GasketSnpThunkOpen):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpThunkOpen)

  leave
  ret


ASM_GLOBAL ASM_PFX(GasketSnpThunkClose)
ASM_PFX(GasketSnpThunkClose):
  pushl %ebp
  movl  %esp, %ebp
  subl  $24, %esp      // sub extra 16 from the stack for alignment
  and   $-16, %esp    // stack needs to end in 0xFFFFFFF0 before call
  movl  8(%ebp), %eax
  movl  %eax, (%esp)

  call    ASM_PFX(EmuSnpThunkClose)

  leave
  ret


