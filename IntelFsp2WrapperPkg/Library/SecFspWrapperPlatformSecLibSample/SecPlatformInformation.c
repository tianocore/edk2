/** @file
  Sample to provide SecPlatformInformation function.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/SecPlatformInformation.h>
#include <Ppi/TopOfTemporaryRam.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param[in]     PeiServices               Pointer to the PEI Services Table.
  @param[in,out] StructureSize             Pointer to the variable describing size of the input buffer.
  @param[out]    PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation (
  IN CONST EFI_PEI_SERVICES                  **PeiServices,
  IN OUT   UINT64                            *StructureSize,
  OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  )
{
  UINT32      *Bist;
  UINT32      Size;
  UINT32      Count;
  UINT32      TopOfTemporaryRam;
  VOID        *TopOfTemporaryRamPpi;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "SecPlatformInformation\n"));

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gTopOfTemporaryRamPpiGuid,
                             0,
                             NULL,
                             (VOID **)&TopOfTemporaryRamPpi
                             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // The entries of BIST information, together with the number of them,
  // reside in the bottom of stack, left untouched by normal stack operation.
  // This routine copies the BIST information to the buffer pointed by
  // PlatformInformationRecord for output.
  //
  TopOfTemporaryRam  = (UINT32)(UINTN)TopOfTemporaryRamPpi - sizeof (UINT32);
  TopOfTemporaryRam -= sizeof (UINT32) * 2;
  Count              = *((UINT32 *)(UINTN)(TopOfTemporaryRam - sizeof (UINT32)));
  Size               = Count * sizeof (IA32_HANDOFF_STATUS);

  if ((*StructureSize) < (UINT64)Size) {
    *StructureSize = Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StructureSize = Size;
  Bist           = (UINT32 *)(TopOfTemporaryRam - sizeof (UINT32) - Size);

  CopyMem (PlatformInformationRecord, Bist, Size);

  return EFI_SUCCESS;
}
