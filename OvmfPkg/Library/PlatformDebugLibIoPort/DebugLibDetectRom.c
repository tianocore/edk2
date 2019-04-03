/** @file
  Detection code for QEMU debug port.
  SEC instance, cannot cache the result of detection.

  Copyright (c) 2017, Red Hat, Inc.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include "DebugLibDetect.h"

/**
  This constructor function does not have anything to do.

  @retval RETURN_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
PlatformRomDebugLibIoPortConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
}

/**
  Return the result of detecting the debug I/O port device.

  @retval TRUE   if the debug I/O port device was detected.
  @retval FALSE  otherwise

**/
BOOLEAN
EFIAPI
PlatformDebugLibIoPortFound (
  VOID
  )
{
  return PlatformDebugLibIoPortDetect ();
}
