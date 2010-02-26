/** @file
  Debug Agent Library provide source-level debug capability.

  Copyright (c) 2010, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUG_AGENT_LIB_H__
#define __DEBUG_AGENT_LIB_H__

#define DEBUG_AGENT_INIT_PREMEM_SEC      1
#define DEBUG_AGENT_INIT_POSTMEM_SEC     2
#define DEBUG_AGENT_INIT_DXE             3
#define DEBUG_AGENT_INIT_SMM             4
#define DEBUG_AGENT_INIT_SMI             5

/**
  Initialize debug agent.

  This function is used to set up debug enviroment.

  @param[in] InitFlag   Init flag is used to decide initialize process.
  @param[in] Context    Context needed according to InitFlag, it was optional.

**/
VOID
EFIAPI
InitializeDebugAgent (
  IN UINT32                InitFlag,
  IN VOID                  *Context  OPTIONAL
  );

/**
  Enable/Disable the interrupt of debug timer.

  If EnableStatus is TRUE, enable the interrupt of debug timer.
  If EnableStatus is FALSE, disable the interrupt of debug timer.

  @param[in] EnableStatus    Enable/Disable.

**/
VOID
EFIAPI
SetDebugTimerInterrupt (
  IN BOOLEAN                EnableStatus
  );

#endif
