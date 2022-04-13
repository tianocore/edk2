/** @file
  Detect Xen SMBIOS data on ARM / AARCH64.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "XenSmbiosPlatformDxe.h"

/**
  Locates the Xen SMBIOS data if it exists

  @return SMBIOS_TABLE_ENTRY_POINT   Address of Xen SMBIOS data

**/
SMBIOS_TABLE_ENTRY_POINT *
GetXenSmbiosTables (
  VOID
  )
{
  //
  // Not implemented yet.
  //
  return NULL;
}
