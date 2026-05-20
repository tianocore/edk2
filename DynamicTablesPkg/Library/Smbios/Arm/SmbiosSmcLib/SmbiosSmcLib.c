/** @file
  SMBIOS SMC helper functions.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2021 - 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmSmcLib.h>
#include <Library/SmbiosSmcLib.h>

/** Checks if the ARM64 SoC ID SMC call is supported

    @return Whether the ARM64 SoC ID call is supported.
**/
STATIC
BOOLEAN
HasSmcArm64SocId (
  VOID
  )
{
  INT32    SmcCallStatus;
  BOOLEAN  Arm64SocIdSupported;
  UINTN    SmcParam;

  Arm64SocIdSupported = FALSE;

  SmcCallStatus = ArmCallSmc0 (SMCCC_VERSION, NULL, NULL, NULL);

  if ((SmcCallStatus < 0) || ((SmcCallStatus >> 16) >= 1)) {
    SmcParam      = SMCCC_ARCH_SOC_ID;
    SmcCallStatus = ArmCallSmc1 (SMCCC_ARCH_FEATURES, &SmcParam, NULL, NULL);
    if (SmcCallStatus >= 0) {
      Arm64SocIdSupported = TRUE;
    }
  }

  return Arm64SocIdSupported;
}

/** Fetches the JEP106 code and SoC Revision.

    @param Jep106Code  JEP 106 code.
    @param SocRevision SoC revision.

    @retval EFI_SUCCESS Succeeded.
    @retval EFI_UNSUPPORTED Failed.
**/
STATIC
EFI_STATUS
SmbiosGetSmcArm64SocId (
  OUT UINT32  *Jep106Code,
  OUT UINT32  *SocRevision
  )
{
  INT32       SmcCallStatus;
  EFI_STATUS  Status;
  UINTN       SmcParam;

  Status = EFI_SUCCESS;

  SmcParam      = 0;
  SmcCallStatus = ArmCallSmc1 (SMCCC_ARCH_SOC_ID, &SmcParam, NULL, NULL);

  if (SmcCallStatus >= 0) {
    *Jep106Code = (UINT32)SmcCallStatus;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  SmcParam      = 1;
  SmcCallStatus = ArmCallSmc1 (SMCCC_ARCH_SOC_ID, &SmcParam, NULL, NULL);

  if (SmcCallStatus >= 0) {
    *SocRevision = (UINT32)SmcCallStatus;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/** Returns the SOC ID, formatted for the SMBIOS Type 4 Processor ID field.

    @param Processor ID.

    @return 0 on success
    @return EFI_UNSUPPORTED if SMCCC_ARCH_SOC_ID is not implemented
**/
UINT64
SmbiosSmcGetSocId (
  UINT64  *ProcessorId
  )
{
  EFI_STATUS  Status;
  UINT32      Jep106Code;
  UINT32      SocRevision;

  if (HasSmcArm64SocId ()) {
    Status = SmbiosGetSmcArm64SocId (&Jep106Code, &SocRevision);
    if (!EFI_ERROR (Status)) {
      *ProcessorId = ((UINT64)SocRevision << 32) | Jep106Code;
    }
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}
