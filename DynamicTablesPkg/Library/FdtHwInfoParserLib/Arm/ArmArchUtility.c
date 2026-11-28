/** @file
  Arm Arch utility functions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "ArchUtility.h"
#include <Library/ArmSmcccSocIdLib.h>
#include <Library/DebugLib.h>

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

/**
  Return the token of the CPU interface object describing a CPU.

  The CPU interface object (e.g. GICC or RINTC) is referenced by the processor
  hierarchy node of the CPU.

  @param[in] CpuIndex  Index of the CPU device node below the "/cpus" node.

  @return The token of the CPU interface object.
**/
CM_OBJECT_TOKEN
GetCpuToken (
  IN UINT32  CpuIndex
  )
{
  ASSERT ((CpuIndex & ~CM_ABSTRACT_TOKEN_ID_MASK) == 0);
  return (CM_OBJECT_TOKEN)CM_ABSTRACT_TOKEN_MAKE (
                            ETokenNameSpaceFdtHwInfo,
                            EFdtHwInfoGicCObject,
                            CpuIndex
                            );
}
