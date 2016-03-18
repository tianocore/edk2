/** @file
  Detect Xen SMBIOS data on ARM / AARCH64.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "SmbiosPlatformDxe.h"

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
