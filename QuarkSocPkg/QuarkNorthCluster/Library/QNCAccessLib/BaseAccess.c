/** @file
Base Lib function for QNC internal network access.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
