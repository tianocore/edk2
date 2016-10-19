/** @file
  Debug Agent Library provide source-level debug capability.

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUG_AGENT_LIB_H__
#define __DEBUG_AGENT_LIB_H__

#define DEBUG_AGENT_INIT_PREMEM_SEC              1
#define DEBUG_AGENT_INIT_POSTMEM_SEC             2
#define DEBUG_AGENT_INIT_DXE_CORE                3
#define DEBUG_AGENT_INIT_SMM                     4
#define DEBUG_AGENT_INIT_ENTER_SMI               5
#define DEBUG_AGENT_INIT_EXIT_SMI                6
#define DEBUG_AGENT_INIT_S3                      7
#define DEBUG_AGENT_INIT_DXE_AP                  8
#define DEBUG_AGENT_INIT_PEI                     9
#define DEBUG_AGENT_INIT_DXE_LOAD               10
#define DEBUG_AGENT_INIT_DXE_UNLOAD             11
#define DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64    12

//
// Context for DEBUG_AGENT_INIT_POSTMEM_SEC
//
typedef struct {
  UINTN          HeapMigrateOffset;
  UINTN          StackMigrateOffset;
} DEBUG_AGENT_CONTEXT_POSTMEM_SEC;

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Refer to the description for InitializeDebugAgent() for more details.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
typedef
VOID
(EFIAPI * DEBUG_AGENT_CONTINUE)(
  IN VOID        *Context
  );


/**
  Initialize debug agent.

  This function is used to set up debug environment to support source level debugging.
  If certain Debug Agent Library instance has to save some private data in the stack,
  this function must work on the mode that doesn't return to the caller, then
  the caller needs to wrap up all rest of logic after InitializeDebugAgent() into one
  function and pass it into InitializeDebugAgent(). InitializeDebugAgent() is
  responsible to invoke the passing-in function at the end of InitializeDebugAgent().

  If the parameter Function is not NULL, Debug Agent Library instance will invoke it by
  passing in the Context to be its parameter.

  If Function() is NULL, Debug Agent Library instance will return after setup debug
  environment.

  @param[in] InitFlag     Init flag is used to decide the initialize process.
  @param[in] Context      Context needed according to InitFlag; it was optional.
  @param[in] Function     Continue function called by debug agent library; it was
                          optional.

**/
VOID
EFIAPI
InitializeDebugAgent (
  IN UINT32                InitFlag,
  IN VOID                  *Context, OPTIONAL
  IN DEBUG_AGENT_CONTINUE  Function  OPTIONAL
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
