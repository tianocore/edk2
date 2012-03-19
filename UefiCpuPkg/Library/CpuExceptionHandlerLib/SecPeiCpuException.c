/** @file
  CPU Exception Library provides SEC/PEIM CPU exception handler.

Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include "CpuExceptionCommon.h"

//
// Image Aglinment size for SEC/PEI phase
//
CONST UINTN      mImageAlignSize = 4;

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
  // Display ExceptionType, CPU information and Image information
  //  
  DumpCpuContent (ExceptionType, SystemContext);
  
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
  InternalSetupCpuExceptionHandlers ();
}
