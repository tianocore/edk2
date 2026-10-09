/** @file
  Legacy BIOS memory protection does not apply for non-x64 archs, this just provides
  a stub.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"

/**
  Maps legacy BIOS memory as readable, writeable, and executable where applicable.
**/
VOID
MapLegacyBiosMemoryRWX (
  VOID
  )
{
  return;
}
