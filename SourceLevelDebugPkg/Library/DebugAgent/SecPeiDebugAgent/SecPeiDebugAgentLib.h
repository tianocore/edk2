/** @file
  Header file for Sec Core Debug Agent Library instance.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SEC_CORE_DEBUG_AGENT_LIB_H_
#define _SEC_CORE_DEBUG_AGENT_LIB_H_

#include <PiPei.h>
#include <Ppi/MemoryDiscovered.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include "DebugAgent.h"

typedef struct {
  UINT32                  InitFlag;
  VOID                    *Context;
  DEBUG_AGENT_CONTINUE    Function;
} DEBUG_AGENT_PHASE2_CONTEXT;

/**
  Caller provided function to be invoked at the end of DebugPortInitialize().

  Refer to the description for DebugPortInitialize() for more details.

  @param[in] Context           The first input argument of DebugPortInitialize().
  @param[in] DebugPortHandle   Debug port handle created by Debug Communication Library.

**/
VOID
EFIAPI
InitializeDebugAgentPhase2 (
  IN VOID               *Context,
  IN DEBUG_PORT_HANDLE  DebugPortHandle
  );

/**
  Debug Agent provided notify callback function on Memory Discovered PPI.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @retval EFI_SUCCESS If the function completed successfully.

**/
EFI_STATUS
EFIAPI
DebugAgentCallbackMemoryDiscoveredPpi (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

#endif
