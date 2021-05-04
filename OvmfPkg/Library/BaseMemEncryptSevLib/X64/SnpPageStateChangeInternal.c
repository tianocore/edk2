/** @file

  SEV-SNP Page Validation functions.

  Copyright (c) 2021 AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/DebugLib.h>
#include <Library/VmgExitLib.h>

#include <Register/Amd/Ghcb.h>
#include <Register/Amd/Msr.h>

#include "SnpPageStateChange.h"

#define IS_ALIGNED(x, y)        ((((x) & (y - 1)) == 0))
#define PAGES_PER_LARGE_ENTRY   512

STATIC
UINTN
MemoryStateToGhcbOp (
  IN SEV_SNP_PAGE_STATE   State
  )
{
  UINTN Cmd;

  switch (State) {
    case SevSnpPageShared: Cmd = SNP_PAGE_STATE_SHARED; break;
    case SevSnpPagePrivate: Cmd = SNP_PAGE_STATE_PRIVATE; break;
    default: ASSERT(0);
  }

  return Cmd;
}

STATIC
VOID
SnpPageStateFailureTerminate (
  VOID
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request termination by the hypervisor
  //
  Msr.GhcbPhysicalAddress = 0;
  Msr.GhcbTerminate.Function = GHCB_INFO_TERMINATE_REQUEST;
  Msr.GhcbTerminate.ReasonCodeSet = GHCB_TERMINATE_GHCB;
  Msr.GhcbTerminate.ReasonCode = GHCB_TERMINATE_GHCB_GENERAL;
  AsmWriteMsr64 (MSR_SEV_ES_GHCB, Msr.GhcbPhysicalAddress);

  AsmVmgExit ();

  ASSERT (FALSE);
  CpuDeadLoop ();
}

/**
 This function issues the PVALIDATE instruction to validate or invalidate the memory
 range specified. If PVALIDATE returns size mismatch then it retry validating with
 smaller page size.

 */
STATIC
VOID
PvalidateRange (
  IN  SNP_PAGE_STATE_CHANGE_INFO    *Info,
  IN  UINTN                         StartIndex,
  IN  UINTN                         EndIndex,
  IN  BOOLEAN                       Validate
  )
{
  UINTN         Address, RmpPageSize, Ret, i;

  for (; StartIndex < EndIndex; StartIndex++) {
    //
    // Get the address and the page size from the Info.
    //
    Address = Info->Entry[StartIndex].GuestFrameNumber << EFI_PAGE_SHIFT;
    RmpPageSize = Info->Entry[StartIndex].PageSize;

    Ret = AsmPvalidate (RmpPageSize, Validate, Address);

    //
    // If we fail to validate due to size mismatch then try with the
    // smaller page size. This senario will occur if the backing page in
    // the RMP entry is 4K and we are validating it as a 2MB.
    //
    if ((Ret == PVALIDATE_RET_SIZE_MISMATCH) && (RmpPageSize == PvalidatePageSize2MB)) {
      for (i = 0; i < PAGES_PER_LARGE_ENTRY; i++) {
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
        DEBUG_ERROR, "%a:%a: Failed to %a address 0x%Lx Error code %d\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        Validate ? "Validate" : "Invalidate",
        Address,
        Ret
        ));
      SnpPageStateFailureTerminate ();
    }
  }
}

/**
 The function is used to set the page state when SEV-SNP is active. The page state
 transition consist of changing the page ownership in the RMP table, and using the
 PVALIDATE instruction to update the Validated bit in RMP table.

 When the UseLargeEntry is set to TRUE, then function will try to use the large RMP
 entry (whevever possible).
 */
VOID
InternalSetPageState (
  IN EFI_PHYSICAL_ADDRESS             BaseAddress,
  IN UINTN                            NumPages,
  IN SEV_SNP_PAGE_STATE               State,
  IN BOOLEAN                          UseLargeEntry
  )
{
  EFI_STATUS                      Status;
  GHCB                            *Ghcb;
  EFI_PHYSICAL_ADDRESS            NextAddress, EndAddress;
  MSR_SEV_ES_GHCB_REGISTER        Msr;
  BOOLEAN                         InterruptState;
  SNP_PAGE_STATE_CHANGE_INFO      *Info;
  UINTN                           i, RmpPageSize;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  Ghcb = Msr.Ghcb;

  EndAddress = BaseAddress + EFI_PAGES_TO_SIZE (NumPages);

  DEBUG ((
    DEBUG_VERBOSE, "%a:%a Address 0x%Lx - 0x%Lx State = %a LargeEntry = %d\n",
    gEfiCallerBaseName,
    __FUNCTION__,
    BaseAddress,
    EndAddress,
    State == SevSnpPageShared ? "Shared" : "Private",
    UseLargeEntry
    ));

  for (; BaseAddress < EndAddress; BaseAddress = NextAddress) {
    //
    // Initialize the GHCB and setup scratch sw to point to shared buffer.
    //
    VmgInit (Ghcb, &InterruptState);
    Info = (SNP_PAGE_STATE_CHANGE_INFO *) Ghcb->SharedBuffer;

    SetMem (Info, sizeof (*Info), 0);

    //
    // Build page state change buffer
    //
    for (i = 0; (EndAddress > BaseAddress) && i < SNP_PAGE_STATE_MAX_ENTRY;
        BaseAddress = NextAddress, i++) {
      //
      // Is this a 2MB aligned page? Check if we can use the Large RMP entry.
      //
      if (UseLargeEntry && IS_ALIGNED (BaseAddress, SIZE_2MB) &&
        ((EndAddress - BaseAddress) >= SIZE_2MB)) {
        RmpPageSize = PvalidatePageSize2MB;
        NextAddress = BaseAddress + SIZE_2MB;
      } else {
        RmpPageSize = PvalidatePageSize4K;
        NextAddress = BaseAddress + EFI_PAGE_SIZE;
      }

      Info->Entry[i].GuestFrameNumber = BaseAddress >> EFI_PAGE_SHIFT;
      Info->Entry[i].PageSize = RmpPageSize;
      Info->Entry[i].Operation = MemoryStateToGhcbOp (State);
      Info->Entry[i].CurrentPage = 0;
    }

    Info->Header.CurrentEntry = 0;
    Info->Header.EndEntry = i - 1;

    //
    // If the request page state change is shared then invalidate the pages before
    // adding the page in the RMP table.
    //
    if (State == SevSnpPageShared) {
      PvalidateRange (Info, 0, i, FALSE);
    }

    //
    // Issue the VMGEXIT and retry if hypervisor failed to process all the entries.
    //
    while (Info->Header.CurrentEntry <= Info->Header.EndEntry) {
      Ghcb->SaveArea.SwScratch = (UINT64) Ghcb->SharedBuffer;
      VmgSetOffsetValid (Ghcb, GhcbSwScratch);

      Status = VmgExit (Ghcb, SVM_EXIT_SNP_PAGE_STATE_CHANGE, 0, 0);
      if ((Status != 0) || (Ghcb->SaveArea.SwExitInfo2)) {
        SnpPageStateFailureTerminate ();
      }
    }

    //
    // If the request page state change is shared then invalidate the pages before
    // adding the page in the RMP table.
    //
    if (State == SevSnpPagePrivate) {
      PvalidateRange (Info, 0, i, TRUE);
    }

    VmgDone (Ghcb, InterruptState);
  }
}
