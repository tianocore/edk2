/** @file
  Detection code for QEMU debug port.
  SEC instance, cannot cache the result of detection.

  Copyright (c) 2017, Red Hat, Inc.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
