/** @file

 Exception handling assembly function declarations.

 Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>
 Portion of Copyright (c) 2014 NVIDIA Corporation. All rights reserved.<BR>
 Copyright (c) 2016 HP Development Company, L.P.
 Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>

 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Base.h>

/**
  Register the EL0 stack pointer for exception handling.

  @param[in]  Stack   Pointer to the top of the EL0 stack.
**/
VOID
RegisterEl0Stack (
  IN  VOID  *Stack
  );

/**
  Entry point of the exception handler assembly code.
**/
VOID
ExceptionHandlersStart (
  VOID
  );
