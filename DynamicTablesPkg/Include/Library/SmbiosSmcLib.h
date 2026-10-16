/** @file
*
*  Copyright (c) 2025 - 2026, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#pragma once

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
  );
