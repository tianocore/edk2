/** @file DxeExceptionLib.h

  Common header file for CPU Exception Handler Library.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef EXCEPTION_COMMON_H_
#define EXCEPTION_COMMON_H_

#define MAX_DEBUG_MESSAGE_LENGTH  0x100

//
// For coding convenience, define the maximum valid
// LoongArch exception.
// Since UEFI V2.11, it will be present in DebugSupport.h.
//
#define MAX_LOONGARCH_EXCEPTION  64

extern INTN  mExceptionKnownNameNum;

/**
  Get ASCII format string exception name by exception type.

  @param[in] ExceptionType  Exception type.

  @return    ASCII format string exception name.

**/
CONST CHAR8 *
GetExceptionNameStr (
  IN EFI_EXCEPTION_TYPE  ExceptionType
  );

/**
  Prints a message to the serial port.

  @param[in]  Format      Format string for the message to print.
  @param[in]  ...         Variable argument list whose contents are accessed
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

  @param[in] CurrentEip      Current instruction pointer.

**/
VOID
DumpModuleImageInfo (
  IN UINTN  CurrentEip
  );

/**
  IPI Interrupt Handler.

  @param InterruptType    The type of interrupt that occurred
  @param SystemContext    A pointer to the system context when the interrupt occurred
**/
VOID
EFIAPI
IpiInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Default exception handler.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Display CPU information.

  @param[in] ExceptionType  Exception type.
  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
DumpImageAndCpuContent (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Get exception types

  @param[in]  SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

  @return     Exception type.

**/
EFI_EXCEPTION_TYPE
EFIAPI
GetExceptionType (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Get Common interrupt types

  @param[in] SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

  @return    Interrupt type.

**/
EFI_EXCEPTION_TYPE
EFIAPI
GetInterruptType (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif
