/** @file
  SVSM Support Library.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/CcSvsmLib.h>
#include <Register/Amd/Msr.h>
#include <Register/Amd/Svsm.h>

#define PAGES_PER_2MB_ENTRY  512

/**
  Issue a GHCB termination request for termination.

  Request termination using the GHCB MSR protocol.

**/
STATIC
VOID
SnpTerminate (
  VOID
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request termination by the hypervisor
  //
  Msr.Uint64                      = 0;
  Msr.GhcbTerminate.Function      = GHCB_INFO_TERMINATE_REQUEST;
  Msr.GhcbTerminate.ReasonCodeSet = GHCB_TERMINATE_GHCB;
  Msr.GhcbTerminate.ReasonCode    = GHCB_TERMINATE_GHCB_GENERAL;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.Uint64);

  AsmVmgExit ();

  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Report the presence of an Secure Virtual Services Module (SVSM).

  Determines the presence of an SVSM.

  @retval  TRUE                   An SVSM is present
  @retval  FALSE                  An SVSM is not present

**/
BOOLEAN
EFIAPI
CcSvsmIsSvsmPresent (
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
CcSvsmSnpGetVmpl (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return CcSvsmIsSvsmPresent () ? SvsmInfo->SvsmGuestVmpl : 0;
}

/**
  Report the Calling Area address (CAA) for the BSP of the SEV-SNP guest.

  If an SVSM is present, the CAA for the BSP is returned.

  @return                         The CAA

**/
UINT64
EFIAPI
CcSvsmSnpGetCaa (
  VOID
  )
{
  SVSM_INFORMATION  *SvsmInfo;

  SvsmInfo = (SVSM_INFORMATION *)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase);

  return CcSvsmIsSvsmPresent () ? SvsmInfo->SvsmCaa : 0;
}

/**
  Perform a native PVALIDATE operation for the page ranges specified.

  Validate or rescind the validation of the specified pages.

  @param[in]       Info           Pointer to a page state change structure

**/
STATIC
VOID
BasePvalidate (
  IN  SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  UINTN                 RmpPageSize;
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  UINTN                 Index;
  UINTN                 Ret;
  EFI_PHYSICAL_ADDRESS  Address;
  BOOLEAN               Validate;

  StartIndex = Info->Header.CurrentEntry;
  EndIndex   = Info->Header.EndEntry;

  for ( ; StartIndex <= EndIndex; StartIndex++) {
    //
    // Get the address and the page size from the Info.
    //
    Address     = ((EFI_PHYSICAL_ADDRESS)Info->Entry[StartIndex].GuestFrameNumber) << EFI_PAGE_SHIFT;
    RmpPageSize = Info->Entry[StartIndex].PageSize;
    Validate    = Info->Entry[StartIndex].Operation == SNP_PAGE_STATE_PRIVATE;

    Ret = AsmPvalidate (RmpPageSize, Validate, Address);

    //
    // If we fail to validate due to size mismatch then try with the
    // smaller page size. This senario will occur if the backing page in
    // the RMP entry is 4K and we are validating it as a 2MB.
    //
    if ((Ret == PVALIDATE_RET_SIZE_MISMATCH) && (RmpPageSize == PvalidatePageSize2MB)) {
      for (Index = 0; Index < PAGES_PER_2MB_ENTRY; Index++) {
        Ret = AsmPvalidate (PvalidatePageSize4K, Validate, Address);
        if (Ret) {
          break;
        }

        Address = Address + EFI_PAGE_SIZE;
      }
    }

    //
    // If validation failed then do not continue.
    //
    if (Ret) {
      DEBUG ((
        DEBUG_ERROR,
        "%a:%a: Failed to %a address 0x%Lx Error code %d\n",
        gEfiCallerBaseName,
        __func__,
        Validate ? "Validate" : "Invalidate",
        Address,
        Ret
        ));

      SnpTerminate ();
    }
  }
}

/**
  Perform a PVALIDATE operation for the page ranges specified.

  Validate or rescind the validation of the specified pages.

  @param[in]       Info           Pointer to a page state change structure

**/
VOID
EFIAPI
CcSvsmSnpPvalidate (
  IN SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  BasePvalidate (Info);
}

/**
  Perform a native RMPADJUST operation to alter the VMSA setting of a page.

  Add or remove the VMSA attribute for a page.

  @param[in]       Vmsa           Pointer to an SEV-ES save area page
  @param[in]       SetVmsa        Boolean indicator as to whether to set or
                                  or clear the VMSA setting for the page

  @retval  EFI_SUCCESS            RMPADJUST operation successful
  @retval  EFI_INVALID_PARAMETER  RMPADJUST operation failed, an invalid
                                  parameter was supplied

**/
STATIC
EFI_STATUS
BaseVmsaRmpAdjust (
  IN SEV_ES_SAVE_AREA  *Vmsa,
  IN BOOLEAN           SetVmsa
  )
{
  UINT64  Rdx;
  UINT32  Ret;

  //
  // The RMPADJUST instruction is used to set or clear the VMSA bit for a
  // page. The VMSA change is only made when running at VMPL0 and is ignored
  // otherwise. If too low a target VMPL is specified, the instruction can
  // succeed without changing the VMSA bit when not running at VMPL0. Using a
  // target VMPL level of 1, RMPADJUST will return a FAIL_PERMISSION error if
  // not running at VMPL0, thus ensuring that the VMSA bit is set appropriately
  // when no error is returned.
  //
  Rdx = 1;
  if (SetVmsa) {
    Rdx |= RMPADJUST_VMSA_PAGE_BIT;
  }

  Ret = AsmRmpAdjust ((UINT64)(UINTN)Vmsa, 0, Rdx);

  return (Ret == 0) ? EFI_SUCCESS : EFI_INVALID_PARAMETER;
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
CcSvsmSnpVmsaRmpAdjust (
  IN SEV_ES_SAVE_AREA  *Vmsa,
  IN UINT32            ApicId,
  IN BOOLEAN           SetVmsa
  )
{
  return BaseVmsaRmpAdjust (Vmsa, SetVmsa);
}
