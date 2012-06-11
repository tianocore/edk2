/** @file
  Header file for Sec Core Debug Agent Library instance.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SEC_CORE_DEBUG_AGENT_LIB_H_
#define _SEC_CORE_DEBUG_AGENT_LIB_H_

#include <PiPei.h>

#include "DebugAgent.h"

typedef struct {
  VOID                    *Context;
  DEBUG_AGENT_CONTINUE    Function;
} DEBUG_AGENT_PHASE2_CONTEXT;

#endif

