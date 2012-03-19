/** @file
  Common header file for CPU Exception Handler Library.

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_EXCEPTION_COMMON_H_
#define _CPU_EXCEPTION_COMMON_H_

#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PrintLib.h>
#include <Library/LocalApicLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#define  CPU_EXCEPTION_NUM     32
//
// Record exception handler information
//
typedef struct {
  UINTN ExceptionStart;
  UINTN ExceptionStubHeaderSize;
} EXCEPTION_HANDLER_TEMPLATE_MAP;

extern UINT32           mErrorCodeFlag;
extern CONST UINTN      mImageAlignSize;

/**
  Return address map of exception handler template so that C code can generate
  exception tables.

  @param AddressMap  Pointer to a buffer where the address map is returned.
**/
VOID
EFIAPI
GetTemplateAddressMap (
  OUT EXCEPTION_HANDLER_TEMPLATE_MAP *AddressMap
  );

/**
  Internal function to setup CPU exception handlers.

**/
VOID
InternalSetupCpuExceptionHandlers (
  VOID
  );

/**
  Prints a message to the serial port.

  @param  Format      Format string for the message to print.
  @param  ...         Variable argument list whose contents are accessed 
                      based on the format string specified by Format.

**/
VOID
EFIAPI
InternalPrintMessage (
  IN  CONST CHAR8  *Format,
  ...
  );

/**
  Find and display image base address and return image base and its entry point.
  
  @param CurrentEip      Current instruction pointer.
  @param EntryPoint      Return module entry point if module header is found.
  
  @return !0     Image base address.
  @return 0      Image header cannot be found.
**/
UINTN 
FindModuleImageBase (
  IN  UINTN              CurrentEip,
  OUT UINTN              *EntryPoint
  );

/**
  Display CPU information.

  @param InterruptType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
DumpCpuContent (
  IN UINTN                InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  );

#endif
