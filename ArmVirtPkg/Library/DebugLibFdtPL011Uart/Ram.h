/** @file
  Declare the variables that modules that can only run from RAM use for
  remembering initialization status.

  Copyright (C) Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DEBUG_LIB_FDT_PL011_UART_RAM_H_
#define DEBUG_LIB_FDT_PL011_UART_RAM_H_

#include <Base.h>

extern UINTN          mDebugLibFdtPL011UartAddress;
extern RETURN_STATUS  mDebugLibFdtPL011UartPermanentStatus;

#endif
