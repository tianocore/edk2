/**@file
  Memory Detection for Virtual Machines.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <IndustryStandard/Q35MchIch9.h>
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>

#include "Platform.h"
#include "Cmos.h"

UINT8  mPhysMemAddressWidth;

STATIC UINT32  mS3AcpiReservedMemoryBase;
STATIC UINT32  mS3AcpiReservedMemorySize;

STATIC UINT16  mQ35TsegMbytes;

VOID
Q35TsegMbytesInitialization (
  VOID
  )
{
  UINT16         ExtendedTsegMbytes;
  RETURN_STATUS  PcdStatus;

  if (mHostBridgeDevId != INTEL_Q35_MCH_DEVICE_ID) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: no TSEG (SMRAM) on host bridge DID=0x%04x; "
      "only DID=0x%04x (Q35) is supported\n",
      __FUNCTION__,
      mHostBridgeDevId,
      INTEL_Q35_MCH_DEVICE_ID
      ));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  //
  // Check if QEMU offers an extended TSEG.
  //
  // This can be seen from writing MCH_EXT_TSEG_MB_QUERY to the MCH_EXT_TSEG_MB
  // register, and reading back the register.
  //
  // On a QEMU machine type that does not offer an extended TSEG, the initial
  // write overwrites whatever value a malicious guest OS may have placed in
  // the (unimplemented) register, before entering S3 or rebooting.
  // Subsequently, the read returns MCH_EXT_TSEG_MB_QUERY unchanged.
  //
  // On a QEMU machine type that offers an extended TSEG, the initial write
  // triggers an update to the register. Subsequently, the value read back
  // (which is guaranteed to differ from MCH_EXT_TSEG_MB_QUERY) tells us the
  // number of megabytes.
  //
  PciWrite16 (DRAMC_REGISTER_Q35 (MCH_EXT_TSEG_MB), MCH_EXT_TSEG_MB_QUERY);
  ExtendedTsegMbytes = PciRead16 (DRAMC_REGISTER_Q35 (MCH_EXT_TSEG_MB));
  if (ExtendedTsegMbytes == MCH_EXT_TSEG_MB_QUERY) {
    mQ35TsegMbytes = PcdGet16 (PcdQ35TsegMbytes);
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: QEMU offers an extended TSEG (%d MB)\n",
    __FUNCTION__,
    ExtendedTsegMbytes
    ));
  PcdStatus = PcdSet16S (PcdQ35TsegMbytes, ExtendedTsegMbytes);
  ASSERT_RETURN_ERROR (PcdStatus);
  mQ35TsegMbytes = ExtendedTsegMbytes;
}

STATIC
UINT64
GetHighestSystemMemoryAddress (
  BOOLEAN  Below4gb
  )
{
  EFI_E820_ENTRY64  *E820Map;
  UINT32            E820EntriesCount;
  EFI_E820_ENTRY64  *Entry;
  EFI_STATUS        Status;
  UINT32            Loop;
  UINT64            HighestAddress;
  UINT64            EntryEnd;

  HighestAddress = 0;

  Status = XenGetE820Map (&E820Map, &E820EntriesCount);
  ASSERT_EFI_ERROR (Status);

  for (Loop = 0; Loop < E820EntriesCount; Loop++) {
    Entry    = E820Map + Loop;
    EntryEnd = Entry->BaseAddr + Entry->Length;

    if ((Entry->Type == EfiAcpiAddressRangeMemory) &&
        (EntryEnd > HighestAddress))
    {
      if (Below4gb && (EntryEnd <= BASE_4GB)) {
        HighestAddress = EntryEnd;
      } else if (!Below4gb && (EntryEnd >= BASE_4GB)) {
        HighestAddress = EntryEnd;
      }
    }
  }

  //
  // Round down the end address.
  //
  return HighestAddress & ~(UINT64)EFI_PAGE_MASK;
}

UINT32
GetSystemMemorySizeBelow4gb (
  VOID
  )
{
  UINT8  Cmos0x34;
  UINT8  Cmos0x35;

  //
  // In PVH case, there is no CMOS, we have to calculate the memory size
  // from parsing the E820
  //
  if (XenPvhDetected ()) {
    UINT64  HighestAddress;

    HighestAddress = GetHighestSystemMemoryAddress (TRUE);
    ASSERT (HighestAddress > 0 && HighestAddress <= BASE_4GB);

    return (UINT32)HighestAddress;
  }

  //
  // CMOS 0x34/0x35 specifies the system memory above 16 MB.
  // * CMOS(0x35) is the high byte
  // * CMOS(0x34) is the low byte
  // * The size is specified in 64kb chunks
  // * Since this is memory above 16MB, the 16MB must be added
  //   into the calculation to get the total memory size.
  //

  Cmos0x34 = (UINT8)CmosRead8 (0x34);
  Cmos0x35 = (UINT8)CmosRead8 (0x35);

  return (UINT32)(((UINTN)((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}

/**
  Initialize the mPhysMemAddressWidth variable, based on CPUID data.
**/
VOID
AddressWidthInitialization (
  VOID
  )
{
  UINT32  RegEax;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    mPhysMemAddressWidth = (UINT8)RegEax;
  } else {
    mPhysMemAddressWidth = 36;
  }

  //
  // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
  //
  ASSERT (mPhysMemAddressWidth <= 52);
  if (mPhysMemAddressWidth > 48) {
    mPhysMemAddressWidth = 48;
  }
}

/**
  Calculate the cap for the permanent PEI memory.
**/
STATIC
UINT32
GetPeiMemoryCap (
  VOID
  )
{
  BOOLEAN  Page1GSupport;
  UINT32   RegEax;
  UINT32   RegEdx;
  UINT32   Pml4Entries;
  UINT32   PdpEntries;
  UINTN    TotalPages;

  //
  // If DXE is 32-bit, then just return the traditional 64 MB cap.
  //
 #ifdef MDE_CPU_IA32
  if (!FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    return SIZE_64MB;
  }

 #endif

  //
  // Dependent on physical address width, PEI memory allocations can be
  // dominated by the page tables built for 64-bit DXE. So we key the cap off
  // of those. The code below is based on CreateIdentityMappingPageTables() in
  // "MdeModulePkg/Core/DxeIplPeim/X64/VirtualMemory.c".
  //
  Page1GSupport = FALSE;
  if (PcdGetBool (PcdUse1GPageTable)) {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000001) {
      AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        Page1GSupport = TRUE;
      }
    }
  }

  if (mPhysMemAddressWidth <= 39) {
    Pml4Entries = 1;
    PdpEntries  = 1 << (mPhysMemAddressWidth - 30);
    ASSERT (PdpEntries <= 0x200);
  } else {
    Pml4Entries = 1 << (mPhysMemAddressWidth - 39);
    ASSERT (Pml4Entries <= 0x200);
    PdpEntries = 512;
  }

  TotalPages = Page1GSupport ? Pml4Entries + 1 :
               (PdpEntries + 1) * Pml4Entries + 1;
  ASSERT (TotalPages <= 0x40201);

  //
  // Add 64 MB for miscellaneous allocations. Note that for
  // mPhysMemAddressWidth values close to 36, the cap will actually be
  // dominated by this increment.
  //
  return (UINT32)(EFI_PAGES_TO_SIZE (TotalPages) + SIZE_64MB);
}

/**
  Publish PEI core memory

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  MemoryBase;
  UINT64                MemorySize;
  UINT32                LowerMemorySize;
  UINT32                PeiMemoryCap;

  LowerMemorySize = GetSystemMemorySizeBelow4gb ();

  if (mBootMode == BOOT_ON_S3_RESUME) {
    MemoryBase = mS3AcpiReservedMemoryBase;
    MemorySize = mS3AcpiReservedMemorySize;
  } else {
    PeiMemoryCap = GetPeiMemoryCap ();
    DEBUG ((
      DEBUG_INFO,
      "%a: mPhysMemAddressWidth=%d PeiMemoryCap=%u KB\n",
      __FUNCTION__,
      mPhysMemAddressWidth,
      PeiMemoryCap >> 10
      ));

    //
    // Determine the range of memory to use during PEI
    //
    MemoryBase =
      PcdGet32 (PcdOvmfDxeMemFvBase) + PcdGet32 (PcdOvmfDxeMemFvSize);
    MemorySize = LowerMemorySize - MemoryBase;
    if (MemorySize > PeiMemoryCap) {
      MemoryBase = LowerMemorySize - PeiMemoryCap;
      MemorySize = PeiMemoryCap;
    }
  }

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory (MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Publish system RAM and reserve memory regions

**/
VOID
InitializeRamRegions (
  VOID
  )
{
  XenPublishRamRegions ();

  if (mBootMode != BOOT_ON_S3_RESUME) {
    //
    // Reserve the lock box storage area
    //
    // Since this memory range will be used on S3 resume, it must be
    // reserved as ACPI NVS.
    //
    // If S3 is unsupported, then various drivers might still write to the
    // LockBox area. We ought to prevent DXE from serving allocation requests
    // such that they would overlap the LockBox storage.
    //
    ZeroMem (
      (VOID *)(UINTN)PcdGet32 (PcdOvmfLockBoxStorageBase),
      (UINTN)PcdGet32 (PcdOvmfLockBoxStorageSize)
      );
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfLockBoxStorageBase),
      (UINT64)(UINTN)PcdGet32 (PcdOvmfLockBoxStorageSize),
      EfiBootServicesData
      );
  }
}
