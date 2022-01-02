/** @file
  NULL library instance of PlatformSecPpiLib

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

/** Return platform SEC PPI before PEI Core

  @param[in,out]  ThisPpiList   Pointer to retrieve EFI_PEI_PPI_DESCRIPTOR.

**/
EFI_STATUS
GetPlatformPrePeiCorePpiDescriptor (
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **ThisPpiList
)
{
  *ThisPpiList = NULL;
  return EFI_NOT_FOUND;
}

