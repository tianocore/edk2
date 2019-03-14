/** @file
  Header file of Debug services instances.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __DEBUG_SERVICE_H__
#define __DEBUG_SERVICE_H__

#include <Ppi/Debug.h>

/**
  Print a debug message to debug output device if the specified error level
  is enabled.

  @param[in] ErrorLevel               The error level of the debug message.
  @param[in] Format                   Format string for the debug message to print.
  @param[in] Marker                   BASE_LIST marker for the variable argument list.

**/
VOID
EFIAPI
PeiDebugBPrint(
  IN UINTN                          ErrorLevel,
  IN CONST CHAR8                    *Format,
  IN BASE_LIST                      Marker
  );

/**
  Prints an assert message containing a filename, line number, and description.
  This may be followed by a breakpoint or a dead loop.

  @param[in] FileName                 The pointer to the name of the source file that
                                      generated the assert condition.
  @param[in] LineNumber               The line number in the source file that generated
                                      the assert condition
  @param[in] Description              The pointer to the description of the assert condition.

**/
VOID
EFIAPI
PeiDebugAssert(
  IN CONST CHAR8                    *FileName,
  IN UINTN                          LineNumber,
  IN CONST CHAR8                    *Description
  );

#endif
