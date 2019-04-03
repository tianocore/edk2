/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  SmmStallLib.h

Abstract:

  This library provides SMM functions for Stall.
  These can be used to save size and simplify code.
  All contents must be runtime and SMM safe.

--*/

#ifndef _SMM_STALL_LIB_H_
#define _SMM_STALL_LIB_H_
#include "PiDxe.h"
#include "Pi/PiSmmCis.h"
extern EFI_SMM_SYSTEM_TABLE2  *mSmst;

/**
  Delay for at least the request number of microseconds

  @param[in] Microseconds  Number of microseconds to delay.

  @retval None

**/
VOID
SmmStall (
  IN  UINTN   Microseconds
  );

#endif
