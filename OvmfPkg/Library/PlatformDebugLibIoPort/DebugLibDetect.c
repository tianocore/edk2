/** @file
  Detection code for QEMU debug port.
  Non-SEC instance, caches the result of detection.

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

//
// Set to TRUE if the debug I/O port has been checked
//
STATIC BOOLEAN mDebugIoPortChecked = FALSE;

//
// Set to TRUE if the debug I/O port is enabled
//
STATIC BOOLEAN mDebugIoPortFound = FALSE;

/**
  This constructor function must not do anything.

  Some modules consuming this library instance, such as the DXE Core, invoke
  the DEBUG() macro before they explicitly call
  ProcessLibraryConstructorList(). Therefore the auto-generated call from
  ProcessLibraryConstructorList() to this constructor function may be preceded
  by some calls to PlatformDebugLibIoPortFound() below. Hence
  PlatformDebugLibIoPortFound() must not rely on anything this constructor
  could set up.

  @retval RETURN_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
PlatformDebugLibIoPortConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
}

/**
  At the first call, check if the debug I/O port device is present, and cache
  the result for later use. At subsequent calls, return the cached result.

  @retval TRUE   if the debug I/O port device was detected.
  @retval FALSE  otherwise

**/
BOOLEAN
EFIAPI
PlatformDebugLibIoPortFound (
  VOID
  )
{
  if (!mDebugIoPortChecked) {
    mDebugIoPortFound = PlatformDebugLibIoPortDetect ();
    mDebugIoPortChecked = TRUE;
  }
  return mDebugIoPortFound;
}
