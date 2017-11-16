/** @file
  Base Debug library instance for QEMU debug port.
  It uses PrintLib to send debug messages to a fixed I/O port.

  Copyright (c) 2017, Red Hat, Inc.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUG_IO_PORT_DETECT_H__
#define __DEBUG_IO_PORT_DETECT_H__

#include <Base.h>

//
// The constant value that is read from the debug I/O port
//
#define BOCHS_DEBUG_PORT_MAGIC    0xE9


/**
  Helper function to return whether the virtual machine has a debug I/O port.
  PlatformDebugLibIoPortFound can call this function directly or cache the
  result.

  @retval TRUE   if the debug I/O port device was detected.
  @retval FALSE  otherwise

**/
BOOLEAN
EFIAPI
PlatformDebugLibIoPortDetect (
  VOID
  );

/**
  Return whether the virtual machine has a debug I/O port.  DebugLib.c
  calls this function instead of PlatformDebugLibIoPortDetect, to allow
  caching if possible.

  @retval TRUE   if the debug I/O port device was detected.
  @retval FALSE  otherwise

**/
BOOLEAN
EFIAPI
PlatformDebugLibIoPortFound (
  VOID
  );

#endif
