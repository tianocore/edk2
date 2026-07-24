/** @file
  Arm SMCCC SoC ID library.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/**
  Check whether the SMCCC Architecture SoC ID interface is supported.

  @retval TRUE   The SMCCC Architecture SoC ID interface is supported.
  @retval FALSE  The SMCCC Architecture SoC ID interface is not supported.
**/
BOOLEAN
ArmSmcccSocIdIsSupported (
  VOID
  );

/**
  Get the JEP106 identification code and SoC revision using the SMCCC
  Architecture SoC ID interface.

  @param[out] Jep106Code   Pointer to the JEP106 identification code.
  @param[out] SocRevision  Pointer to the SoC revision.

  @retval EFI_SUCCESS            The SoC identification information was
                                 returned successfully.
  @retval EFI_INVALID_PARAMETER  A required output parameter is NULL.
  @retval EFI_UNSUPPORTED        The SMCCC Architecture SoC ID interface is
                                 unsupported or an SoC ID call failed.
**/
EFI_STATUS
ArmSmcccGetSocId (
  OUT UINT32  *Jep106Code,
  OUT UINT32  *SocRevision
  );
