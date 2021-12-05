/** @file
  The header file of SMM access DXE.

Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_ACCESS_DRIVER_H_
#define SMM_ACCESS_DRIVER_H_

#include <PiDxe.h>
#include <Protocol/SmmAccess2.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/SmramMemoryReserve.h>

#define  SMM_ACCESS_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('S', 'M', 'M', 'A')

typedef struct {
  UINTN                       Signature;
  EFI_HANDLE                  Handle;
  EFI_SMM_ACCESS2_PROTOCOL    SmmAccess;
  //
  // Local Data for SMM Access interface goes here
  //
  UINT32                      SmmRegionState;
  UINT32                      NumberRegions;
  EFI_SMRAM_DESCRIPTOR        *SmramDesc;
} SMM_ACCESS_PRIVATE_DATA;

#endif
