/** @file
  SMBASE relocation for hot-plugged CPUs.

  Copyright (c) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>                             // BASE_1MB
#include <Library/BaseLib.h>                  // CpuPause()
#include <Library/BaseMemoryLib.h>            // CopyMem()
#include <Library/DebugLib.h>                 // DEBUG()
#include <Library/LocalApicLib.h>             // SendInitSipiSipi()
#include <Library/SynchronizationLib.h>       // InterlockedCompareExchange64()
#include <Register/Intel/SmramSaveStateMap.h> // SMM_DEFAULT_SMBASE

#include "FirstSmiHandlerContext.h"           // FIRST_SMI_HANDLER_CONTEXT

#include "Smbase.h"

extern CONST UINT8   mPostSmmPen[];
extern CONST UINT16  mPostSmmPenSize;
extern CONST UINT8   mFirstSmiHandler[];
extern CONST UINT16  mFirstSmiHandlerSize;

/**
  Allocate a non-SMRAM reserved memory page for the Post-SMM Pen for hot-added
  CPUs.

  This function may only be called from the entry point function of the driver.

  @param[out] PenAddress   The address of the allocated (normal RAM) reserved
                           page.

  @param[in] BootServices  Pointer to the UEFI boot services table. Used for
                           allocating the normal RAM (not SMRAM) reserved page.

  @retval EFI_SUCCESS          Allocation successful.

  @retval EFI_BAD_BUFFER_SIZE  The Post-SMM Pen template is not smaller than
                               EFI_PAGE_SIZE.

  @return                      Error codes propagated from underlying services.
                               DEBUG_ERROR messages have been logged. No
                               resources have been allocated.
**/
EFI_STATUS
SmbaseAllocatePostSmmPen (
  OUT UINT32                   *PenAddress,
  IN  CONST EFI_BOOT_SERVICES  *BootServices
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;

  //
  // The pen code must fit in one page, and the last byte must remain free for
  // signaling the SMM Monarch.
  //
  if (mPostSmmPenSize >= EFI_PAGE_SIZE) {
    Status = EFI_BAD_BUFFER_SIZE;
    DEBUG ((
      DEBUG_ERROR,
      "%a: mPostSmmPenSize=%u: %r\n",
      __FUNCTION__,
      mPostSmmPenSize,
      Status
      ));
    return Status;
  }

  Address = BASE_1MB - 1;
  Status  = BootServices->AllocatePages (
                            AllocateMaxAddress,
                            EfiReservedMemoryType,
                            1,
                            &Address
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: AllocatePages(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: Post-SMM Pen at 0x%Lx\n", __FUNCTION__, Address));
  *PenAddress = (UINT32)Address;
  return EFI_SUCCESS;
}

/**
  Copy the Post-SMM Pen template code into the reserved page allocated with
  SmbaseAllocatePostSmmPen().

  Note that this effects an "SMRAM to normal RAM" copy.

  The SMM Monarch is supposed to call this function from the root MMI handler.

  @param[in] PenAddress  The allocation address returned by
                         SmbaseAllocatePostSmmPen().
**/
VOID
SmbaseReinstallPostSmmPen (
  IN UINT32  PenAddress
  )
{
  CopyMem ((VOID *)(UINTN)PenAddress, mPostSmmPen, mPostSmmPenSize);
}

/**
  Release the reserved page allocated with SmbaseAllocatePostSmmPen().

  This function may only be called from the entry point function of the driver,
  on the error path.

  @param[in] PenAddress    The allocation address returned by
                           SmbaseAllocatePostSmmPen().

  @param[in] BootServices  Pointer to the UEFI boot services table. Used for
                           releasing the normal RAM (not SMRAM) reserved page.
**/
VOID
SmbaseReleasePostSmmPen (
  IN UINT32                   PenAddress,
  IN CONST EFI_BOOT_SERVICES  *BootServices
  )
{
  BootServices->FreePages (PenAddress, 1);
}

/**
  Place the handler routine for the first SMIs of hot-added CPUs at
  (SMM_DEFAULT_SMBASE + SMM_HANDLER_OFFSET).

  Note that this effects an "SMRAM to SMRAM" copy.

  Additionally, shut the APIC ID gate in FIRST_SMI_HANDLER_CONTEXT.

  This function may only be called from the entry point function of the driver,
  and only after PcdQ35SmramAtDefaultSmbase has been determined to be TRUE.
**/
VOID
SmbaseInstallFirstSmiHandler (
  VOID
  )
{
  FIRST_SMI_HANDLER_CONTEXT  *Context;

  CopyMem (
    (VOID *)(UINTN)(SMM_DEFAULT_SMBASE + SMM_HANDLER_OFFSET),
    mFirstSmiHandler,
    mFirstSmiHandlerSize
    );

  Context             = (VOID *)(UINTN)SMM_DEFAULT_SMBASE;
  Context->ApicIdGate = MAX_UINT64;
}

/**
  Relocate the SMBASE on a hot-added CPU. Then pen the hot-added CPU in the
  normal RAM reserved memory page, set up earlier with
  SmbaseAllocatePostSmmPen() and SmbaseReinstallPostSmmPen().

  The SMM Monarch is supposed to call this function from the root MMI handler.

  The SMM Monarch is responsible for calling SmbaseInstallFirstSmiHandler(),
  SmbaseAllocatePostSmmPen(), and SmbaseReinstallPostSmmPen() before calling
  this function.

  If the OS maliciously boots the hot-added CPU ahead of letting the ACPI CPU
  hotplug event handler broadcast the CPU hotplug MMI, then the hot-added CPU
  returns to the OS rather than to the pen, upon RSM. In that case, this
  function will hang forever (unless the OS happens to signal back through the
  last byte of the pen page).

  @param[in] ApicId      The APIC ID of the hot-added CPU whose SMBASE should
                         be relocated.

  @param[in] Smbase      The new SMBASE address. The root MMI handler is
                         responsible for passing in a free ("unoccupied")
                         SMBASE address that was pre-configured by
                         PiSmmCpuDxeSmm in CPU_HOT_PLUG_DATA.

  @param[in] PenAddress  The address of the Post-SMM Pen for hot-added CPUs, as
                         returned by SmbaseAllocatePostSmmPen(), and installed
                         by SmbaseReinstallPostSmmPen().

  @retval EFI_SUCCESS            The SMBASE of the hot-added CPU with APIC ID
                                 ApicId has been relocated to Smbase. The
                                 hot-added CPU has reported back about leaving
                                 SMM.

  @retval EFI_PROTOCOL_ERROR     Synchronization bug encountered around
                                 FIRST_SMI_HANDLER_CONTEXT.ApicIdGate.

  @retval EFI_INVALID_PARAMETER  Smbase does not fit in 32 bits. No relocation
                                 has been attempted.
**/
EFI_STATUS
SmbaseRelocate (
  IN APIC_ID  ApicId,
  IN UINTN    Smbase,
  IN UINT32   PenAddress
  )
{
  EFI_STATUS                          Status;
  volatile UINT8                      *SmmVacated;
  volatile FIRST_SMI_HANDLER_CONTEXT  *Context;
  UINT64                              ExchangeResult;

  if (Smbase > MAX_UINT32) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "%a: ApicId=" FMT_APIC_ID " Smbase=0x%Lx: %r\n",
      __FUNCTION__,
      ApicId,
      (UINT64)Smbase,
      Status
      ));
    return Status;
  }

  SmmVacated = (UINT8 *)(UINTN)PenAddress + (EFI_PAGE_SIZE - 1);
  Context    = (VOID *)(UINTN)SMM_DEFAULT_SMBASE;

  //
  // Clear AboutToLeaveSmm, so we notice when the hot-added CPU is just about
  // to reach RSM, and we can proceed to polling the last byte of the reserved
  // page (which could be attacked by the OS).
  //
  Context->AboutToLeaveSmm = 0;

  //
  // Clear the last byte of the reserved page, so we notice when the hot-added
  // CPU checks back in from the pen.
  //
  *SmmVacated = 0;

  //
  // Boot the hot-added CPU.
  //
  // There are 2*2 cases to consider:
  //
  // (1) The CPU was hot-added before the SMI was broadcast.
  //
  // (1.1) The OS is benign.
  //
  //       The hot-added CPU is in RESET state, with the broadcast SMI pending
  //       for it. The directed SMI below will be ignored (it's idempotent),
  //       and the INIT-SIPI-SIPI will launch the CPU directly into SMM.
  //
  // (1.2) The OS is malicious.
  //
  //       The hot-added CPU has been booted, by the OS. Thus, the hot-added
  //       CPU is spinning on the APIC ID gate. In that case, both the SMI and
  //       the INIT-SIPI-SIPI below will be ignored.
  //
  // (2) The CPU was hot-added after the SMI was broadcast.
  //
  // (2.1) The OS is benign.
  //
  //       The hot-added CPU is in RESET state, with no SMI pending for it. The
  //       directed SMI will latch the SMI for the CPU. Then the INIT-SIPI-SIPI
  //       will launch the CPU into SMM.
  //
  // (2.2) The OS is malicious.
  //
  //       The hot-added CPU is executing OS code. The directed SMI will pull
  //       the hot-added CPU into SMM, where it will start spinning on the APIC
  //       ID gate. The INIT-SIPI-SIPI will be ignored.
  //
  SendSmiIpi (ApicId);
  SendInitSipiSipi (ApicId, PenAddress);

  //
  // Expose the desired new SMBASE value to the hot-added CPU.
  //
  Context->NewSmbase = (UINT32)Smbase;

  //
  // Un-gate SMBASE relocation for the hot-added CPU whose APIC ID is ApicId.
  //
  ExchangeResult = InterlockedCompareExchange64 (
                     &Context->ApicIdGate,
                     MAX_UINT64,
                     ApicId
                     );
  if (ExchangeResult != MAX_UINT64) {
    Status = EFI_PROTOCOL_ERROR;
    DEBUG ((
      DEBUG_ERROR,
      "%a: ApicId=" FMT_APIC_ID " ApicIdGate=0x%Lx: %r\n",
      __FUNCTION__,
      ApicId,
      ExchangeResult,
      Status
      ));
    return Status;
  }

  //
  // Wait until the hot-added CPU is just about to execute RSM.
  //
  while (Context->AboutToLeaveSmm == 0) {
    CpuPause ();
  }

  //
  // Now wait until the hot-added CPU reports back from the pen (or the OS
  // attacks the last byte of the reserved page).
  //
  while (*SmmVacated == 0) {
    CpuPause ();
  }

  Status = EFI_SUCCESS;
  return Status;
}
