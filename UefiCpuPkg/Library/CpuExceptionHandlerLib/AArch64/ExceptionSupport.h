/** @file
  Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>
  Portion of Copyright (c) 2014 NVIDIA Corporation. All rights reserved.<BR>
  Copyright (c) 2016 HP Development Company, L.P.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/PcdLib.h>
#include <Protocol/DebugSupport.h> // for exception type definitions

VOID
RegisterEl0Stack (
  IN  VOID  *Stack
  );

VOID
CommonCExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContex
  );

VOID
ExceptionHandlersStart (
  VOID
  );
