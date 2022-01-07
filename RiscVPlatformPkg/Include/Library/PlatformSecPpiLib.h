/** @file
  RISC-V platform SEC PPI before PEI Core.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RISCV_PLATFORM_SEC_PPI_H_
#define RISCV_PLATFORM_SEC_PPI_H_

#include <PiPei.h>

/** Return platform SEC PPI before PEI Core

  @param[in,out]  ThisPpiList   Pointer to retrieve EFI_PEI_PPI_DESCRIPTOR.

**/
EFI_STATUS
GetPlatformPrePeiCorePpiDescriptor (
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **ThisPpiList
  );

#endif
