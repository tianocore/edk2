/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EXCEPTION_LIB_H__
#define __EXCEPTION_LIB_H__

#include <Uefi.h>
#include <Protocol/DebugSupport.h> // for EFI_EXCEPTION_CALLBACK definition only
#include <Protocol/Cpu.h> // for EFI_CPU_INTERRUPT_HANDLER definition only

/**
This function registers and enables the handler specified by ExceptionHandler for a processor
exception type specified by ExceptionType. If ExceptionHandler is NULL, then the
handler for the processor exception type specified by ExceptionType is uninstalled.
The installed handler is called once for each processor exception.

@param  ExceptionType    The exception type for which to register/deregister
@param  ExceptionHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
when a processor exception occurs. If this parameter is NULL, then the handler will be uninstalled.

@retval RETURN_SUCCESS           The handler for the processor exception was successfully installed or uninstalled.
@retval RETURN_ALREADY_STARTED   ExceptionHandler is not NULL, and a handler for ExceptionType was
previously installed.
@retval RETURN_INVALID_PARAMETER ExceptionHandler is NULL, and a handler for ExceptionType was not
previously installed.
@retval RETURN_UNSUPPORTED       The exception specified by ExceptionType is not supported.

**/
RETURN_STATUS
RegisterExceptionHandler(
  IN EFI_EXCEPTION_TYPE             ExceptionType,
  IN EFI_CPU_INTERRUPT_HANDLER      ExceptionHandler
  );

#endif // __EXCEPTION_LIB_H__
