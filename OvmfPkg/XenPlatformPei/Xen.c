/**@file
  Xen Platform PEI support

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>
  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Guid/XenInfo.h>
#include <IndustryStandard/E820.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/MtrrLib.h>
#include <IndustryStandard/Xen/arch-x86/hvm/start_info.h>
#include <Library/XenHypercallLib.h>
#include <IndustryStandard/Xen/memory.h>

#include "Platform.h"
#include "Xen.h"

STATIC UINT32 mXenLeaf = 0;

EFI_XEN_INFO mXenInfo;

//
// Location of the firmware info struct setup by hvmloader.
// Only the E820 table is used by OVMF.
//
EFI_XEN_OVMF_INFO *mXenHvmloaderInfo;
STATIC EFI_E820_ENTRY64 mE820Entries[128];
STATIC UINT32 mE820EntriesCount;

/**
  Returns E820 map provided by Xen

  @param Entries      Pointer to E820 map
  @param Count        Number of entries

  @return EFI_STATUS
**/
EFI_STATUS
XenGetE820Map (
  EFI_E820_ENTRY64 **Entries,
  UINT32 *Count
  )
{
  INTN ReturnCode;
  xen_memory_map_t Parameters;
  UINTN LoopIndex;
  UINTN Index;
  EFI_E820_ENTRY64 TmpEntry;

  //
  // Get E820 produced by hvmloader
  //
  if (mXenHvmloaderInfo != NULL) {
    ASSERT (mXenHvmloaderInfo->E820 < MAX_ADDRESS);
    *Entries = (EFI_E820_ENTRY64 *)(UINTN) mXenHvmloaderInfo->E820;
    *Count = mXenHvmloaderInfo->E820EntriesCount;

    return EFI_SUCCESS;
  }

  //
  // Otherwise, get the E820 table from the Xen hypervisor
  //

  if (mE820EntriesCount > 0) {
    *Entries = mE820Entries;
    *Count = mE820EntriesCount;
    return EFI_SUCCESS;
  }

  Parameters.nr_entries = 128;
  set_xen_guest_handle (Parameters.buffer, mE820Entries);

  // Returns a errno
  ReturnCode = XenHypercallMemoryOp (XENMEM_memory_map, &Parameters);
  ASSERT (ReturnCode == 0);

  mE820EntriesCount = Parameters.nr_entries;

  //
  // Sort E820 entries
  //
  for (LoopIndex = 1; LoopIndex < mE820EntriesCount; LoopIndex++) {
    for (Index = LoopIndex; Index < mE820EntriesCount; Index++) {
      if (mE820Entries[Index - 1].BaseAddr > mE820Entries[Index].BaseAddr) {
        TmpEntry = mE820Entries[Index];
        mE820Entries[Index] = mE820Entries[Index - 1];
        mE820Entries[Index - 1] = TmpEntry;
      }
    }
  }

  *Count = mE820EntriesCount;
  *Entries = mE820Entries;

  return EFI_SUCCESS;
}

/**
  Connects to the Hypervisor.

  @return EFI_STATUS

**/
EFI_STATUS
XenConnect (
  )
{
  UINT32 Index;
  UINT32 TransferReg;
  UINT32 TransferPages;
  UINT32 XenVersion;
  EFI_XEN_OVMF_INFO *Info;
  CHAR8 Sig[sizeof (Info->Signature) + 1];
  UINT32 *PVHResetVectorData;
  RETURN_STATUS Status;

  ASSERT (mXenLeaf != 0);

  //
  // Prepare HyperPages to be able to make hypercalls
  //

  AsmCpuid (mXenLeaf + 2, &TransferPages, &TransferReg, NULL, NULL);
  mXenInfo.HyperPages = AllocatePages (TransferPages);
  if (!mXenInfo.HyperPages) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < TransferPages; Index++) {
    AsmWriteMsr64 (TransferReg,
                   (UINTN) mXenInfo.HyperPages +
                   (Index << EFI_PAGE_SHIFT) + Index);
  }

  //
  // Find out the Xen version
  //

  AsmCpuid (mXenLeaf + 1, &XenVersion, NULL, NULL, NULL);
  DEBUG ((DEBUG_ERROR, "Detected Xen version %d.%d\n",
          XenVersion >> 16, XenVersion & 0xFFFF));
  mXenInfo.VersionMajor = (UINT16)(XenVersion >> 16);
  mXenInfo.VersionMinor = (UINT16)(XenVersion & 0xFFFF);

  //
  // Check if there are information left by hvmloader
  //

  Info = (EFI_XEN_OVMF_INFO *)(UINTN) OVMF_INFO_PHYSICAL_ADDRESS;
  //
  // Copy the signature, and make it null-terminated.
  //
  AsciiStrnCpyS (Sig, sizeof (Sig), (CHAR8 *) &Info->Signature,
    sizeof (Info->Signature));
  if (AsciiStrCmp (Sig, "XenHVMOVMF") == 0) {
    mXenHvmloaderInfo = Info;
  } else {
    mXenHvmloaderInfo = NULL;
  }

  mXenInfo.RsdpPvh = NULL;

  //
  // Locate and use information from the start of day structure if we have
  // booted via the PVH entry point.
  //

  PVHResetVectorData = (VOID *)(UINTN) PcdGet32 (PcdXenPvhStartOfDayStructPtr);
  //
  // That magic value is written in XenResetVector/Ia32/XenPVHMain.asm
  //
  if (PVHResetVectorData[1] == SIGNATURE_32 ('X', 'P', 'V', 'H')) {
    struct hvm_start_info *HVMStartInfo;

    HVMStartInfo = (VOID *)(UINTN) PVHResetVectorData[0];
    if (HVMStartInfo->magic == XEN_HVM_START_MAGIC_VALUE) {
      ASSERT (HVMStartInfo->rsdp_paddr != 0);
      if (HVMStartInfo->rsdp_paddr != 0) {
        mXenInfo.RsdpPvh = (VOID *)(UINTN)HVMStartInfo->rsdp_paddr;
      }
    }
  }

  BuildGuidDataHob (
    &gEfiXenInfoGuid,
    &mXenInfo,
    sizeof(mXenInfo)
    );

  //
  // Initialize the XenHypercall library, now that the XenInfo HOB is
  // available
  //
  Status = XenHypercallLibInit ();
  ASSERT_RETURN_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Figures out if we are running inside Xen HVM.

  @retval TRUE   Xen was detected
  @retval FALSE  Xen was not detected

**/
BOOLEAN
XenDetect (
  VOID
  )
{
  UINT8 Signature[13];

  if (mXenLeaf != 0) {
    return TRUE;
  }

  Signature[12] = '\0';
  for (mXenLeaf = 0x40000000; mXenLeaf < 0x40010000; mXenLeaf += 0x100) {
    AsmCpuid (mXenLeaf,
              NULL,
              (UINT32 *) &Signature[0],
              (UINT32 *) &Signature[4],
              (UINT32 *) &Signature[8]);

    if (!AsciiStrCmp ((CHAR8 *) Signature, "XenVMMXenVMM")) {
      return TRUE;
    }
  }

  mXenLeaf = 0;
  return FALSE;
}

BOOLEAN
XenHvmloaderDetected (
  VOID
  )
{
  return (mXenHvmloaderInfo != NULL);
}

BOOLEAN
XenPvhDetected (
  VOID
  )
{
  //
  // This function should only be used after XenConnect
  //
  ASSERT (mXenInfo.HyperPages != NULL);

  return mXenHvmloaderInfo == NULL;
}

VOID
XenPublishRamRegions (
  VOID
  )
{
  EFI_E820_ENTRY64      *E820Map;
  UINT32                E820EntriesCount;
  EFI_STATUS            Status;
  EFI_E820_ENTRY64      *Entry;
  UINTN                 Index;
  UINT64                LapicBase;
  UINT64                LapicEnd;


  DEBUG ((DEBUG_INFO, "Using memory map provided by Xen\n"));

  //
  // Parse RAM in E820 map
  //
  E820EntriesCount = 0;
  Status = XenGetE820Map (&E820Map, &E820EntriesCount);
  ASSERT_EFI_ERROR (Status);

  AddMemoryBaseSizeHob (0, 0xA0000);
  //
  // Video memory + Legacy BIOS region, to allow Linux to boot.
  //
  AddReservedMemoryBaseSizeHob (0xA0000, BASE_1MB - 0xA0000, TRUE);

  LapicBase = PcdGet32 (PcdCpuLocalApicBaseAddress);
  LapicEnd = LapicBase + SIZE_1MB;
  AddIoMemoryRangeHob (LapicBase, LapicEnd);

  for (Index = 0; Index < E820EntriesCount; Index++) {
    UINT64 Base;
    UINT64 End;
    UINT64 ReservedBase;
    UINT64 ReservedEnd;

    Entry = &E820Map[Index];

    //
    // Round up the start address, and round down the end address.
    //
    Base = ALIGN_VALUE (Entry->BaseAddr, (UINT64)EFI_PAGE_SIZE);
    End = (Entry->BaseAddr + Entry->Length) & ~(UINT64)EFI_PAGE_MASK;

    //
    // Ignore the first 1MB, this is handled before the loop.
    //
    if (Base < BASE_1MB) {
      Base = BASE_1MB;
    }
    if (Base >= End) {
      continue;
    }

    switch (Entry->Type) {
    case EfiAcpiAddressRangeMemory:
      AddMemoryRangeHob (Base, End);
      break;
    case EfiAcpiAddressRangeACPI:
      AddReservedMemoryRangeHob (Base, End, FALSE);
      break;
    case EfiAcpiAddressRangeReserved:
      //
      // hvmloader marks a range that overlaps with the local APIC memory
      // mapped region as reserved, but CpuDxe wants it as mapped IO. We
      // have already added it as mapped IO, so skip it here.
      //

      //
      // add LAPIC predecessor range, if any
      //
      ReservedBase = Base;
      ReservedEnd = MIN (End, LapicBase);
      if (ReservedBase < ReservedEnd) {
        AddReservedMemoryRangeHob (ReservedBase, ReservedEnd, FALSE);
      }

      //
      // add LAPIC successor range, if any
      //
      ReservedBase = MAX (Base, LapicEnd);
      ReservedEnd = End;
      if (ReservedBase < ReservedEnd) {
        AddReservedMemoryRangeHob (ReservedBase, ReservedEnd, FALSE);
      }
      break;
    default:
      break;
    }
  }
}


/**
  Perform Xen PEI initialization.

  @return EFI_SUCCESS     Xen initialized successfully
  @return EFI_NOT_FOUND   Not running under Xen

**/
EFI_STATUS
InitializeXen (
  VOID
  )
{
  RETURN_STATUS PcdStatus;

  PcdStatus = PcdSetBoolS (PcdPciDisableBusEnumeration, TRUE);
  ASSERT_RETURN_ERROR (PcdStatus);

  return EFI_SUCCESS;
}
