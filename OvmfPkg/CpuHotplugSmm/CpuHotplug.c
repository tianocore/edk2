/** @file
  Root SMI handler for VCPU hotplug SMIs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <CpuHotPlugData.h>                  // CPU_HOT_PLUG_DATA
#include <IndustryStandard/Q35MchIch9.h>     // ICH9_APM_CNT
#include <IndustryStandard/QemuCpuHotplug.h> // QEMU_CPUHP_CMD_GET_PENDING
#include <Library/BaseLib.h>                 // CpuDeadLoop()
#include <Library/DebugLib.h>                // ASSERT()
#include <Library/MmServicesTableLib.h>      // gMmst
#include <Library/PcdLib.h>                  // PcdGetBool()
#include <Library/SafeIntLib.h>              // SafeUintnSub()
#include <Protocol/MmCpuIo.h>                // EFI_MM_CPU_IO_PROTOCOL
#include <Protocol/SmmCpuService.h>          // EFI_SMM_CPU_SERVICE_PROTOCOL
#include <Uefi/UefiBaseType.h>               // EFI_STATUS

#include "ApicId.h"                          // APIC_ID
#include "QemuCpuhp.h"                       // QemuCpuhpWriteCpuSelector()
#include "Smbase.h"                          // SmbaseAllocatePostSmmPen()

//
// We use this protocol for accessing IO Ports.
//
STATIC EFI_MM_CPU_IO_PROTOCOL *mMmCpuIo;
//
// The following protocol is used to report the addition or removal of a CPU to
// the SMM CPU driver (PiSmmCpuDxeSmm).
//
STATIC EFI_SMM_CPU_SERVICE_PROTOCOL *mMmCpuService;
//
// This structure is a communication side-channel between the
// EFI_SMM_CPU_SERVICE_PROTOCOL consumer (i.e., this driver) and provider
// (i.e., PiSmmCpuDxeSmm).
//
STATIC CPU_HOT_PLUG_DATA *mCpuHotPlugData;
//
// SMRAM arrays for fetching the APIC IDs of processors with pending events (of
// known event types), for the time of just one MMI.
//
// The lifetimes of these arrays match that of this driver only because we
// don't want to allocate SMRAM at OS runtime, and potentially fail (or
// fragment the SMRAM map).
//
// These arrays provide room for ("possible CPU count" minus one) APIC IDs
// each, as we don't expect every possible CPU to appear, or disappear, in a
// single MMI. The numbers of used (populated) elements in the arrays are
// determined on every MMI separately.
//
STATIC APIC_ID *mPluggedApicIds;
STATIC APIC_ID *mToUnplugApicIds;
//
// Address of the non-SMRAM reserved memory page that contains the Post-SMM Pen
// for hot-added CPUs.
//
STATIC UINT32 mPostSmmPenAddress;
//
// Represents the registration of the CPU Hotplug MMI handler.
//
STATIC EFI_HANDLE mDispatchHandle;


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
  IN EFI_HANDLE DispatchHandle,
  IN CONST VOID *Context        OPTIONAL,
  IN OUT VOID   *CommBuffer     OPTIONAL,
  IN OUT UINTN  *CommBufferSize OPTIONAL
  )
{
  EFI_STATUS Status;
  UINT8      ApmControl;
  UINT32     PluggedCount;
  UINT32     ToUnplugCount;
  UINT32     PluggedIdx;
  UINT32     NewSlot;

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
  Status = mMmCpuIo->Io.Read (mMmCpuIo, MM_IO_UINT8, ICH9_APM_CNT, 1,
                          &ApmControl);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to read ICH9_APM_CNT: %r\n", __FUNCTION__,
      Status));
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
             &ToUnplugCount
             );
  if (EFI_ERROR (Status)) {
    goto Fatal;
  }
  if (ToUnplugCount > 0) {
    DEBUG ((DEBUG_ERROR, "%a: hot-unplug is not supported yet\n",
      __FUNCTION__));
    goto Fatal;
  }

  //
  // Process hot-added CPUs.
  //
  // The Post-SMM Pen need not be reinstalled multiple times within a single
  // root MMI handling. Even reinstalling once per root MMI is only prudence;
  // in theory installing the pen in the driver's entry point function should
  // suffice.
  //
  SmbaseReinstallPostSmmPen (mPostSmmPenAddress);

  PluggedIdx = 0;
  NewSlot = 0;
  while (PluggedIdx < PluggedCount) {
    APIC_ID NewApicId;
    UINT32  CheckSlot;
    UINTN   NewProcessorNumberByProtocol;

    NewApicId = mPluggedApicIds[PluggedIdx];

    //
    // Check if the supposedly hot-added CPU is already known to us.
    //
    for (CheckSlot = 0;
         CheckSlot < mCpuHotPlugData->ArrayLength;
         CheckSlot++) {
      if (mCpuHotPlugData->ApicId[CheckSlot] == NewApicId) {
        break;
      }
    }
    if (CheckSlot < mCpuHotPlugData->ArrayLength) {
      DEBUG ((DEBUG_VERBOSE, "%a: APIC ID " FMT_APIC_ID " was hot-plugged "
        "before; ignoring it\n", __FUNCTION__, NewApicId));
      PluggedIdx++;
      continue;
    }

    //
    // Find the first empty slot in CPU_HOT_PLUG_DATA.
    //
    while (NewSlot < mCpuHotPlugData->ArrayLength &&
           mCpuHotPlugData->ApicId[NewSlot] != MAX_UINT64) {
      NewSlot++;
    }
    if (NewSlot == mCpuHotPlugData->ArrayLength) {
      DEBUG ((DEBUG_ERROR, "%a: no room for APIC ID " FMT_APIC_ID "\n",
        __FUNCTION__, NewApicId));
      goto Fatal;
    }

    //
    // Store the APIC ID of the new processor to the slot.
    //
    mCpuHotPlugData->ApicId[NewSlot] = NewApicId;

    //
    // Relocate the SMBASE of the new CPU.
    //
    Status = SmbaseRelocate (NewApicId, mCpuHotPlugData->SmBase[NewSlot],
               mPostSmmPenAddress);
    if (EFI_ERROR (Status)) {
      goto RevokeNewSlot;
    }

    //
    // Add the new CPU with EFI_SMM_CPU_SERVICE_PROTOCOL.
    //
    Status = mMmCpuService->AddProcessor (mMmCpuService, NewApicId,
                              &NewProcessorNumberByProtocol);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: AddProcessor(" FMT_APIC_ID "): %r\n",
        __FUNCTION__, NewApicId, Status));
      goto RevokeNewSlot;
    }

    DEBUG ((DEBUG_INFO, "%a: hot-added APIC ID " FMT_APIC_ID ", SMBASE 0x%Lx, "
      "EFI_SMM_CPU_SERVICE_PROTOCOL assigned number %Lu\n", __FUNCTION__,
      NewApicId, (UINT64)mCpuHotPlugData->SmBase[NewSlot],
      (UINT64)NewProcessorNumberByProtocol));

    NewSlot++;
    PluggedIdx++;
  }

  //
  // We've handled this MMI.
  //
  return EFI_SUCCESS;

RevokeNewSlot:
  mCpuHotPlugData->ApicId[NewSlot] = MAX_UINT64;

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
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  UINTN      Size;

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
  Status = gMmst->MmLocateProtocol (&gEfiMmCpuIoProtocolGuid,
                    NULL /* Registration */, (VOID **)&mMmCpuIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: locate MmCpuIo: %r\n", __FUNCTION__, Status));
    goto Fatal;
  }
  Status = gMmst->MmLocateProtocol (&gEfiSmmCpuServiceProtocolGuid,
                    NULL /* Registration */, (VOID **)&mMmCpuService);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: locate MmCpuService: %r\n", __FUNCTION__,
      Status));
    goto Fatal;
  }

  //
  // Our DEPEX on EFI_SMM_CPU_SERVICE_PROTOCOL guarantees that PiSmmCpuDxeSmm
  // has pointed PcdCpuHotPlugDataAddress to CPU_HOT_PLUG_DATA in SMRAM.
  //
  mCpuHotPlugData = (VOID *)(UINTN)PcdGet64 (PcdCpuHotPlugDataAddress);
  if (mCpuHotPlugData == NULL) {
    Status = EFI_NOT_FOUND;
    DEBUG ((DEBUG_ERROR, "%a: CPU_HOT_PLUG_DATA: %r\n", __FUNCTION__, Status));
    goto Fatal;
  }
  //
  // If the possible CPU count is 1, there's nothing for this driver to do.
  //
  if (mCpuHotPlugData->ArrayLength == 1) {
    return EFI_UNSUPPORTED;
  }
  //
  // Allocate the data structures that depend on the possible CPU count.
  //
  if (RETURN_ERROR (SafeUintnSub (mCpuHotPlugData->ArrayLength, 1, &Size)) ||
      RETURN_ERROR (SafeUintnMult (sizeof (APIC_ID), Size, &Size))) {
    Status = EFI_ABORTED;
    DEBUG ((DEBUG_ERROR, "%a: invalid CPU_HOT_PLUG_DATA\n", __FUNCTION__));
    goto Fatal;
  }
  Status = gMmst->MmAllocatePool (EfiRuntimeServicesData, Size,
                    (VOID **)&mPluggedApicIds);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmAllocatePool(): %r\n", __FUNCTION__, Status));
    goto Fatal;
  }
  Status = gMmst->MmAllocatePool (EfiRuntimeServicesData, Size,
                    (VOID **)&mToUnplugApicIds);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: MmAllocatePool(): %r\n", __FUNCTION__, Status));
    goto ReleasePluggedApicIds;
  }

  //
  // Allocate the Post-SMM Pen for hot-added CPUs.
  //
  Status = SmbaseAllocatePostSmmPen (&mPostSmmPenAddress,
             SystemTable->BootServices);
  if (EFI_ERROR (Status)) {
    goto ReleaseToUnplugApicIds;
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
    DEBUG ((DEBUG_ERROR, "%a: modern CPU hotplug interface: %r\n",
      __FUNCTION__, Status));
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
    DEBUG ((DEBUG_ERROR, "%a: MmiHandlerRegister(): %r\n", __FUNCTION__,
      Status));
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
