/** @file
  GHCBRegister Support Library.

  Copyright (C) 2021, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/VmgExitLib.h>
#include <Library/GhcbRegisterLib.h>
#include <Register/Amd/Msr.h>

/**
  Handle an SEV-SNP/GHCB protocol check failure.

  Notify the hypervisor using the VMGEXIT instruction that the SEV-SNP guest
  wishes to be terminated.

  @param[in] ReasonCode  Reason code to provide to the hypervisor for the
                         termination request.

**/
STATIC
VOID
SevEsProtocolFailure (
  IN UINT8  ReasonCode
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request termination by the hypervisor
  //
  Msr.GhcbPhysicalAddress = 0;
  Msr.GhcbTerminate.Function = GHCB_INFO_TERMINATE_REQUEST;
  Msr.GhcbTerminate.ReasonCodeSet = GHCB_TERMINATE_GHCB;
  Msr.GhcbTerminate.ReasonCode = ReasonCode;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**

  This function can be used to register the GHCB GPA.

  @param[in]  Address           The physical address to be registered.

**/
VOID
EFIAPI
GhcbRegister (
  IN  EFI_PHYSICAL_ADDRESS   Address
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  MSR_SEV_ES_GHCB_REGISTER  CurrentMsr;
  EFI_PHYSICAL_ADDRESS      GuestFrameNumber;

  GuestFrameNumber = Address >> EFI_PAGE_SHIFT;

  //
  // Save the current MSR Value
  //
  CurrentMsr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);

  //
  // Use the GHCB MSR Protocol to request to register the GPA.
  //
  Msr.GhcbPhysicalAddress = 0;
  Msr.GhcbGpaRegister.Function = GHCB_INFO_GHCB_GPA_REGISTER_REQUEST;
  Msr.GhcbGpaRegister.GuestFrameNumber = GuestFrameNumber;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);

  //
  // If hypervisor responded with a different GPA than requested then fail.
  //
  if ((Msr.GhcbGpaRegister.Function != GHCB_INFO_GHCB_GPA_REGISTER_RESPONSE) ||
      (Msr.GhcbGpaRegister.GuestFrameNumber != GuestFrameNumber)) {
    SevEsProtocolFailure (GHCB_TERMINATE_GHCB_GENERAL);
  }

  //
  // Restore the MSR
  //
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, CurrentMsr.GhcbPhysicalAddress);
}
