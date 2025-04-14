/** @file
  File defines the Sec routines for the AMD SEV

  Copyright (c) 2021, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/CpuPageTableLib.h>
#include <Library/DebugLib.h>
#include <Library/LocalApicLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/BaseMemoryLib.h>
#include <Register/Amd/Ghcb.h>
#include <Register/Amd/Msr.h>

#include "AmdSev.h"

/**
  Handle an SEV-ES/GHCB protocol check failure.

  Notify the hypervisor using the VMGEXIT instruction that the SEV-ES guest
  wishes to be terminated.

  @param[in] ReasonCode  Reason code to provide to the hypervisor for the
                         termination request.

**/
VOID
SevEsProtocolFailure (
  IN UINT8  ReasonCode
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request termination by the hypervisor
  //
  Msr.GhcbPhysicalAddress         = 0;
  Msr.GhcbTerminate.Function      = GHCB_INFO_TERMINATE_REQUEST;
  Msr.GhcbTerminate.ReasonCodeSet = GHCB_TERMINATE_GHCB;
  Msr.GhcbTerminate.ReasonCode    = ReasonCode;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
  Determine if SEV-SNP is active.

  @retval TRUE   SEV-SNP is enabled
  @retval FALSE  SEV-SNP is not enabled

**/
BOOLEAN
SevSnpIsEnabled (
  VOID
  )
{
  MSR_SEV_STATUS_REGISTER  Msr;

  //
  // Read the SEV_STATUS MSR to determine whether SEV-SNP is active.
  //
  Msr.Uint32 = AsmReadMsr32 (MSR_SEV_STATUS);

  //
  // Check MSR_0xC0010131 Bit 2 (Sev-Snp Enabled)
  //
  if (Msr.Bits.SevSnpBit) {
    return TRUE;
  }

  return FALSE;
}

/**
 Register the GHCB GPA

*/
STATIC
VOID
SevSnpGhcbRegister (
  EFI_PHYSICAL_ADDRESS  Address
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request to register the GPA.
  //
  Msr.GhcbPhysicalAddress      = Address & ~EFI_PAGE_MASK;
  Msr.GhcbGpaRegister.Function = GHCB_INFO_GHCB_GPA_REGISTER_REQUEST;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);

  //
  // If hypervisor responded with a different GPA than requested then fail.
  //
  if ((Msr.GhcbGpaRegister.Function != GHCB_INFO_GHCB_GPA_REGISTER_RESPONSE) ||
      ((Msr.GhcbPhysicalAddress & ~EFI_PAGE_MASK) != Address))
  {
    SevEsProtocolFailure (GHCB_TERMINATE_GHCB_GENERAL);
  }
}

/**
 Verify that Hypervisor supports the SNP feature.

 */
STATIC
BOOLEAN
HypervisorSnpFeatureCheck (
  VOID
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  UINT64                    Features;

  //
  // Use the GHCB MSR Protocol to query the hypervisor capabilities
  //
  Msr.GhcbPhysicalAddress             = 0;
  Msr.GhcbHypervisorFeatures.Function = GHCB_HYPERVISOR_FEATURES_REQUEST;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);

  Features =  RShiftU64 (Msr.GhcbPhysicalAddress, 12);

  if ((Msr.GhcbHypervisorFeatures.Function != GHCB_HYPERVISOR_FEATURES_RESPONSE) ||
      (!(Features & GHCB_HV_FEATURES_SNP)))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Validate the SEV-ES/GHCB protocol level.

  Verify that the level of SEV-ES/GHCB protocol supported by the hypervisor
  and the guest intersect. If they don't intersect, request termination.

**/
VOID
SevEsProtocolCheck (
  VOID
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  GHCB                      *Ghcb;

  //
  // Use the GHCB MSR Protocol to obtain the GHCB SEV-ES Information for
  // protocol checking
  //
  Msr.GhcbPhysicalAddress = 0;
  Msr.GhcbInfo.Function   = GHCB_INFO_SEV_INFO_GET;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);

  if (Msr.GhcbInfo.Function != GHCB_INFO_SEV_INFO) {
    SevEsProtocolFailure (GHCB_TERMINATE_GHCB_GENERAL);
  }

  if (Msr.GhcbProtocol.SevEsProtocolMin > Msr.GhcbProtocol.SevEsProtocolMax) {
    SevEsProtocolFailure (GHCB_TERMINATE_GHCB_PROTOCOL);
  }

  if ((Msr.GhcbProtocol.SevEsProtocolMin > GHCB_VERSION_MAX) ||
      (Msr.GhcbProtocol.SevEsProtocolMax < GHCB_VERSION_MIN))
  {
    SevEsProtocolFailure (GHCB_TERMINATE_GHCB_PROTOCOL);
  }

  //
  // We cannot use the MemEncryptSevSnpIsEnabled () because the
  // ProcessLibraryConstructorList () is not called yet.
  //
  if (SevSnpIsEnabled ()) {
    //
    // Check if hypervisor supports the SNP feature
    //
    if (!HypervisorSnpFeatureCheck ()) {
      SevEsProtocolFailure (GHCB_TERMINATE_GHCB_PROTOCOL);
    }

    //
    // Unlike the SEV-ES guest, the SNP requires that GHCB GPA must be
    // registered with the Hypervisor before the use. This can be done
    // using the new VMGEXIT defined in the GHCB v2. Register the GPA
    // before it is used.
    //
    SevSnpGhcbRegister ((EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfSecGhcbBase));
  }

  //
  // SEV-ES protocol checking succeeded, set the initial GHCB address
  //
  Msr.GhcbPhysicalAddress = FixedPcdGet32 (PcdOvmfSecGhcbBase);
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  Ghcb = Msr.Ghcb;
  SetMem (Ghcb, FixedPcdGet32 (PcdOvmfSecGhcbSize), 0);

  //
  // Set the version to the maximum that can be supported
  //
  Ghcb->ProtocolVersion = MIN (Msr.GhcbProtocol.SevEsProtocolMax, GHCB_VERSION_MAX);
  Ghcb->GhcbUsage       = GHCB_STANDARD_USAGE;
}

/**
 Determine if the SEV is active.

 During the early booting, GuestType is set in the work area. Verify that it
 is an SEV guest.

 @retval TRUE   SEV is enabled
 @retval FALSE  SEV is not enabled

**/
BOOLEAN
IsSevGuest (
  VOID
  )
{
  OVMF_WORK_AREA  *WorkArea;

  //
  // Ensure that the size of the Confidential Computing work area header
  // is same as what is provided through a fixed PCD.
  //
  ASSERT (
    (UINTN)FixedPcdGet32 (PcdOvmfConfidentialComputingWorkAreaHeader) ==
    sizeof (CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER)
    );

  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);

  return ((WorkArea != NULL) && (WorkArea->Header.GuestType == CcGuestTypeAmdSev));
}

/**
  Determine if SEV-ES is active.

  During early booting, SEV-ES support code will set a flag to indicate that
  SEV-ES is enabled. Return the value of this flag as an indicator that SEV-ES
  is enabled.

  @retval TRUE   SEV-ES is enabled
  @retval FALSE  SEV-ES is not enabled

**/
BOOLEAN
SevEsIsEnabled (
  VOID
  )
{
  SEC_SEV_ES_WORK_AREA  *SevEsWorkArea;

  if (!IsSevGuest ()) {
    return FALSE;
  }

  SevEsWorkArea = (SEC_SEV_ES_WORK_AREA *)FixedPcdGet32 (PcdSevEsWorkAreaBase);

  return ((SevEsWorkArea->SevStatusMsrValue & BIT1) != 0);
}

/**
 Validate System RAM used for decompressing the PEI and DXE firmware volumes
 when SEV-SNP is active. The PCDs SecValidatedStart and SecValidatedEnd are
 set in OvmfPkg/Include/Fdf/FvmainCompactScratchEnd.fdf.inc.

**/
VOID
SecValidateSystemRam (
  VOID
  )
{
  PHYSICAL_ADDRESS  Start, End;

  if (IsSevGuest () && SevSnpIsEnabled ()) {
    Start = (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecValidatedStart);
    End   = (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecValidatedEnd);

    MemEncryptSevSnpPreValidateSystemRam (Start, EFI_SIZE_TO_PAGES ((UINTN)(End - Start)));
  }
}

/**
  Map known MMIO regions unencrypted if SEV-ES is active.

  During early booting, page table entries default to having the encryption bit
  set for SEV-ES/SEV-SNP guests. In cases where there is MMIO to an address, the
  encryption bit should be cleared. Clear it here for any known MMIO accesses
  during SEC, which is currently just the APIC base address.

**/
VOID
SecMapApicBaseUnencrypted (
  VOID
  )
{
  IA32_CR4            Cr4;
  PAGING_MODE         PagingMode;
  PHYSICAL_ADDRESS    Cr3;
  UINT64              ApicAddress;
  VOID                *Buffer;
  UINTN               BufferSize;
  IA32_MAP_ATTRIBUTE  MapAttribute;
  IA32_MAP_ATTRIBUTE  MapMask;
  RETURN_STATUS       Status;

  if (!SevEsIsEnabled ()) {
    return;
  }

  Cr4.UintN  = AsmReadCr4 ();
  PagingMode = Cr4.Bits.LA57 ? Paging5Level : Paging4Level;

  ApicAddress = (UINT64)GetLocalApicBaseAddress ();
  Buffer      = (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfSecApicPageTableBase);
  Cr3         = AsmReadCr3 ();

  MapAttribute.Uint64         = ApicAddress;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;
  MapMask.Uint64              = MAX_UINT64;
  BufferSize                  = SIZE_4KB;

  Status = PageTableMap (
             (UINTN *)&Cr3,
             PagingMode,
             Buffer,
             &BufferSize,
             ApicAddress,
             SIZE_4KB,
             &MapAttribute,
             &MapMask,
             NULL
             );
  if (RETURN_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to map APIC MMIO region as unencrypted: %d\n", Status));
    ASSERT (FALSE);
  }

  CpuFlushTlb ();
}
