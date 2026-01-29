/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _EFI_EDB_H_
#define _EFI_EDB_H_

#include "EdbCommon.h"

#define EBC_DEBUGGER_MAJOR_VERSION  1
#define EBC_DEBUGGER_MINOR_VERSION  0

#define EFI_DEBUG_RETURN    1
#define EFI_DEBUG_BREAK     2
#define EFI_DEBUG_CONTINUE  3

/**
  Driver Entry point.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

**/
EFI_STATUS
EfiDebuggerEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**

  The default Exception Callback for the VM interpreter.
  In this function, we report status code, and print debug information
  about EBC_CONTEXT, then dead loop.

  @param ExceptionType    Exception type.
  @param SystemContext    EBC system context.

**/
VOID
EFIAPI
EdbExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

extern EFI_DEBUGGER_PRIVATE_DATA  mDebuggerPrivate;

#include "EdbSupport.h"
#include "EdbCommand.h"
#include "EdbDisasm.h"
#include "EdbDisasmSupport.h"
#include "EdbSymbol.h"
#include "EdbHook.h"

#endif
