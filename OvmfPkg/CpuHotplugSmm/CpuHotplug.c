/** @file
  Root SMI handler for VCPU hotplug SMIs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <CpuHotPlugData.h>                  // CPU_HOT_PLUG_DATA
#include <IndustryStandard/Q35MchIch9.h>     // ICH9_APM_CNT
#include <IndustryStandard/QemuCpuHotplug.h> // QEMU_CPUHP_CMD_GET_PENDING
#include <Library/BaseLib.h>                 // CpuDeadLoop()
#include <Library/CpuLib.h>                  // CpuSleep()
#include <Library/DebugLib.h>                // ASSERT()
#include <Library/MmServicesTableLib.h>      // gMmst
#include <Library/PcdLib.h>                  // PcdGetBool()
#include <Library/SafeIntLib.h>              // SafeUintnSub()
#include <Pcd/CpuHotEjectData.h>             // CPU_HOT_EJECT_DATA
#include <Protocol/MmCpuIo.h>                // EFI_MM_CPU_IO_PROTOCOL
#include <Protocol/SmmCpuService.h>          // EFI_SMM_CPU_SERVICE_PROTOCOL
#include <Register/Intel/ArchitecturalMsr.h> // MSR_IA32_APIC_BASE_REGISTER
#include <Uefi/UefiBaseType.h>               // EFI_STATUS

#include "ApicId.h"                          // APIC_ID
#include "QemuCpuhp.h"                       // QemuCpuhpWriteCpuSelector()
#include "Smbase.h"                          // SmbaseAllocatePostSmmPen()

//
// We use this protocol for accessing IO Ports.
//
STATIC EFI_MM_CPU_IO_PROTOCOL  *mMmCpuIo;
//
// The following protocol is used to report the addition or removal of a CPU to
// the SMM CPU driver (PiSmmCpuDxeSmm).
//
STATIC EFI_SMM_CPU_SERVICE_PROTOCOL  *mMmCpuService;
//
// These structures serve as communication side-channels between the
// EFI_SMM_CPU_SERVICE_PROTOCOL consumer (i.e., this driver) and provider
// (i.e., PiSmmCpuDxeSmm).
//
STATIC CPU_HOT_PLUG_DATA   *mCpuHotPlugData;
STATIC CPU_HOT_EJECT_DATA  *mCpuHotEjectData;
//
// SMRAM arrays for fetching the APIC IDs of processors with pending events (of
// known event types), for the time of just one MMI.
//
// The lifetimes of these arrays match that of this driver only because we
// don't want to allocate SMRAM at OS runtime, and potentially fail (or
// fragment the SMRAM map).
//
// The first array stores APIC IDs for hot-plug events, the second and the
// third store APIC IDs and QEMU CPU Selectors (both indexed similarly) for
// hot-unplug events. All of these provide room for "possible CPU count" minus
// one elements as we don't expect every possible CPU to appear, or disappear,
// in a single MMI. The numbers of used (populated) elements in the arrays are
// determined on every MMI separately.
//
STATIC APIC_ID  *mPluggedApicIds;
STATIC APIC_ID  *mToUnplugApicIds;
STATIC UINT32   *mToUnplugSelectors;
//
// Address of the non-SMRAM reserved memory page that contains the Post-SMM Pen
// for hot-added CPUs.
//
STATIC UINT32  mPostSmmPenAddress;
//
// Represents the registration of the CPU Hotplug MMI handler.
//
STATIC EFI_HANDLE  mDispatchHandle;

/**
  Process CPUs that have been hot-added, per QemuCpuhpCollectApicIds().

  For each such CPU, relocate the SMBASE, and report the CPU to PiSmmCpuDxeSmm
  via EFI_SMM_CPU_SERVICE_PROTOCOL. If the supposedly hot-added CPU is already
  known, skip it silently.

  @param[in] PluggedApicIds    The APIC IDs of the CPUs that have been
                               hot-plugged.

  @param[in] PluggedCount      The number of filled-in APIC IDs in
                               PluggedApicIds.

  @retval EFI_SUCCESS          CPUs corresponding to all the APIC IDs are
                               populated.

  @retval EFI_OUT_OF_RESOURCES Out of APIC ID space in "mCpuHotPlugData".

  @return                      Error codes propagated from SmbaseRelocate()
                               and mMmCpuService->AddProcessor().
**/
STATIC
EFI_STATUS
ProcessHotAddedCpus (
  IN APIC_ID  *PluggedApicIds,
  IN UINT32   PluggedCount
  )
{
  EFI_STATUS  Status;
  UINT32      PluggedIdx;
  UINT32      NewSlot;

  //
  // The Post-SMM Pen need not be reinstalled multiple times within a single
  // root MMI handling. Even reinstalling once per root MMI is only prudence;
  // in theory installing the pen in the driver's entry point function should
  // suffice.
  //
  SmbaseReinstallPostSmmPen (mPostSmmPenAddress);

  PluggedIdx = 0;
  NewSlot    = 0;
  while (PluggedIdx < PluggedCount) {
    APIC_ID  NewApicId;
    UINT32   CheckSlot;
    UINTN    NewProcessorNumberByProtocol;

    NewApicId = PluggedApicIds[PluggedIdx];

    //
    // Check if the supposedly hot-added CPU is already known to us.
    //
    for (CheckSlot = 0;
         CheckSlot < mCpuHotPlugData->ArrayLength;
         CheckSlot++)
    {
      if (mCpuHotPlugData->ApicId[CheckSlot] == NewApicId) {
        break;
      }
    }

    if (CheckSlot < mCpuHotPlugData->ArrayLength) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: APIC ID " FMT_APIC_ID " was hot-plugged "
                                   "before; ignoring it\n",
        __func__,
        NewApicId
        ));
      PluggedIdx++;
      continue;
    }

    //
    // Find the first empty slot in CPU_HOT_PLUG_DATA.
    //
    while (NewSlot < mCpuHotPlugData->ArrayLength &&
           mCpuHotPlugData->ApicId[NewSlot] != MAX_UINT64)
    {
      NewSlot++;
    }

    if (NewSlot == mCpuHotPlugData->ArrayLength) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: no room for APIC ID " FMT_APIC_ID "\n",
        __func__,
        NewApicId
        ));
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Store the APIC ID of the new processor to the slot.
    //
    mCpuHotPlugData->ApicId[NewSlot] = NewApicId;

    //
    // Relocate the SMBASE of the new CPU.
    //
    Status = SmbaseRelocate (
               NewApicId,
               mCpuHotPlugData->SmBase[NewSlot],
               mPostSmmPenAddress
               );
    if (EFI_ERROR (Status)) {
      goto RevokeNewSlot;
    }

    //
    // Add the new CPU with EFI_SMM_CPU_SERVICE_PROTOCOL.
    //
    Status = mMmCpuService->AddProcessor (
                              mMmCpuService,
                              NewApicId,
                              &NewProcessorNumberByProtocol
                              );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: AddProcessor(" FMT_APIC_ID "): %r\n",
        __func__,
        NewApicId,
        Status
        ));
      goto RevokeNewSlot;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: hot-added APIC ID " FMT_APIC_ID ", SMBASE 0x%Lx, "
                                           "EFI_SMM_CPU_SERVICE_PROTOCOL assigned number %Lu\n",
      __func__,
      NewApicId,
      (UINT64)mCpuHotPlugData->SmBase[NewSlot],
      (UINT64)NewProcessorNumberByProtocol
      ));

    NewSlot++;
    PluggedIdx++;
  }

  //
  // We've processed this batch of hot-added CPUs.
  //
  return EFI_SUCCESS;

RevokeNewSlot:
  mCpuHotPlugData->ApicId[NewSlot] = MAX_UINT64;

  return Status;
}

/**
  EjectCpu needs to know the BSP at SMI exit at a point when
  some of the EFI_SMM_CPU_SERVICE_PROTOCOL state has been torn
  down.
  Reuse the logic from OvmfPkg::PlatformSmmBspElection() to
  do that.

  @retval TRUE   If the CPU executing this function is the BSP.

  @retval FALSE  If the CPU executing this function is an AP.
**/
STATIC
BOOLEAN
CheckIfBsp (
  VOID
  )
{
  MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;
  BOOLEAN                      IsBsp;

  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);
  IsBsp              = (BOOLEAN)(ApicBaseMsr.Bits.BSP == 1);
  return IsBsp;
}

/**
  CPU Hot-eject handler, called from SmmCpuFeaturesRendezvousExit()
  on each CPU at exit from SMM.

  If, the executing CPU is neither the BSP, nor being ejected, nothing
  to be done.
  If, the executing CPU is being ejected, wait in a halted loop
  until ejected.
  If, the executing CPU is the BSP, set QEMU CPU status to eject
  for CPUs being ejected.

  @param[in] ProcessorNum      ProcessorNum denotes the CPU exiting SMM,
                               and will be used as an index into
                               CPU_HOT_EJECT_DATA->QemuSelectorMap. It is
                               identical to the processor handle number in
                               EFI_SMM_CPU_SERVICE_PROTOCOL.
**/
VOID
EFIAPI
EjectCpu (
  IN UINTN  ProcessorNum
  )
{
  UINT64  QemuSelector;

  if (CheckIfBsp ()) {
    UINT32  Idx;

    for (Idx = 0; Idx < mCpuHotEjectData->ArrayLength; Idx++) {
      QemuSelector = mCpuHotEjectData->QemuSelectorMap[Idx];

      if (QemuSelector != CPU_EJECT_QEMU_SELECTOR_INVALID) {
        //
        // This to-be-ejected-CPU has already received the BSP's SMI exit
        // signal and will execute SmmCpuFeaturesRendezvousExit()
        // followed by this callback or is already penned in the
        // CpuSleep() loop below.
        //
        // Tell QEMU to context-switch it out.
        //
        QemuCpuhpWriteCpuSelector (mMmCpuIo, (UINT32)QemuSelector);
        QemuCpuhpWriteCpuStatus (mMmCpuIo, QEMU_CPUHP_STAT_EJECT);

        //
        // Now that we've ejected the CPU corresponding to QemuSelectorMap[Idx],
        // clear its eject status to ensure that an invalid future SMI does
        // not end up trying a spurious eject or a newly hotplugged CPU does
        // not get penned in the CpuSleep() loop.
        //
        // Note that the QemuCpuhpWriteCpuStatus() command above is a write to
        // a different address space and uses the EFI_MM_CPU_IO_PROTOCOL.
        //
        // This means that we are guaranteed that the following assignment
        // will not be reordered before the eject. And, so we can safely
        // do this write here.
        //
        mCpuHotEjectData->QemuSelectorMap[Idx] =
          CPU_EJECT_QEMU_SELECTOR_INVALID;

        DEBUG ((
          DEBUG_INFO,
          "%a: Unplugged ProcessorNum %u, "
          "QemuSelector %Lu\n",
          __func__,
          Idx,
          QemuSelector
          ));
      }
    }

    //
    // We are done until the next hot-unplug; clear the handler.
    //
    // mCpuHotEjectData->Handler is a NOP for any CPU not under ejection.
    // So, once we are done with all the ejections, we can safely reset it
    // here since any CPU dereferencing it would only see either the old
    // or the new value (since it is aligned at a natural boundary.)
    //
    mCpuHotEjectData->Handler = NULL;
    return;
  }

  //
  // Reached only on APs
  //

  //
  // mCpuHotEjectData->QemuSelectorMap[ProcessorNum] is updated
  // on the BSP in the ongoing SMI at two places:
  //
  // - UnplugCpus() where the BSP determines if a CPU is under ejection
  //   or not. As a comment in UnplugCpus() at set-up, and in
  //   SmmCpuFeaturesRendezvousExit() where it is dereferenced describe,
  //   any such updates are guaranteed to be ordered-before the
  //   dereference below.
  //
  // - EjectCpu() on the BSP (above) updates QemuSelectorMap[ProcessorNum]
  //   for a CPU once it's ejected.
  //
  //   The CPU under ejection: might be executing anywhere between the
  //   AllCpusInSync loop in SmiRendezvous(), to about to dereference
  //   QemuSelectorMap[ProcessorNum].
  //   As described in the comment above where we do the reset, this
  //   is not a problem since the ejected CPU never sees the after value.
  //   CPUs not-under ejection: never see any changes so they are fine.
  //
  QemuSelector = mCpuHotEjectData->QemuSelectorMap[ProcessorNum];
  if (QemuSelector == CPU_EJECT_QEMU_SELECTOR_INVALID) {
    /* wait until BSP is done */
    while (mCpuHotEjectData->Handler != NULL) {
      CpuPause ();
    }

    return;
  }

  //
  // APs being unplugged get here from SmmCpuFeaturesRendezvousExit()
  // after having been cleared to exit the SMI and so have no SMM
  // processing remaining.
  //
  // Keep them penned here until the BSP tells QEMU to eject them.
  //
  for ( ; ;) {
    DisableInterrupts ();
    CpuSleep ();
  }
}

/**
  Process to be hot-unplugged CPUs, per QemuCpuhpCollectApicIds().

  For each such CPU, report the CPU to PiSmmCpuDxeSmm via
  EFI_SMM_CPU_SERVICE_PROTOCOL and stash the QEMU Cpu Selectors for later
  ejection. If the to be hot-unplugged CPU is unknown, skip it silently.

  Additonally, if we do stash any Cpu Selectors, also install a CPU eject
  handler which would handle the ejection.

  @param[in] ToUnplugApicIds    The APIC IDs of the CPUs that are about to be
                                hot-unplugged.

  @param[in] ToUnplugSelectors  The QEMU Selectors of the CPUs that are about to
                                be hot-unplugged.

  @param[in] ToUnplugCount      The number of filled-in APIC IDs in
                                ToUnplugApicIds.

  @retval EFI_ALREADY_STARTED   For the ProcessorNum that
                                EFI_SMM_CPU_SERVICE_PROTOCOL had assigned to
                                one of the APIC IDs in ToUnplugApicIds,
                                mCpuHotEjectData->QemuSelectorMap already has
                                the QemuSelector value stashed. (This should
                                never happen.)

  @retval EFI_SUCCESS           Known APIC IDs have been removed from SMM data
                                structures.

  @return                       Error codes propagated from
                                mMmCpuService->RemoveProcessor().
**/
STATIC
EFI_STATUS
UnplugCpus (
  IN APIC_ID  *ToUnplugApicIds,
  IN UINT32   *ToUnplugSelectors,
  IN UINT32   ToUnplugCount
  )
{
  EFI_STATUS  Status;
  UINT32      ToUnplugIdx;
  UINT32      EjectCount;
  UINTN       ProcessorNum;

  ToUnplugIdx = 0;
  EjectCount  = 0;
  while (ToUnplugIdx < ToUnplugCount) {
    APIC_ID  RemoveApicId;
    UINT32   QemuSelector;

    RemoveApicId = ToUnplugApicIds[ToUnplugIdx];
    QemuSelector = ToUnplugSelectors[ToUnplugIdx];

    //
    // mCpuHotPlugData->ApicId maps ProcessorNum -> ApicId. Use RemoveApicId
    // to find the corresponding ProcessorNum for the CPU to be removed.
    //
    // With this we can establish a 3 way mapping:
    //    APIC_ID -- ProcessorNum -- QemuSelector
    //
    // We stash the ProcessorNum -> QemuSelector mapping so it can later be
    // used for CPU hot-eject in SmmCpuFeaturesRendezvousExit() context (where
    // we only have ProcessorNum available.)
    //

    for (ProcessorNum = 0;
         ProcessorNum < mCpuHotPlugData->ArrayLength;
         ProcessorNum++)
    {
      if (mCpuHotPlugData->ApicId[ProcessorNum] == RemoveApicId) {
        break;
      }
    }

    //
    // Ignore the unplug if APIC ID not found
    //
    if (ProcessorNum == mCpuHotPlugData->ArrayLength) {
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: did not find APIC ID " FMT_APIC_ID
        " to unplug\n",
        __func__,
        RemoveApicId
        ));
      ToUnplugIdx++;
      continue;
    }

    //
    // Mark ProcessorNum for removal from SMM data structures
    //
    Status = mMmCpuService->RemoveProcessor (mMmCpuService, ProcessorNum);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: RemoveProcessor(" FMT_APIC_ID "): %r\n",
        __func__,
        RemoveApicId,
        Status
        ));
      return Status;
    }

    if (mCpuHotEjectData->QemuSelectorMap[ProcessorNum] !=
        CPU_EJECT_QEMU_SELECTOR_INVALID)
    {
      //
      // mCpuHotEjectData->QemuSelectorMap[ProcessorNum] is set to
      // CPU_EJECT_QEMU_SELECTOR_INVALID when mCpuHotEjectData->QemuSelectorMap
      // is allocated, and once the subject processsor is ejected.
      //
      // Additionally, mMmCpuService->RemoveProcessor(ProcessorNum) invalidates
      // mCpuHotPlugData->ApicId[ProcessorNum], so a given ProcessorNum can
      // never match more than one APIC ID -- nor, by transitivity, designate
      // more than one QemuSelector -- in a single invocation of UnplugCpus().
      //
      DEBUG ((
        DEBUG_ERROR,
        "%a: ProcessorNum %Lu maps to QemuSelector %Lu, "
        "cannot also map to %u\n",
        __func__,
        (UINT64)ProcessorNum,
        mCpuHotEjectData->QemuSelectorMap[ProcessorNum],
        QemuSelector
        ));

      return EFI_ALREADY_STARTED;
    }

    //
    // Stash the QemuSelector so we can do the actual ejection later.
    //
    mCpuHotEjectData->QemuSelectorMap[ProcessorNum] = (UINT64)QemuSelector;

    DEBUG ((
      DEBUG_INFO,
      "%a: Started hot-unplug on ProcessorNum %Lu, APIC ID "
      FMT_APIC_ID ", QemuSelector %u\n",
      __func__,
      (UINT64)ProcessorNum,
      RemoveApicId,
      QemuSelector
      ));

    EjectCount++;
    ToUnplugIdx++;
  }

  if (EjectCount != 0) {
    //
    // We have processors to be ejected; install the handler.
    //
    mCpuHotEjectData->Handler = EjectCpu;

    //
    // The BSP and APs load mCpuHotEjectData->Handler, and
    // mCpuHotEjectData->QemuSelectorMap[] in SmmCpuFeaturesRendezvousExit()
    // and EjectCpu().
    //
    // The comment in SmmCpuFeaturesRendezvousExit() details how we use
    // the AllCpusInSync control-dependency to ensure that any loads are
    // ordered-after the stores above.
    //
    // Ensure that the stores above are ordered-before the AllCpusInSync store
    // by using a MemoryFence() with release semantics.
    //
    MemoryFence ();
  }

  //
  // We've removed this set of APIC IDs from SMM data structures and
  // have installed an ejection handler if needed.
  //
  return EFI_SUCCESS;
}

/**
  CPU Hotplug MMI handler function.

  This is a root MMI handler.

  @param[in] DispatchHandle      The unique handle assigned to this handler by
                                 EFI_MM_SYSTEM_TABLE.MmiHandlerRegister().

  @param[in] Context             Context passed in by
                                 EFI_MM_SYSTEM_TABLE.MmiManage(). Due to
                                 CpuHotplugMmi() being a root MMI handler,
                                 Context is ASSERT()ed to be NULL.

  @param[in,out] CommBuffer      Ignored, due to CpuHotplugMmi() being a root
                                 MMI handler.

  @param[in,out] CommBufferSize  Ignored, due to CpuHotplugMmi() being a root
                                 MMI handler.

  @retval EFI_SUCCESS                       The MMI was handled and the MMI
                                            source was quiesced. When returned
                                            by a non-root MMI handler,
                                            EFI_SUCCESS terminates the
                                            processing of MMI handlers in
                                            EFI_MM_SYSTEM_TABLE.MmiManage().
                                            For a root MMI handler (i.e., for
                                            the present function too),
                                            EFI_SUCCESS behaves identically to
                                            EFI_WARN_INTERRUPT_SOURCE_QUIESCED,
                                            as further root MMI handlers are
                                            going to be called by
                                            EFI_MM_SYSTEM_TABLE.MmiManage()
                                            anyway.

  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The MMI source has been quiesced,
                                              but other handlers should still
                                              be called.

  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The MMI source is still pending,
                                              and other handlers should still
                                              be called.

  @retval EFI_INTERRUPT_PENDING               The MMI source could not be
                                              quiesced.
**/
STATIC
EFI_STATUS
EFIAPI
CpuHotplugMmi (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context        OPTIONAL,
  IN OUT VOID    *CommBuffer     OPTIONAL,
  IN OUT UINTN   *CommBufferSize OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT8       ApmControl;
  UINT32      PluggedCount;
  UINT32      ToUnplugCount;

  //
  // Assert that we are entering this function due to our root MMI handler
  // registration.
  //
  ASSERT (DispatchHandle == mDispatchHandle);
  //
  // When MmiManage() is invoked to process root MMI handlers, the caller (the
  // MM Core) is expected to pass in a NULL Context. MmiManage() then passes
  // the same NULL Context to individual handlers.
  //
  ASSERT (Context == NULL);
  //
  // Read the MMI command value from the APM Control Port, to see if this is an
  // MMI we should care about.
  //
  Status = mMmCpuIo->Io.Read (
                          mMmCpuIo,
                          MM_IO_UINT8,
                          ICH9_APM_CNT,
                          1,
                          &ApmControl
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to read ICH9_APM_CNT: %r\n",
      __func__,
      Status
      ));
    //
    // We couldn't even determine if the MMI was for us or not.
    //
    goto Fatal;
  }

  if (ApmControl != ICH9_APM_CNT_CPU_HOTPLUG) {
    //
    // The MMI is not for us.
    //
    return EFI_WARN_INTERRUPT_SOURCE_QUIESCED;
  }

  //
  // Collect the CPUs with pending events.
  //
  Status = QemuCpuhpCollectApicIds (
             mMmCpuIo,
             mCpuHotPlugData->ArrayLength,     // PossibleCpuCount
             mCpuHotPlugData->ArrayLength - 1, // ApicIdCount
             mPluggedApicIds,
             &PluggedCount,
             mToUnplugApicIds,
             mToUnplugSelectors,
             &ToUnplugCount
             );
  if (EFI_ERROR (Status)) {
    goto Fatal;
  }

  if (PluggedCount > 0) {
    Status = ProcessHotAddedCpus (mPluggedApicIds, PluggedCount);
    if (EFI_ERROR (Status)) {
      goto Fatal;
    }
  }

  if (ToUnplugCount > 0) {
    Status = UnplugCpus (mToUnplugApicIds, mToUnplugSelectors, ToUnplugCount);
    if (EFI_ERROR (Status)) {
      goto Fatal;
    }
  }

  //
  // We've handled this MMI.
  //
  return EFI_SUCCESS;

Fatal:
  ASSERT (FALSE);
  CpuDeadLoop ();
  //
  // We couldn't handle this MMI.
  //
  return EFI_INTERRUPT_PENDING;
}

//
// Entry point function of this driver.
//
EFI_STATUS
EFIAPI
CpuHotplugEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Len;
  UINTN       Size;
  UINTN       SizeSel;

  //
  // This module should only be included when SMM support is required.
  //
  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));
  //
  // This driver depends on the dynamically detected "SMRAM at default SMBASE"
  // feature.
  //
  if (!PcdGetBool (PcdQ35SmramAtDefaultSmbase)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Errors from here on are fatal; we cannot allow the boot to proceed if we
  // can't set up this driver to handle CPU hotplug.
  //
  // First, collect the protocols needed later. All of these protocols are
  // listed in our module DEPEX.
  //
  Status = gMmst->MmLocateProtocol (
                    &gEfiMmCpuIoProtocolGuid,
                    NULL /* Registration */,
                    (VOID **)&mMmCpuIo
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: locate MmCpuIo: %r\n", __func__, Status));
    goto Fatal;
  }

  Status = gMmst->MmLocateProtocol (
                    &gEfiSmmCpuServiceProtocolGuid,
                    NULL /* Registration */,
                    (VOID **)&mMmCpuService
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: locate MmCpuService: %r\n",
      __func__,
      Status
      ));
    goto Fatal;
  }

  //
  // Our DEPEX on EFI_SMM_CPU_SERVICE_PROTOCOL guarantees that PiSmmCpuDxeSmm
  // has pointed:
  // - PcdCpuHotPlugDataAddress to CPU_HOT_PLUG_DATA in SMRAM,
  // - PcdCpuHotEjectDataAddress to CPU_HOT_EJECT_DATA in SMRAM, if the
  //   possible CPU count is greater than 1.
  //
  mCpuHotPlugData  = (VOID *)(UINTN)PcdGet64 (PcdCpuHotPlugDataAddress);
  mCpuHotEjectData = (VOID *)(UINTN)PcdGet64 (PcdCpuHotEjectDataAddress);

  if (mCpuHotPlugData == NULL) {
    Status = EFI_NOT_FOUND;
    DEBUG ((DEBUG_ERROR, "%a: CPU_HOT_PLUG_DATA: %r\n", __func__, Status));
    goto Fatal;
  }

  //
  // If the possible CPU count is 1, there's nothing for this driver to do.
  //
  if (mCpuHotPlugData->ArrayLength == 1) {
    return EFI_UNSUPPORTED;
  }

  if (mCpuHotEjectData == NULL) {
    Status = EFI_NOT_FOUND;
  } else if (mCpuHotPlugData->ArrayLength != mCpuHotEjectData->ArrayLength) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CPU_HOT_EJECT_DATA: %r\n", __func__, Status));
    goto Fatal;
  }

  //
  // Allocate the data structures that depend on the possible CPU count.
  //
  if (RETURN_ERROR (SafeUintnSub (mCpuHotPlugData->ArrayLength, 1, &Len)) ||
      RETURN_ERROR (SafeUintnMult (sizeof (APIC_ID), Len, &Size)) ||
      RETURN_ERROR (SafeUintnMult (sizeof (UINT32), Len, &SizeSel)))
  {
    Status = EFI_ABORTED;
    DEBUG ((DEBUG_ERROR, "%a: invalid CPU_HOT_PLUG_DATA\n", __func__));
    goto Fatal;
  }

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    Size,
                    (VOID **)&mPluggedApicIds
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmAllocatePool(): %r\n", __func__, Status));
    goto Fatal;
  }

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    Size,
                    (VOID **)&mToUnplugApicIds
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmAllocatePool(): %r\n", __func__, Status));
    goto ReleasePluggedApicIds;
  }

  Status = gMmst->MmAllocatePool (
                    EfiRuntimeServicesData,
                    SizeSel,
                    (VOID **)&mToUnplugSelectors
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmAllocatePool(): %r\n", __func__, Status));
    goto ReleaseToUnplugApicIds;
  }

  //
  // Allocate the Post-SMM Pen for hot-added CPUs.
  //
  Status = SmbaseAllocatePostSmmPen (
             &mPostSmmPenAddress,
             SystemTable->BootServices
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseToUnplugSelectors;
  }

  //
  // Sanity-check the CPU hotplug interface.
  //
  // Both of the following features are part of QEMU 5.0, introduced primarily
  // in commit range 3e08b2b9cb64..3a61c8db9d25:
  //
  // (a) the QEMU_CPUHP_CMD_GET_ARCH_ID command of the modern CPU hotplug
  //     interface,
  //
  // (b) the "SMRAM at default SMBASE" feature.
  //
  // From these, (b) is restricted to 5.0+ machine type versions, while (a)
  // does not depend on machine type version. Because we ensured the stricter
  // condition (b) through PcdQ35SmramAtDefaultSmbase above, the (a)
  // QEMU_CPUHP_CMD_GET_ARCH_ID command must now be available too. While we
  // can't verify the presence of precisely that command, we can still verify
  // (sanity-check) that the modern interface is active, at least.
  //
  // Consult the "Typical usecases | Detecting and enabling modern CPU hotplug
  // interface" section in QEMU's "docs/specs/acpi_cpu_hotplug.txt", on the
  // following.
  //
  QemuCpuhpWriteCpuSelector (mMmCpuIo, 0);
  QemuCpuhpWriteCpuSelector (mMmCpuIo, 0);
  QemuCpuhpWriteCommand (mMmCpuIo, QEMU_CPUHP_CMD_GET_PENDING);
  if (QemuCpuhpReadCommandData2 (mMmCpuIo) != 0) {
    Status = EFI_NOT_FOUND;
    DEBUG ((
      DEBUG_ERROR,
      "%a: modern CPU hotplug interface: %r\n",
      __func__,
      Status
      ));
    goto ReleasePostSmmPen;
  }

  //
  // Register the handler for the CPU Hotplug MMI.
  //
  Status = gMmst->MmiHandlerRegister (
                    CpuHotplugMmi,
                    NULL,            // HandlerType: root MMI handler
                    &mDispatchHandle
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MmiHandlerRegister(): %r\n",
      __func__,
      Status
      ));
    goto ReleasePostSmmPen;
  }

  //
  // Install the handler for the hot-added CPUs' first SMI.
  //
  SmbaseInstallFirstSmiHandler ();

  return EFI_SUCCESS;

ReleasePostSmmPen:
  SmbaseReleasePostSmmPen (mPostSmmPenAddress, SystemTable->BootServices);
  mPostSmmPenAddress = 0;

ReleaseToUnplugSelectors:
  gMmst->MmFreePool (mToUnplugSelectors);
  mToUnplugSelectors = NULL;

ReleaseToUnplugApicIds:
  gMmst->MmFreePool (mToUnplugApicIds);
  mToUnplugApicIds = NULL;

ReleasePluggedApicIds:
  gMmst->MmFreePool (mPluggedApicIds);
  mPluggedApicIds = NULL;

Fatal:
  ASSERT (FALSE);
  CpuDeadLoop ();
  return Status;
}
