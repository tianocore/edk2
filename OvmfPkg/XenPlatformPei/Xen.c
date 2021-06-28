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
#include <Library/BaseMemoryLib.h>
#include <Library/CpuLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Guid/XenInfo.h>
#include <IndustryStandard/E820.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/MtrrLib.h>
#include <IndustryStandard/PageTable.h>
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


EFI_STATUS
PhysicalAddressIdentityMapping (
  IN EFI_PHYSICAL_ADDRESS   AddressToMap
  )
{
  INTN                            Index;
  PAGE_MAP_AND_DIRECTORY_POINTER  *L4, *L3;
  PAGE_TABLE_ENTRY                *PageTable;

  DEBUG ((DEBUG_INFO, "Mapping 1:1 of address 0x%lx\n", (UINT64)AddressToMap));

  // L4 / Top level Page Directory Pointers

  L4 = (VOID*)(UINTN)PcdGet32 (PcdOvmfSecPageTablesBase);
  Index = PML4_OFFSET (AddressToMap);

  if (!L4[Index].Bits.Present) {
    L3 = AllocatePages (1);
    if (L3 == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (L3, EFI_PAGE_SIZE);

    L4[Index].Bits.ReadWrite = 1;
    L4[Index].Bits.Accessed = 1;
    L4[Index].Bits.PageTableBaseAddress = (EFI_PHYSICAL_ADDRESS)L3 >> 12;
    L4[Index].Bits.Present = 1;
  }

  // L3 / Next level Page Directory Pointers

  L3 = (VOID*)(EFI_PHYSICAL_ADDRESS)(L4[Index].Bits.PageTableBaseAddress << 12);
  Index = PDP_OFFSET (AddressToMap);

  if (!L3[Index].Bits.Present) {
    PageTable = AllocatePages (1);
    if (PageTable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (PageTable, EFI_PAGE_SIZE);

    L3[Index].Bits.ReadWrite = 1;
    L3[Index].Bits.Accessed = 1;
    L3[Index].Bits.PageTableBaseAddress = (EFI_PHYSICAL_ADDRESS)PageTable >> 12;
    L3[Index].Bits.Present = 1;
  }

  // L2 / Page Table Entries

  PageTable = (VOID*)(EFI_PHYSICAL_ADDRESS)(L3[Index].Bits.PageTableBaseAddress << 12);
  Index = PDE_OFFSET (AddressToMap);

  if (!PageTable[Index].Bits.Present) {
    PageTable[Index].Bits.ReadWrite = 1;
    PageTable[Index].Bits.Accessed = 1;
    PageTable[Index].Bits.Dirty = 1;
    PageTable[Index].Bits.MustBe1 = 1;
    PageTable[Index].Bits.PageTableBaseAddress = AddressToMap >> 21;
    PageTable[Index].Bits.Present = 1;
  }

  CpuFlushTlb ();

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
MapSharedInfoPage (
  IN VOID *PagePtr
  )
{
  xen_add_to_physmap_t  Parameters;
  INTN                  ReturnCode;

  Parameters.domid = DOMID_SELF;
  Parameters.space = XENMAPSPACE_shared_info;
  Parameters.idx = 0;
  Parameters.gpfn = (UINTN)PagePtr >> EFI_PAGE_SHIFT;
  ReturnCode = XenHypercallMemoryOp (XENMEM_add_to_physmap, &Parameters);
  if (ReturnCode != 0) {
    return EFI_NO_MAPPING;
  }
  return EFI_SUCCESS;
}

STATIC
VOID
UnmapXenPage (
  IN VOID *PagePtr
  )
{
  xen_remove_from_physmap_t Parameters;
  INTN                      ReturnCode;

  Parameters.domid = DOMID_SELF;
  Parameters.gpfn = (UINTN)PagePtr >> EFI_PAGE_SHIFT;
  ReturnCode = XenHypercallMemoryOp (XENMEM_remove_from_physmap, &Parameters);
  ASSERT (ReturnCode == 0);
}


STATIC
UINT64
GetCpuFreq (
  IN XEN_VCPU_TIME_INFO *VcpuTime
  )
{
  UINT32 Version;
  UINT32 TscToSystemMultiplier;
  INT8   TscShift;
  UINT64 CpuFreq;

  do {
    Version = VcpuTime->Version;
    MemoryFence ();
    TscToSystemMultiplier = VcpuTime->TscToSystemMultiplier;
    TscShift = VcpuTime->TscShift;
    MemoryFence ();
  } while (((Version & 1) != 0) && (Version != VcpuTime->Version));

  CpuFreq = DivU64x32 (LShiftU64 (1000000000ULL, 32), TscToSystemMultiplier);
  if (TscShift >= 0) {
      CpuFreq = RShiftU64 (CpuFreq, TscShift);
  } else {
      CpuFreq = LShiftU64 (CpuFreq, -TscShift);
  }
  return CpuFreq;
}

STATIC
VOID
XenDelay (
  IN XEN_VCPU_TIME_INFO *VcpuTimeInfo,
  IN UINT64             DelayNs
  )
{
  UINT64        Tick;
  UINT64        CpuFreq;
  UINT64        Delay;
  UINT64        DelayTick;
  UINT64        NewTick;
  RETURN_STATUS Status;

  Tick = AsmReadTsc ();

  CpuFreq = GetCpuFreq (VcpuTimeInfo);
  Status = SafeUint64Mult (DelayNs, CpuFreq, &Delay);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "XenDelay (%lu ns): delay too big in relation to CPU freq %lu Hz\n",
      DelayNs, CpuFreq));
    ASSERT_EFI_ERROR (Status);
    CpuDeadLoop ();
  }

  DelayTick = DivU64x32 (Delay, 1000000000);

  NewTick = Tick + DelayTick;

  //
  // Check for overflow
  //
  if (NewTick < Tick) {
    //
    // Overflow, wait for TSC to also overflow
    //
    while (AsmReadTsc () >= Tick) {
      CpuPause ();
    }
  }

  while (AsmReadTsc () <= NewTick) {
    CpuPause ();
  }
}


/**
  Calculate the frequency of the Local Apic Timer
**/
VOID
CalibrateLapicTimer (
  VOID
  )
{
  XEN_SHARED_INFO       *SharedInfo;
  XEN_VCPU_TIME_INFO    *VcpuTimeInfo;
  UINT32                TimerTick, TimerTick2, DiffTimer;
  UINT64                TscTick, TscTick2;
  UINT64                Freq;
  UINT64                Dividend;
  EFI_STATUS            Status;


  SharedInfo = (VOID*)((UINTN)PcdGet32 (PcdCpuLocalApicBaseAddress) + SIZE_1MB);
  Status = PhysicalAddressIdentityMapping ((EFI_PHYSICAL_ADDRESS)SharedInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "Failed to add page table entry for Xen shared info page: %r\n",
      Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  Status = MapSharedInfoPage (SharedInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to map Xen's shared info page: %r\n",
      Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  VcpuTimeInfo = &SharedInfo->VcpuInfo[0].Time;

  InitializeApicTimer (1, MAX_UINT32, TRUE, 0);
  DisableApicTimerInterrupt ();

  TimerTick = GetApicTimerCurrentCount ();
  TscTick = AsmReadTsc ();
  XenDelay (VcpuTimeInfo, 1000000ULL);
  TimerTick2 = GetApicTimerCurrentCount ();
  TscTick2 = AsmReadTsc ();


  DiffTimer = TimerTick - TimerTick2;
  Status = SafeUint64Mult (GetCpuFreq (VcpuTimeInfo), DiffTimer, &Dividend);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "overflow while calculating APIC frequency\n"));
    DEBUG ((DEBUG_ERROR, "CPU freq: %lu Hz; APIC timer tick count for 1 ms: %u\n",
      GetCpuFreq (VcpuTimeInfo), DiffTimer));
    ASSERT_EFI_ERROR (Status);
    CpuDeadLoop ();
  }

  Freq = DivU64x64Remainder (Dividend, TscTick2 - TscTick, NULL);
  DEBUG ((DEBUG_INFO, "APIC Freq % 8lu Hz\n", Freq));

  ASSERT (Freq <= MAX_UINT32);
  Status = PcdSet32S (PcdFSBClock, (UINT32)Freq);
  ASSERT_EFI_ERROR (Status);

  UnmapXenPage (SharedInfo);
}
