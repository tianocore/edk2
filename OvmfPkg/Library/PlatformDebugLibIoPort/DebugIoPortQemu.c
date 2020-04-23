/** @file
  Detection code for QEMU debug port.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2012, Red Hat, Inc.<BR>
  Copyright (c) 2020, Citrix Systems, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include "DebugLibDetect.h"

//
// The constant value that is read from the debug I/O port
//
#define BOCHS_DEBUG_PORT_MAGIC    0xE9

/**
  Return the result of detecting the debug I/O port device.

  @retval TRUE   if the debug I/O port device was detected.
  @retval FALSE  otherwise

**/
BOOLEAN
EFIAPI
PlatformDebugLibIoPortDetect (
  VOID
  )
{
  return IoRead8 (PcdGet16 (PcdDebugIoPort)) == BOCHS_DEBUG_PORT_MAGIC;
}
