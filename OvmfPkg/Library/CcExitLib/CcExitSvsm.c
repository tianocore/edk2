/** @file
  SVSM Support Library.

  Copyright (C) 2022, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CcExitLib.h>
#include <Register/Amd/Msr.h>
#include <Register/Amd/Svsm.h>

/**
  Report the presence of an Secure Virtual Services Module (SVSM).

  Determines the presence of an SVSM.

  @retval  TRUE                   An SVSM is present
  @retval  FALSE                  An SVSM is not present

**/
BOOLEAN
EFIAPI
CcExitSnpSvsmPresent (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return (SvsmInfo != NULL && SvsmInfo->SvsmSize != 0);
}

/**
  Report the VMPL level at which the SEV-SNP guest is running.

  Determines the VMPL level at which the guest is running. If an SVSM is
  not present, then it must be VMPL0, otherwise return what is reported
  by the SVSM.

  @return                         The VMPL level

**/
UINT8
EFIAPI
CcExitSnpGetVmpl (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return CcExitSnpSvsmPresent () ? SvsmInfo->SvsmGuestVmpl : 0;
}

/**
  Perform a PVALIDATE operation for the page ranges specified.

  Validate or rescind the validation of the specified pages.

  @param[in]       Info           Pointer to a page state change structure

**/
VOID
EFIAPI
CcExitSnpPvalidate (
  IN SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
}

/**
  Perform an RMPADJUST operation to alter the VMSA setting of a page.

  Add or remove the VMSA attribute for a page.

  @param[in]       Vmsa           Pointer to an SEV-ES save area page
  @param[in]       ApicId         APIC ID associated with the VMSA
  @param[in]       SetVmsa        Boolean indicator as to whether to set or
                                  or clear the VMSA setting for the page

  @retval  EFI_SUCCESS            RMPADJUST operation successful
  @retval  EFI_UNSUPPORTED        Operation is not supported
  @retval  EFI_INVALID_PARAMETER  RMPADJUST operation failed, an invalid
                                  parameter was supplied

**/
EFI_STATUS
EFIAPI
CcExitSnpVmsaRmpAdjust (
  IN SEV_ES_SAVE_AREA  *Vmsa,
  IN UINT32            ApicId,
  IN BOOLEAN           SetVmsa
  )
{
  return EFI_UNSUPPORTED;
}
