/** @file
  SMBIOS SMC helper functions.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2021 - 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025 - 2026, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ArmSmcccSocIdLib.h>
#include <Library/SmbiosSmcLib.h>

/**
  Return the SoC ID formatted for the SMBIOS Type 4 Processor ID field.

  @param[out] ProcessorId  Pointer to the SMBIOS Processor ID.

  @retval EFI_SUCCESS            The Processor ID was returned successfully.
  @retval EFI_INVALID_PARAMETER  ProcessorId is NULL.
  @retval EFI_UNSUPPORTED        The SMCCC Architecture SoC ID interface is
                                 unsupported or an SoC ID call failed.
**/
EFI_STATUS
SmbiosSmcGetSocId (
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
