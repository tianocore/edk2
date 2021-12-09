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

#define IS_ALIGNED(x, y)  ((((x) & (y - 1)) == 0))
#define PAGES_PER_LARGE_ENTRY  512

STATIC
UINTN
MemoryStateToGhcbOp (
  IN SEV_SNP_PAGE_STATE  State
  )
{
  UINTN  Cmd;

  switch (State) {
    case SevSnpPageShared: Cmd = SNP_PAGE_STATE_SHARED;
      break;
    case SevSnpPagePrivate: Cmd = SNP_PAGE_STATE_PRIVATE;
      break;
    default: ASSERT (0);
  }

  return Cmd;
}

VOID
SnpPageStateFailureTerminate (
  VOID
  )
{
  MSR_SEV_ES_GHCB_REGISTER  Msr;

  //
  // Use the GHCB MSR Protocol to request termination by the hypervisor
  //
  Msr.GhcbPhysicalAddress         = 0;
  Msr.GhcbTerminate.Function      = GHCB_INFO_TERMINATE_REQUEST;
  Msr.GhcbTerminate.ReasonCodeSet = GHCB_TERMINATE_GHCB;
  Msr.GhcbTerminate.ReasonCode    = GHCB_TERMINATE_GHCB_GENERAL;
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
  IN  SNP_PAGE_STATE_CHANGE_INFO  *Info,
  IN  UINTN                       StartIndex,
  IN  UINTN                       EndIndex,
  IN  BOOLEAN                     Validate
  )
{
  UINTN  Address, RmpPageSize, Ret, i;

  for ( ; StartIndex <= EndIndex; StartIndex++) {
    //
    // Get the address and the page size from the Info.
    //
    Address     = Info->Entry[StartIndex].GuestFrameNumber << EFI_PAGE_SHIFT;
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
        DEBUG_ERROR,
        "%a:%a: Failed to %a address 0x%Lx Error code %d\n",
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

STATIC
EFI_PHYSICAL_ADDRESS
BuildPageStateBuffer (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN EFI_PHYSICAL_ADDRESS        EndAddress,
  IN SEV_SNP_PAGE_STATE          State,
  IN BOOLEAN                     UseLargeEntry,
  IN SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  EFI_PHYSICAL_ADDRESS  NextAddress;
  UINTN                 i, RmpPageSize;

  // Clear the page state structure
  SetMem (Info, sizeof (*Info), 0);

  i           = 0;
  NextAddress = EndAddress;

  //
  // Populate the page state entry structure
  //
  while ((BaseAddress < EndAddress) && (i < SNP_PAGE_STATE_MAX_ENTRY)) {
    //
    // Is this a 2MB aligned page? Check if we can use the Large RMP entry.
    //
    if (UseLargeEntry && IS_ALIGNED (BaseAddress, SIZE_2MB) &&
        ((EndAddress - BaseAddress) >= SIZE_2MB))
    {
      RmpPageSize = PvalidatePageSize2MB;
      NextAddress = BaseAddress + SIZE_2MB;
    } else {
      RmpPageSize = PvalidatePageSize4K;
      NextAddress = BaseAddress + EFI_PAGE_SIZE;
    }

    Info->Entry[i].GuestFrameNumber = BaseAddress >> EFI_PAGE_SHIFT;
    Info->Entry[i].PageSize         = RmpPageSize;
    Info->Entry[i].Operation        = MemoryStateToGhcbOp (State);
    Info->Entry[i].CurrentPage      = 0;
    Info->Header.EndEntry           = (UINT16)i;

    BaseAddress = NextAddress;
    i++;
  }

  return NextAddress;
}

STATIC
VOID
PageStateChangeVmgExit (
  IN GHCB                        *Ghcb,
  IN SNP_PAGE_STATE_CHANGE_INFO  *Info
  )
{
  EFI_STATUS  Status;

  //
  // As per the GHCB specification, the hypervisor can resume the guest before
  // processing all the entries. Checks whether all the entries are processed.
  //
  // The stragtegy here is to wait for the hypervisor to change the page
  // state in the RMP table before guest access the memory pages. If the
  // page state was not successful, then later memory access will result
  // in the crash.
  //
  while (Info->Header.CurrentEntry <= Info->Header.EndEntry) {
    Ghcb->SaveArea.SwScratch = (UINT64)Ghcb->SharedBuffer;
    VmgSetOffsetValid (Ghcb, GhcbSwScratch);

    Status = VmgExit (Ghcb, SVM_EXIT_SNP_PAGE_STATE_CHANGE, 0, 0);

    //
    // The Page State Change VMGEXIT can pass the failure through the
    // ExitInfo2. Lets check both the return value as well as ExitInfo2.
    //
    if ((Status != 0) || (Ghcb->SaveArea.SwExitInfo2)) {
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
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 NumPages,
  IN SEV_SNP_PAGE_STATE    State,
  IN BOOLEAN               UseLargeEntry
  )
{
  GHCB                        *Ghcb;
  EFI_PHYSICAL_ADDRESS        NextAddress, EndAddress;
  MSR_SEV_ES_GHCB_REGISTER    Msr;
  BOOLEAN                     InterruptState;
  SNP_PAGE_STATE_CHANGE_INFO  *Info;

  Msr.GhcbPhysicalAddress = AsmReadMsr64 (MSR_SEV_ES_GHCB);
  Ghcb                    = Msr.Ghcb;

  EndAddress = BaseAddress + EFI_PAGES_TO_SIZE (NumPages);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a:%a Address 0x%Lx - 0x%Lx State = %a LargeEntry = %d\n",
    gEfiCallerBaseName,
    __FUNCTION__,
    BaseAddress,
    EndAddress,
    State == SevSnpPageShared ? "Shared" : "Private",
    UseLargeEntry
    ));

  while (BaseAddress < EndAddress) {
    UINTN  CurrentEntry, EndEntry;

    //
    // Initialize the GHCB
    //
    VmgInit (Ghcb, &InterruptState);

    //
    // Build the page state structure
    //
    Info        = (SNP_PAGE_STATE_CHANGE_INFO *)Ghcb->SharedBuffer;
    NextAddress = BuildPageStateBuffer (
                    BaseAddress,
                    EndAddress,
                    State,
                    UseLargeEntry,
                    Info
                    );

    //
    // Save the current and end entry from the page state structure. We need
    // it later.
    //
    CurrentEntry = Info->Header.CurrentEntry;
    EndEntry     = Info->Header.EndEntry;

    //
    // If the caller requested to change the page state to shared then
    // invalidate the pages before making the page shared in the RMP table.
    //
    if (State == SevSnpPageShared) {
      PvalidateRange (Info, CurrentEntry, EndEntry, FALSE);
    }

    //
    // Invoke the page state change VMGEXIT.
    //
    PageStateChangeVmgExit (Ghcb, Info);

    //
    // If the caller requested to change the page state to private then
    // validate the pages after it has been added in the RMP table.
    //
    if (State == SevSnpPagePrivate) {
      PvalidateRange (Info, CurrentEntry, EndEntry, TRUE);
    }

    VmgDone (Ghcb, InterruptState);

    BaseAddress = NextAddress;
  }
}
