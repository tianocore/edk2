/** @file
  IA32 specific defintions for debug agent library instance.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ARCH_DEBUG_SUPPORT_H_
#define _ARCH_DEBUG_SUPPORT_H_

#include "ArchRegisters.h"
#include "TransferProtocol.h"

typedef DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_IA32          DEBUG_DATA_REPONSE_READ_REGISTER_GROUP;
typedef DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM_IA32   DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGLIM;
typedef DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE_IA32  DEBUG_DATA_REPONSE_READ_REGISTER_GROUP_SEGBASE;

#define DEBUG_SW_BREAKPOINT_SYMBOL       0xcc

#define DEBUG_ARCH_SYMBOL                DEBUG_DATA_BREAK_CPU_ARCH_IA32

typedef DEBUG_DATA_IA32_SYSTEM_CONTEXT   DEBUG_CPU_CONTEXT;

#endif
