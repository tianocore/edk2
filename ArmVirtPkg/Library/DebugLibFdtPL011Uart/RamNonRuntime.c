/** @file
  Provide an empty lib instance constructor for modules that can only run from
  RAM but are not DXE_RUNTIME_DRIVER modules.

  This ensures that e.g. any HobLib constructor is ordered correctly. (The
  DXE_CORE calls constructors late, but the DXE_CORE HobLib instance needs no
  construction anyway.)

  Copyright (C) Red Hat

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

/**
  Empty library instance constructor, only for ensuring the connectivity of the
  constructor dependency graph.
**/
RETURN_STATUS
EFIAPI
DebugLibFdtPL011UartRamConstructor (
  VOID
  )
{
  return RETURN_SUCCESS;
}
