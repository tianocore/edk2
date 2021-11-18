/** @file
  IA32 specific definitions for debug agent library instance.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ARCH_DEBUG_SUPPORT_H_
#define _ARCH_DEBUG_SUPPORT_H_

#include "ProcessorContext.h"
#include "TransferProtocol.h"

#define DEBUG_SW_BREAKPOINT_SYMBOL  0xcc
#define DEBUG_ARCH_SYMBOL           DEBUG_DATA_BREAK_CPU_ARCH_IA32

typedef DEBUG_DATA_IA32_FX_SAVE_STATE   DEBUG_DATA_FX_SAVE_STATE;
typedef DEBUG_DATA_IA32_SYSTEM_CONTEXT  DEBUG_CPU_CONTEXT;

#endif
