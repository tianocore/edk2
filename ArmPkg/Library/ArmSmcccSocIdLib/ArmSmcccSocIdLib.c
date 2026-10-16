/** @file
  Arm SMCCC SoC ID library.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2021 - 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <IndustryStandard/ArmStdSmc.h>

#include <Library/ArmSmcLib.h>
#include <Library/ArmSmcccSocIdLib.h>

/**
  Check whether the SMCCC Architecture SoC ID interface is supported.

  @retval TRUE   The SMCCC Architecture SoC ID interface is supported.
  @retval FALSE  The SMCCC Architecture SoC ID interface is not supported.
**/
BOOLEAN
ArmSmcccSocIdIsSupported (
  VOID
  )
{
  INT32  SmcCallStatus;
  UINTN  SmcParameter;

  SmcCallStatus = ArmCallSmc0 (SMCCC_VERSION, NULL, NULL, NULL);
  if ((SmcCallStatus >= 0) && ((SmcCallStatus >> 16) < 1)) {
    return FALSE;
  }

  SmcParameter  = SMCCC_ARCH_SOC_ID;
  SmcCallStatus = ArmCallSmc1 (
                    SMCCC_ARCH_FEATURES,
                    &SmcParameter,
                    NULL,
                    NULL
                    );

  return (SmcCallStatus >= 0);
}

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
  )
{
  INT32   SmcCallStatus;
  UINTN   SmcParameter;
  UINT32  LocalJep106Code;
  UINT32  LocalSocRevision;

  if ((Jep106Code == NULL) || (SocRevision == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!ArmSmcccSocIdIsSupported ()) {
    return EFI_UNSUPPORTED;
  }

  SmcParameter  = 0;
  SmcCallStatus = ArmCallSmc1 (
                    SMCCC_ARCH_SOC_ID,
                    &SmcParameter,
                    NULL,
                    NULL
                    );
  if (SmcCallStatus < 0) {
    return EFI_UNSUPPORTED;
  }

  LocalJep106Code = (UINT32)SmcCallStatus;

  SmcParameter  = 1;
  SmcCallStatus = ArmCallSmc1 (
                    SMCCC_ARCH_SOC_ID,
                    &SmcParameter,
                    NULL,
                    NULL
                    );
  if (SmcCallStatus < 0) {
    return EFI_UNSUPPORTED;
  }

  LocalSocRevision = (UINT32)SmcCallStatus;

  *Jep106Code  = LocalJep106Code;
  *SocRevision = LocalSocRevision;

  return EFI_SUCCESS;
}
