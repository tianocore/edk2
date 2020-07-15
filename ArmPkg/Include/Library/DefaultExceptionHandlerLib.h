/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEFAULT_EXCEPTION_HANDLER_LIB_H__
#define __DEFAULT_EXCEPTION_HANDLER_LIB_H__

/**
  This is the default action to take on an unexpected exception

  @param  ExceptionType    Type of the exception
  @param  SystemContext    Register state at the time of the Exception

**/
VOID
DefaultExceptionHandler (
  IN     EFI_EXCEPTION_TYPE           ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  );

#endif
