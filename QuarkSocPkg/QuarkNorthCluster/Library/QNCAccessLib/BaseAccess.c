/** @file
Base Lib function for QNC internal network access.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <Uefi.h>

/**
  Gets the base address of PCI Express for Quark North Cluster.

  @return The base address of PCI Express for Quark North Cluster.

**/
UINTN
EFIAPI
QncGetPciExpressBaseAddress (
  VOID
  )
{
  return (UINTN) PcdGet64(PcdPciExpressBaseAddress);
}
