/** @file
  Non-existing functions other than Ia32 architecture.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  Non-existing.c

**/

#include "DxeIpl.h"

EFI_PHYSICAL_ADDRESS
CreateIdentityMappingPageTables (
  IN UINT32                NumberOfProcessorPhysicalAddressBits
  )
{
  //
  // This function cannot work on non-Ia32 architecture.
  //
  ASSERT (FALSE);
  return 0;
}

VOID
ActivateLongMode (
  IN  EFI_PHYSICAL_ADDRESS  PageTables,
  IN  EFI_PHYSICAL_ADDRESS  HobStart,
  IN  EFI_PHYSICAL_ADDRESS  Stack,
  IN  EFI_PHYSICAL_ADDRESS  CodeEntryPoint1,
  IN  EFI_PHYSICAL_ADDRESS  CodeEntryPoint2
  )
{
  //
  // This function cannot work on non-Ia32 architecture.
  //
  ASSERT (FALSE);
}

VOID
LoadGo64Gdt(
  VOID
  )
{
  //
  // This function cannot work on non-Ia32 architecture.
  //
  ASSERT (FALSE);
}
