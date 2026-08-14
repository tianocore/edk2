/** @file
  Arm Arch utility functions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ArchUtility.h"
#include <Library/ArmSmcccSocIdLib.h>

/**
  Return the SoC ID formatted for the SMBIOS Type 4 Processor ID field.

  @param[out] ProcessorId  Pointer to the SMBIOS Processor ID.

  @retval EFI_SUCCESS            The Processor ID was returned successfully.
  @retval EFI_INVALID_PARAMETER  ProcessorId is NULL.
  @retval EFI_UNSUPPORTED        Unsupported.
**/
EFI_STATUS
GetSocId (
  OUT UINT64  *ProcessorId
  )
{
  EFI_STATUS  Status;
  UINT32      Jep106Code;
  UINT32      SocRevision;

  if (ProcessorId == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ArmSmcccGetSocId (&Jep106Code, &SocRevision);
  if (!EFI_ERROR (Status)) {
    *ProcessorId = ((UINT64)SocRevision << 32) | Jep106Code;
  }

  return Status;
}
