/** @file
  X64 specific defintions for debug agent library instance.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ARCH_DEBUG_SUPPORT_H_
#define _ARCH_DEBUG_SUPPORT_H_

#include "ProcessorContext.h"
#include "TransferProtocol.h"

#define DEBUG_SW_BREAKPOINT_SYMBOL       0xcc
#define DEBUG_ARCH_SYMBOL                DEBUG_DATA_BREAK_CPU_ARCH_X64

typedef DEBUG_DATA_X64_FX_SAVE_STATE     DEBUG_DATA_FX_SAVE_STATE;
typedef DEBUG_DATA_X64_SYSTEM_CONTEXT    DEBUG_CPU_CONTEXT;

#endif
