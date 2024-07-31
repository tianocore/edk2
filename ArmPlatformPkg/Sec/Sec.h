/** @file
  Generic SEC driver for ARM platforms

  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SEC_H_
#define SEC_H_

#include <PiPei.h>

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#include <Ppi/TemporaryRamSupport.h>

/**
  Helper function to switch to a different stack. Implemented in assembler as
  this cannot be done from C code.
**/
VOID
SecSwitchStack (
  INTN  StackDelta
  );

/**
  Vector Table for the PEI Phase. This is executable code but not a callable
  function. Implemented in assembler.
**/
VOID
PeiVectorTable (
  VOID
  );

/**
  Minimal high level handling of exceptions occurring in PEI.
**/
VOID
PeiCommonExceptionEntry (
  IN UINT32  Entry,
  IN UINTN   LR
  );

#endif
