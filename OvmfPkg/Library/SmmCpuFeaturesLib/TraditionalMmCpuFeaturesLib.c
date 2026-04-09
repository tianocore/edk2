/** @file
  The CPU specific programming for PiSmmCpuDxeSmm module.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Q35MchIch9.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/SmmCpuFeaturesLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Pcd/CpuHotEjectData.h>
#include <PiSmm.h>

STATIC CPU_HOT_EJECT_DATA  *mCpuHotEjectData = NULL;

/**
  The common constructor function

  @retval EFI_SUCCESS      The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
MmCpuFeaturesLibConstructorCommon (
  VOID
  );

/**
  Initialize mCpuHotEjectData if PcdCpuMaxLogicalProcessorNumber > 1.

  Also setup the corresponding PcdCpuHotEjectDataAddress.
**/
STATIC
VOID
InitCpuHotEjectData (
  VOID
  )
{
  UINTN          Size;
  UINT32         Idx;
  UINT32         MaxNumberOfCpus;
  RETURN_STATUS  PcdStatus;

  MaxNumberOfCpus = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
  if (MaxNumberOfCpus == 1) {
    return;
  }

  //
  // We allocate CPU_HOT_EJECT_DATA and CPU_HOT_EJECT_DATA->QemuSelectorMap[]
  // in a single allocation, and explicitly align the QemuSelectorMap[] (which
  // is a UINT64 array) at its natural boundary.
  // Accordingly, allocate:
  //   sizeof(*mCpuHotEjectData) + (MaxNumberOfCpus * sizeof(UINT64))
  // and, add sizeof(UINT64) - 1 to use as padding if needed.
  //

  if (RETURN_ERROR (SafeUintnMult (MaxNumberOfCpus, sizeof (UINT64), &Size)) ||
      RETURN_ERROR (SafeUintnAdd (Size, sizeof (*mCpuHotEjectData), &Size)) ||
      RETURN_ERROR (SafeUintnAdd (Size, sizeof (UINT64) - 1, &Size)))
  {
    DEBUG ((DEBUG_ERROR, "%a: invalid CPU_HOT_EJECT_DATA\n", __func__));
    goto Fatal;
  }

  mCpuHotEjectData = AllocatePool (Size);
  if (mCpuHotEjectData == NULL) {
    ASSERT (mCpuHotEjectData != NULL);
    goto Fatal;
  }

  mCpuHotEjectData->Handler     = NULL;
  mCpuHotEjectData->ArrayLength = MaxNumberOfCpus;

  mCpuHotEjectData->QemuSelectorMap = ALIGN_POINTER (
                                        mCpuHotEjectData + 1,
                                        sizeof (UINT64)
                                        );
  //
  // We use mCpuHotEjectData->QemuSelectorMap to map
  // ProcessorNum -> QemuSelector. Initialize to invalid values.
  //
  for (Idx = 0; Idx < mCpuHotEjectData->ArrayLength; Idx++) {
    mCpuHotEjectData->QemuSelectorMap[Idx] = CPU_EJECT_QEMU_SELECTOR_INVALID;
  }

  //
  // Expose address of CPU Hot eject Data structure
  //
  PcdStatus = PcdSet64S (
                PcdCpuHotEjectDataAddress,
                (UINTN)(VOID *)mCpuHotEjectData
                );
  ASSERT_RETURN_ERROR (PcdStatus);

  return;

Fatal:
  CpuDeadLoop ();
}

/**
  Hook point in normal execution mode that allows the one CPU that was elected
  as monarch during System Management Mode initialization to perform additional
  initialization actions immediately after all of the CPUs have processed their
  first SMI and called SmmCpuFeaturesInitializeProcessor() relocating SMBASE
  into a buffer in SMRAM and called SmmCpuFeaturesHookReturnFromSmm().
**/
VOID
EFIAPI
SmmCpuFeaturesSmmRelocationComplete (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       MapPagesBase;
  UINTN       MapPagesCount;

  InitCpuHotEjectData ();

  if (!MemEncryptSevIsEnabled ()) {
    return;
  }

  //
  // Now that SMBASE relocation is complete, re-encrypt the original SMRAM save
  // state map's container pages, and release the pages to DXE. (The pages were
  // allocated in PlatformPei.)
  //
  Status = MemEncryptSevLocateInitialSmramSaveStateMapPages (
             &MapPagesBase,
             &MapPagesCount
             );
  ASSERT_EFI_ERROR (Status);

  Status = MemEncryptSevSetPageEncMask (
             0,              // Cr3BaseAddress -- use current CR3
             MapPagesBase,   // BaseAddress
             MapPagesCount   // NumPages
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MemEncryptSevSetPageEncMask(): %r\n",
      __func__,
      Status
      ));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  ZeroMem ((VOID *)MapPagesBase, EFI_PAGES_TO_SIZE (MapPagesCount));

  if (PcdGetBool (PcdQ35SmramAtDefaultSmbase)) {
    //
    // The initial SMRAM Save State Map has been covered as part of a larger
    // reserved memory allocation in PlatformPei's InitializeRamRegions(). That
    // allocation is supposed to survive into OS runtime; we must not release
    // any part of it. Only re-assert the containment here.
    //
    ASSERT (SMM_DEFAULT_SMBASE <= MapPagesBase);
    ASSERT (
      (MapPagesBase + EFI_PAGES_TO_SIZE (MapPagesCount) <=
       SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE)
      );
  } else {
    Status = gBS->FreePages (MapPagesBase, MapPagesCount);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Processor specific hook point each time a CPU exits System Management Mode.

  @param[in] CpuIndex  The index of the CPU that is exiting SMM.  The value
                       must be between 0 and the NumberOfCpus field in the
                       System Management System Table (SMST).
**/
VOID
EFIAPI
SmmCpuFeaturesRendezvousExit (
  IN UINTN  CpuIndex
  )
{
  //
  // We only call the Handler if CPU hot-eject is enabled
  // (PcdCpuMaxLogicalProcessorNumber > 1), and hot-eject is needed
  // in this SMI exit (otherwise mCpuHotEjectData->Handler is not armed.)
  //

  if (mCpuHotEjectData != NULL) {
    CPU_HOT_EJECT_HANDLER  Handler;

    //
    // As the comment above mentions, mCpuHotEjectData->Handler might be
    // written to on the BSP as part of handling of the CPU-ejection.
    //
    // We know that any initial assignment to mCpuHotEjectData->Handler
    // (on the BSP, in the CpuHotplugMmi() context) is ordered-before the
    // load below, since it is guaranteed to happen before the
    // control-dependency of the BSP's SMI exit signal -- by way of a store
    // to AllCpusInSync (on the BSP, in BspHandler()) and the corresponding
    // AllCpusInSync loop (on the APs, in SmiRendezvous()) which depends on
    // that store.
    //
    // This guarantees that these pieces of code can never execute
    // simultaneously. In addition, we ensure that the following load is
    // ordered-after the AllCpusInSync loop by using a MemoryFence() with
    // acquire semantics.
    //
    MemoryFence ();

    Handler = mCpuHotEjectData->Handler;

    if (Handler != NULL) {
      Handler (CpuIndex);
    }
  }
}

/**
  The constructor function

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCpuFeaturesLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return MmCpuFeaturesLibConstructorCommon ();
}
