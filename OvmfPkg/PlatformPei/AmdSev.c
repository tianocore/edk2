/**@file
  Initialize Secure Encrypted Virtualization (SEV) support

  Copyright (c) 2017 - 2020, Advanced Micro Devices. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
//
// The package level header files this module uses
//
#include <IndustryStandard/Q35MchIch9.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Pi/PrePiHob.h>
#include <PiPei.h>
#include <Register/Amd/Msr.h>
#include <Register/Intel/SmramSaveStateMap.h>
#include <Library/CcExitLib.h>
#include <ConfidentialComputingGuestAttr.h>

#include "Platform.h"

STATIC
UINT64
GetHypervisorFeature (
  VOID
  );

/**
  Initialize SEV-SNP support if running as an SEV-SNP guest.

**/
STATIC
VOID
AmdSevSnpInitialize (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  UINT64                       HvFeatures;
  EFI_STATUS                   PcdStatus;

  if (!MemEncryptSevSnpIsEnabled ()) {
    return;
  }

  //
  // Query the hypervisor feature using the CcExitVmgExit and set the value in the
  // hypervisor features PCD.
  //
  HvFeatures = GetHypervisorFeature ();
  PcdStatus  = PcdSet64S (PcdGhcbHypervisorFeatures, HvFeatures);
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // Iterate through the system RAM and validate it.
  //
  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if ((Hob.Raw != NULL) && (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR)) {
      ResourceHob = Hob.ResourceDescriptor;

      if (ResourceHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
        if (ResourceHob->PhysicalStart >= SIZE_4GB) {
          ResourceHob->ResourceType = BZ3937_EFI_RESOURCE_MEMORY_UNACCEPTED;
          continue;
        }

        MemEncryptSevSnpPreValidateSystemRam (
          ResourceHob->PhysicalStart,
          EFI_SIZE_TO_PAGES ((UINTN)ResourceHob->ResourceLength)
          );
      }
    }
  }
}

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
 Get the hypervisor features bitmap

**/
STATIC
UINT64
GetHypervisorFeature (
  VOID
  )
{
  UINT64                    Status;
  GHCB                      *Ghcb;
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  BOOLEAN                   InterruptState;
  UINT64                    Features;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  Ghcb                    = Msr.Ghcb;

  //
  // Initialize the GHCB
  //
  CcExitVmgInit (Ghcb, &InterruptState);

  //
  // Query the Hypervisor Features.
  //
  Status = CcExitVmgExit (Ghcb, SVM_EXIT_HYPERVISOR_FEATURES, 0, 0);
  if ((Status != 0)) {
    SevEsProtocolFailure (GHCB_TERMINATE_GHCB_GENERAL);
  }

  Features = Ghcb->SaveArea.SwExitInfo2;

  CcExitVmgDone (Ghcb, InterruptState);

  return Features;
}

/**

  This function can be used to register the GHCB GPA.

  @param[in]  Address           The physical address to be registered.

**/
STATIC
VOID
GhcbRegister (
  IN  EFI_PHYSICAL_ADDRESS  Address
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;
  MSR_SEV_ES_GHCB_REGISTER  CurrentMsr;

  //
  // Save the current MSR Value
  //
  CurrentMsr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);

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

  //
  // Restore the MSR
  //
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, CurrentMsr.GhcbPhysicalAddress);
}

/**

  Initialize SEV-ES support if running as an SEV-ES guest.

  **/
STATIC
VOID
AmdSevEsInitialize (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT8                *GhcbBase;
  PHYSICAL_ADDRESS     GhcbBasePa;
  UINTN                GhcbPageCount;
  UINT8                *GhcbBackupBase;
  UINT8                *GhcbBackupPages;
  UINTN                GhcbBackupPageCount;
  SEV_ES_PER_CPU_DATA  *SevEsData;
  UINTN                PageCount;
  RETURN_STATUS        Status;
  IA32_DESCRIPTOR      Gdtr;
  VOID                 *Gdt;

  if (!MemEncryptSevEsIsEnabled ()) {
    return;
  }

  Status = PcdSetBoolS (PcdSevEsIsEnabled, TRUE);
  ASSERT_RETURN_ERROR (Status);

  //
  // Allocate GHCB and per-CPU variable pages.
  //   Since the pages must survive across the UEFI to OS transition
  //   make them reserved.
  //
  GhcbPageCount = PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber * 2;
  GhcbBase      = AllocateReservedPages (GhcbPageCount);
  ASSERT (GhcbBase != NULL);

  GhcbBasePa = (PHYSICAL_ADDRESS)(UINTN)GhcbBase;

  //
  // Each vCPU gets two consecutive pages, the first is the GHCB and the
  // second is the per-CPU variable page. Loop through the allocation and
  // only clear the encryption mask for the GHCB pages.
  //
  for (PageCount = 0; PageCount < GhcbPageCount; PageCount += 2) {
    Status = MemEncryptSevClearPageEncMask (
               0,
               GhcbBasePa + EFI_PAGES_TO_SIZE (PageCount),
               1
               );
    ASSERT_RETURN_ERROR (Status);
  }

  ZeroMem (GhcbBase, EFI_PAGES_TO_SIZE (GhcbPageCount));

  Status = PcdSet64S (PcdGhcbBase, GhcbBasePa);
  ASSERT_RETURN_ERROR (Status);
  Status = PcdSet64S (PcdGhcbSize, EFI_PAGES_TO_SIZE (GhcbPageCount));
  ASSERT_RETURN_ERROR (Status);

  DEBUG ((
    DEBUG_INFO,
    "SEV-ES is enabled, %lu GHCB pages allocated starting at 0x%p\n",
    (UINT64)GhcbPageCount,
    GhcbBase
    ));

  //
  // Allocate #VC recursion backup pages. The number of backup pages needed is
  // one less than the maximum VC count.
  //
  GhcbBackupPageCount = PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber * (VMGEXIT_MAXIMUM_VC_COUNT - 1);
  GhcbBackupBase      = AllocatePages (GhcbBackupPageCount);
  ASSERT (GhcbBackupBase != NULL);

  GhcbBackupPages = GhcbBackupBase;
  for (PageCount = 1; PageCount < GhcbPageCount; PageCount += 2) {
    SevEsData =
      (SEV_ES_PER_CPU_DATA *)(GhcbBase + EFI_PAGES_TO_SIZE (PageCount));
    SevEsData->GhcbBackupPages = GhcbBackupPages;

    GhcbBackupPages += EFI_PAGE_SIZE * (VMGEXIT_MAXIMUM_VC_COUNT - 1);
  }

  DEBUG ((
    DEBUG_INFO,
    "SEV-ES is enabled, %lu GHCB backup pages allocated starting at 0x%p\n",
    (UINT64)GhcbBackupPageCount,
    GhcbBackupBase
    ));

  //
  // SEV-SNP guest requires that GHCB GPA must be registered before using it.
  //
  if (MemEncryptSevSnpIsEnabled ()) {
    GhcbRegister (GhcbBasePa);
  }

  AsmWriteMsr64 (MSR_SEV_ES_GHCB, GhcbBasePa);

  //
  // Now that the PEI GHCB is set up, the SEC GHCB page is no longer necessary
  // to keep shared. Later, it is exposed to the OS as EfiConventionalMemory, so
  // it needs to be marked private. The size of the region is hardcoded in
  // OvmfPkg/ResetVector/ResetVector.nasmb in the definition of
  // SNP_SEC_MEM_BASE_DESC_2.
  //
  Status = MemEncryptSevSetPageEncMask (
             0,                                  // Cr3 -- use system Cr3
             FixedPcdGet32 (PcdOvmfSecGhcbBase), // BaseAddress
             1                                   // NumPages
             );
  ASSERT_RETURN_ERROR (Status);

  //
  // The SEV support will clear the C-bit from non-RAM areas.  The early GDT
  // lives in a non-RAM area, so when an exception occurs (like a #VC) the GDT
  // will be read as un-encrypted even though it was created before the C-bit
  // was cleared (encrypted). This will result in a failure to be able to
  // handle the exception.
  //
  AsmReadGdtr (&Gdtr);

  Gdt = AllocatePages (EFI_SIZE_TO_PAGES ((UINTN)Gdtr.Limit + 1));
  ASSERT (Gdt != NULL);

  CopyMem (Gdt, (VOID *)Gdtr.Base, Gdtr.Limit + 1);
  Gdtr.Base = (UINTN)Gdt;
  AsmWriteGdtr (&Gdtr);
}

/**

  Function checks if SEV support is available, if present then it sets
  the dynamic PcdPteMemoryEncryptionAddressOrMask with memory encryption mask.

  **/
VOID
AmdSevInitialize (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64         EncryptionMask;
  RETURN_STATUS  PcdStatus;

  //
  // Check if SEV is enabled
  //
  if (!MemEncryptSevIsEnabled ()) {
    return;
  }

  //
  // Check and perform SEV-SNP initialization if required. This need to be
  // done before the GHCB page is made shared in the AmdSevEsInitialize(). This
  // is because the system RAM must be validated before it is made shared.
  // The AmdSevSnpInitialize() validates the system RAM.
  //
  AmdSevSnpInitialize ();

  //
  // Set Memory Encryption Mask PCD
  //
  EncryptionMask = MemEncryptSevGetEncryptionMask ();
  PcdStatus      = PcdSet64S (PcdPteMemoryEncryptionAddressOrMask, EncryptionMask);
  ASSERT_RETURN_ERROR (PcdStatus);

  DEBUG ((DEBUG_INFO, "SEV is enabled (mask 0x%lx)\n", EncryptionMask));

  //
  // Set Pcd to Deny the execution of option ROM when security
  // violation.
  //
  PcdStatus = PcdSet32S (PcdOptionRomImageVerificationPolicy, 0x4);
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // When SMM is required, cover the pages containing the initial SMRAM Save
  // State Map with a memory allocation HOB:
  //
  // There's going to be a time interval between our decrypting those pages for
  // SMBASE relocation and re-encrypting the same pages after SMBASE
  // relocation. We shall ensure that the DXE phase stay away from those pages
  // until after re-encryption, in order to prevent an information leak to the
  // hypervisor.
  //
  if (PlatformInfoHob->SmmSmramRequire && (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME)) {
    RETURN_STATUS  LocateMapStatus;
    UINTN          MapPagesBase;
    UINTN          MapPagesCount;

    LocateMapStatus = MemEncryptSevLocateInitialSmramSaveStateMapPages (
                        &MapPagesBase,
                        &MapPagesCount
                        );
    ASSERT_RETURN_ERROR (LocateMapStatus);

    if (PlatformInfoHob->Q35SmramAtDefaultSmbase) {
      //
      // The initial SMRAM Save State Map has been covered as part of a larger
      // reserved memory allocation in InitializeRamRegions().
      //
      ASSERT (SMM_DEFAULT_SMBASE <= MapPagesBase);
      ASSERT (
        (MapPagesBase + EFI_PAGES_TO_SIZE (MapPagesCount) <=
         SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE)
        );
    } else {
      BuildMemoryAllocationHob (
        MapPagesBase,                      // BaseAddress
        EFI_PAGES_TO_SIZE (MapPagesCount), // Length
        EfiBootServicesData                // MemoryType
        );
    }
  }

  //
  // Check and perform SEV-ES initialization if required.
  //
  AmdSevEsInitialize (PlatformInfoHob);

  //
  // Set the Confidential computing attr PCD to communicate which SEV
  // technology is active.
  //
  if (MemEncryptSevSnpIsEnabled ()) {
    PcdStatus = PcdSet64S (PcdConfidentialComputingGuestAttr, CCAttrAmdSevSnp);
  } else if (MemEncryptSevEsIsEnabled ()) {
    PcdStatus = PcdSet64S (PcdConfidentialComputingGuestAttr, CCAttrAmdSevEs);
  } else {
    PcdStatus = PcdSet64S (PcdConfidentialComputingGuestAttr, CCAttrAmdSev);
  }

  ASSERT_RETURN_ERROR (PcdStatus);
}

/**
 The function performs SEV specific region initialization.

 **/
VOID
SevInitializeRam (
  VOID
  )
{
  if (MemEncryptSevSnpIsEnabled ()) {
    //
    // If SEV-SNP is enabled, reserve the Secrets and CPUID memory area.
    //
    // This memory range is given to the PSP by the hypervisor to populate
    // the information used during the SNP VM boots, and it need to persist
    // across the kexec boots. Mark it as EfiReservedMemoryType so that
    // the guest firmware and OS does not use it as a system memory.
    //
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSnpSecretsBase),
      (UINT64)(UINTN)PcdGet32 (PcdOvmfSnpSecretsSize),
      EfiReservedMemoryType
      );
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfCpuidBase),
      (UINT64)(UINTN)PcdGet32 (PcdOvmfCpuidSize),
      EfiReservedMemoryType
      );
  }
}
