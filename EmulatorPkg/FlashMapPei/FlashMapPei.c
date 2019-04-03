/*++ @file
  PEIM to build GUIDed HOBs for platform specific flash map

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "PiPei.h"

#include <Guid/SystemNvDataGuid.h>
#include <Ppi/EmuThunk.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
PeimInitializeFlashMap (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:
  Build GUIDed HOBs for platform specific flash map

Arguments:
  FfsHeader   - A pointer to the EFI_FFS_FILE_HEADER structure.
  PeiServices - General purpose services available to every PEIM.

Returns:
  EFI_STATUS

**/
{
  EFI_STATUS              Status;
  EMU_THUNK_PPI           *Thunk;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  EFI_PHYSICAL_ADDRESS    FdBase;
  EFI_PHYSICAL_ADDRESS    FdFixUp;
  UINT64                  FdSize;

  DEBUG ((EFI_D_ERROR, "EmulatorPkg Flash Map PEIM Loaded\n"));

  //
  // Get the Fwh Information PPI
  //
  Status = PeiServicesLocatePpi (
            &gEmuThunkPpiGuid, // GUID
            0,                 // INSTANCE
            &PpiDescriptor,     // EFI_PEI_PPI_DESCRIPTOR
            (VOID **)&Thunk       // PPI
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Assume that FD0 contains the Flash map.
  //
  Status = Thunk->FirmwareDevices (0, &FdBase, &FdSize, &FdFixUp);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PcdSet64 (PcdFlashNvStorageVariableBase64, PcdGet64 (PcdEmuFlashNvStorageVariableBase) + FdFixUp);
  PcdSet64 (PcdFlashNvStorageFtwWorkingBase64, PcdGet64 (PcdEmuFlashNvStorageFtwWorkingBase) + FdFixUp);
  PcdSet64 (PcdFlashNvStorageFtwSpareBase64, PcdGet64 (PcdEmuFlashNvStorageFtwSpareBase) + FdFixUp);

  return EFI_SUCCESS;
}
