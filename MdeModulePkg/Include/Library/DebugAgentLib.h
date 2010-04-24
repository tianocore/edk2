/** @file
  Debug Agent Library provide source-level debug capability.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUG_AGENT_LIB_H__
#define __DEBUG_AGENT_LIB_H__

#define DEBUG_AGENT_INIT_PREMEM_SEC      1
#define DEBUG_AGENT_INIT_POSTMEM_SEC     2
#define DEBUG_AGENT_INIT_DXE_CORE        3
#define DEBUG_AGENT_INIT_SMM             4
#define DEBUG_AGENT_INIT_ENTER_SMI       5
#define DEBUG_AGENT_INIT_EXIT_SMI        6
#define DEBUG_AGENT_INIT_S3              7

/**
  Initialize debug agent.

  This function is used to set up debug enviroment.

  @param[in] InitFlag   Init flag is used to decide the initialize process.
  @param[in] Context    Context needed according to InitFlag; it was optional.

**/
VOID
EFIAPI
InitializeDebugAgent (
  IN UINT32                InitFlag,
  IN VOID                  *Context  OPTIONAL
  );

/**
  Enable/Disable the interrupt of debug timer and return the interrupt state
  prior to the operation.

  If EnableStatus is TRUE, enable the interrupt of debug timer.
  If EnableStatus is FALSE, disable the interrupt of debug timer.

  @param[in] EnableStatus    Enable/Disable.

  @retval TRUE  Debug timer interrupt were enabled on entry to this call.
  @retval FALSE Debug timer interrupt were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndSetDebugTimerInterrupt (
  IN BOOLEAN                EnableStatus
  );

#endif
