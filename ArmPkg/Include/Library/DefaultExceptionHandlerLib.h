/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
