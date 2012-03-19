/** @file
  CPU Exception Library provides DXE/SMM CPU exception handler.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include "CpuExceptionCommon.h"
#include <Library/SynchronizationLib.h>

//
// Spinlock for CPU information display
//
SPIN_LOCK        mDisplayMessageSpinLock;

//
// Image align size for DXE/SMM
//
CONST UINTN      mImageAlignSize = SIZE_4KB;

/**
  Common exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
CommonExceptionHandler (
  IN EFI_EXCEPTION_TYPE   ExceptionType, 
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  //
  // Get Spinlock to display CPU information
  //
  while (!AcquireSpinLockOrFail (&mDisplayMessageSpinLock)) {
    CpuPause ();
  }

  //
  // Display ExceptionType, CPU information and Image information
  //
  DumpCpuContent (ExceptionType, SystemContext);

  //
  // Release Spinlock
  //
  ReleaseSpinLock (&mDisplayMessageSpinLock);

  //
  // Enter a dead loop.
  //
  CpuDeadLoop ();
}

/**
  Setup CPU exception handlers.

  This API will setups the CPU exception handler to display CPU contents and run into
  CpuDeadLoop(). 
  Note: Before invoking this API, caller must allocate memory for IDT table and load 
        IDTR by AsmWriteIdtr().
  
**/
VOID
EFIAPI
SetupCpuExceptionHandlers (
  IN VOID
  )
{
  InitializeSpinLock (&mDisplayMessageSpinLock);
  InternalSetupCpuExceptionHandlers ();
}

