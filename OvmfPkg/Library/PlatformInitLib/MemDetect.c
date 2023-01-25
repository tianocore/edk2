/**@file
  Memory Detection for Virtual Machines.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <IndustryStandard/E820.h>
#include <IndustryStandard/I440FxPiix4.h>
#include <IndustryStandard/Q35MchIch9.h>
#include <IndustryStandard/CloudHv.h>
#include <IndustryStandard/Xen/arch-x86/hvm/start_info.h>
#include <PiPei.h>
#include <Register/Intel/SmramSaveStateMap.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CcProbeLib.h>
#include <Library/DebugLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/MtrrLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/TdxLib.h>

#include <Library/PlatformInitLib.h>

#define MEGABYTE_SHIFT  20

VOID
EFIAPI
PlatformQemuUc32BaseInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (PlatformInfoHob->HostBridgeDevId == 0xffff /* microvm */) {
    return;
  }

  if (PlatformInfoHob->HostBridgeDevId == CLOUDHV_DEVICE_ID) {
    PlatformInfoHob->Uc32Size = CLOUDHV_MMIO_HOLE_SIZE;
    PlatformInfoHob->Uc32Base = CLOUDHV_MMIO_HOLE_ADDRESS;
    return;
  }

  PlatformGetSystemMemorySizeBelow4gb (PlatformInfoHob);

  if (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    ASSERT (PcdGet64 (PcdPciExpressBaseAddress) <= MAX_UINT32);
    ASSERT (PcdGet64 (PcdPciExpressBaseAddress) >= PlatformInfoHob->LowMemory);

    if (PlatformInfoHob->LowMemory <= BASE_2GB) {
      // Newer qemu with gigabyte aligned memory,
      // 32-bit pci mmio window is 2G -> 4G then.
      PlatformInfoHob->Uc32Base = BASE_2GB;
    } else {
      //
      // On q35, the 32-bit area that we'll mark as UC, through variable MTRRs,
      // starts at PcdPciExpressBaseAddress. The platform DSC is responsible for
      // setting PcdPciExpressBaseAddress such that describing the
      // [PcdPciExpressBaseAddress, 4GB) range require a very small number of
      // variable MTRRs (preferably 1 or 2).
      //
      PlatformInfoHob->Uc32Base = (UINT32)PcdGet64 (PcdPciExpressBaseAddress);
    }

    return;
  }

  ASSERT (PlatformInfoHob->HostBridgeDevId == INTEL_82441_DEVICE_ID);
  //
  // On i440fx, start with the [LowerMemorySize, 4GB) range. Make sure one
  // variable MTRR suffices by truncating the size to a whole power of two,
  // while keeping the end affixed to 4GB. This will round the base up.
  //
  PlatformInfoHob->Uc32Size = GetPowerOfTwo32 ((UINT32)(SIZE_4GB - PlatformInfoHob->LowMemory));
  PlatformInfoHob->Uc32Base = (UINT32)(SIZE_4GB - PlatformInfoHob->Uc32Size);
  //
  // Assuming that LowerMemorySize is at least 1 byte, Uc32Size is at most 2GB.
  // Therefore Uc32Base is at least 2GB.
  //
  ASSERT (PlatformInfoHob->Uc32Base >= BASE_2GB);

  if (PlatformInfoHob->Uc32Base != PlatformInfoHob->LowMemory) {
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: rounded UC32 base from 0x%x up to 0x%x, for "
      "an UC32 size of 0x%x\n",
      __FUNCTION__,
      PlatformInfoHob->LowMemory,
      PlatformInfoHob->Uc32Base,
      PlatformInfoHob->Uc32Size
      ));
  }
}

typedef VOID (*E820_SCAN_CALLBACK) (
  EFI_E820_ENTRY64       *E820Entry,
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  );

/**
  Store first address not used by e820 RAM entries in
  PlatformInfoHob->FirstNonAddress
**/
STATIC
VOID
PlatformGetFirstNonAddressCB (
  IN     EFI_E820_ENTRY64       *E820Entry,
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64  Candidate;

  if (E820Entry->Type != EfiAcpiAddressRangeMemory) {
    return;
  }

  Candidate = E820Entry->BaseAddr + E820Entry->Length;
  if (PlatformInfoHob->FirstNonAddress < Candidate) {
    DEBUG ((DEBUG_INFO, "%a: FirstNonAddress=0x%Lx\n", __FUNCTION__, Candidate));
    PlatformInfoHob->FirstNonAddress = Candidate;
  }
}

/**
  Store the low (below 4G) memory size in
  PlatformInfoHob->LowMemory
**/
STATIC
VOID
PlatformGetLowMemoryCB (
  IN     EFI_E820_ENTRY64       *E820Entry,
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64  Candidate;

  if (E820Entry->Type != EfiAcpiAddressRangeMemory) {
    return;
  }

  Candidate = E820Entry->BaseAddr + E820Entry->Length;
  if (Candidate >= BASE_4GB) {
    return;
  }

  if (PlatformInfoHob->LowMemory < Candidate) {
    DEBUG ((DEBUG_INFO, "%a: LowMemory=0x%Lx\n", __FUNCTION__, Candidate));
    PlatformInfoHob->LowMemory = (UINT32)Candidate;
  }
}

/**
  Create HOBs for reservations and RAM (except low memory).
**/
STATIC
VOID
PlatformAddHobCB (
  IN     EFI_E820_ENTRY64       *E820Entry,
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64  Base, End;

  Base = E820Entry->BaseAddr;
  End  = E820Entry->BaseAddr + E820Entry->Length;

  switch (E820Entry->Type) {
    case EfiAcpiAddressRangeMemory:
      if (Base >= BASE_4GB) {
        //
        // Round up the start address, and round down the end address.
        //
        Base = ALIGN_VALUE (Base, (UINT64)EFI_PAGE_SIZE);
        End  = End & ~(UINT64)EFI_PAGE_MASK;
        if (Base < End) {
          DEBUG ((DEBUG_INFO, "%a: HighMemory [0x%Lx, 0x%Lx)\n", __FUNCTION__, Base, End));
          PlatformAddMemoryRangeHob (Base, End);
        }
      }

      break;
    case EfiAcpiAddressRangeReserved:
      BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_RESERVED, 0, Base, End - Base);
      DEBUG ((DEBUG_INFO, "%a: Reserved [0x%Lx, 0x%Lx)\n", __FUNCTION__, Base, End));
      break;
    default:
      DEBUG ((
        DEBUG_WARN,
        "%a: Type %u [0x%Lx, 0x%Lx) (NOT HANDLED)\n",
        __FUNCTION__,
        E820Entry->Type,
        Base,
        End
        ));
      break;
  }
}

/**
  Check whenever the 64bit PCI MMIO window overlaps with a reservation
  from qemu.  If so move down the MMIO window to resolve the conflict.

  This happens on (virtual) AMD machines with 1TB address space,
  because the AMD IOMMU uses an address window just below 1TB.
**/
STATIC
VOID
PlatformReservationConflictCB (
  IN     EFI_E820_ENTRY64       *E820Entry,
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64  IntersectionBase;
  UINT64  IntersectionEnd;
  UINT64  NewBase;

  IntersectionBase = MAX (
                       E820Entry->BaseAddr,
                       PlatformInfoHob->PcdPciMmio64Base
                       );
  IntersectionEnd = MIN (
                      E820Entry->BaseAddr + E820Entry->Length,
                      PlatformInfoHob->PcdPciMmio64Base +
                      PlatformInfoHob->PcdPciMmio64Size
                      );

  if (IntersectionBase >= IntersectionEnd) {
    return;  // no overlap
  }

  NewBase = E820Entry->BaseAddr - PlatformInfoHob->PcdPciMmio64Size;
  NewBase = NewBase & ~(PlatformInfoHob->PcdPciMmio64Size - 1);

  DEBUG ((
    DEBUG_INFO,
    "%a: move mmio: 0x%Lx => %Lx\n",
    __FUNCTION__,
    PlatformInfoHob->PcdPciMmio64Base,
    NewBase
    ));
  PlatformInfoHob->PcdPciMmio64Base = NewBase;
}

/**
  Iterate over the entries in QEMU's fw_cfg E820 RAM map, call the
  passed callback for each entry.

  @param[in] Callback              The callback function to be called.

  @param[in out]  PlatformInfoHob  PlatformInfo struct which is passed
                                   through to the callback.

  @retval EFI_SUCCESS              The fw_cfg E820 RAM map was found and processed.

  @retval EFI_PROTOCOL_ERROR       The RAM map was found, but its size wasn't a
                                   whole multiple of sizeof(EFI_E820_ENTRY64). No
                                   RAM entry was processed.

  @return                          Error codes from QemuFwCfgFindFile(). No RAM
                                   entry was processed.
**/
STATIC
EFI_STATUS
PlatformScanE820 (
  IN      E820_SCAN_CALLBACK     Callback,
  IN OUT  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  EFI_E820_ENTRY64      E820Entry;
  UINTN                 Processed;

  Status = QemuFwCfgFindFile ("etc/e820", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize % sizeof E820Entry != 0) {
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  for (Processed = 0; Processed < FwCfgSize; Processed += sizeof E820Entry) {
    QemuFwCfgReadBytes (sizeof E820Entry, &E820Entry);
    Callback (&E820Entry, PlatformInfoHob);
  }

  return EFI_SUCCESS;
}

/**
  Returns PVH memmap

  @param Entries      Pointer to PVH memmap
  @param Count        Number of entries

  @return EFI_STATUS
**/
EFI_STATUS
GetPvhMemmapEntries (
  struct hvm_memmap_table_entry  **Entries,
  UINT32                         *Count
  )
{
  UINT32                 *PVHResetVectorData;
  struct hvm_start_info  *pvh_start_info;

  PVHResetVectorData = (VOID *)(UINTN)PcdGet32 (PcdXenPvhStartOfDayStructPtr);
  if (PVHResetVectorData == 0) {
    return EFI_NOT_FOUND;
  }

  pvh_start_info = (struct hvm_start_info *)(UINTN)PVHResetVectorData[0];

  *Entries = (struct hvm_memmap_table_entry *)(UINTN)pvh_start_info->memmap_paddr;
  *Count   = pvh_start_info->memmap_entries;

  return EFI_SUCCESS;
}

STATIC
UINT64
GetHighestSystemMemoryAddressFromPvhMemmap (
  BOOLEAN  Below4gb
  )
{
  struct hvm_memmap_table_entry  *Memmap;
  UINT32                         MemmapEntriesCount;
  struct hvm_memmap_table_entry  *Entry;
  EFI_STATUS                     Status;
  UINT32                         Loop;
  UINT64                         HighestAddress;
  UINT64                         EntryEnd;

  HighestAddress = 0;

  Status = GetPvhMemmapEntries (&Memmap, &MemmapEntriesCount);
  ASSERT_EFI_ERROR (Status);

  for (Loop = 0; Loop < MemmapEntriesCount; Loop++) {
    Entry    = Memmap + Loop;
    EntryEnd = Entry->addr + Entry->size;

    if ((Entry->type == XEN_HVM_MEMMAP_TYPE_RAM) &&
        (EntryEnd > HighestAddress))
    {
      if (Below4gb && (EntryEnd <= BASE_4GB)) {
        HighestAddress = EntryEnd;
      } else if (!Below4gb && (EntryEnd >= BASE_4GB)) {
        HighestAddress = EntryEnd;
      }
    }
  }

  return HighestAddress;
}

VOID
EFIAPI
PlatformGetSystemMemorySizeBelow4gb (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  EFI_STATUS  Status;
  UINT8       Cmos0x34;
  UINT8       Cmos0x35;

  if ((PlatformInfoHob->HostBridgeDevId == CLOUDHV_DEVICE_ID) &&
      (CcProbe () != CcGuestTypeIntelTdx))
  {
    // Get the information from PVH memmap
    PlatformInfoHob->LowMemory = (UINT32)GetHighestSystemMemoryAddressFromPvhMemmap (TRUE);
    return;
  }

  Status = PlatformScanE820 (PlatformGetLowMemoryCB, PlatformInfoHob);
  if (!EFI_ERROR (Status) && (PlatformInfoHob->LowMemory > 0)) {
    return;
  }

  //
  // CMOS 0x34/0x35 specifies the system memory above 16 MB.
  // * CMOS(0x35) is the high byte
  // * CMOS(0x34) is the low byte
  // * The size is specified in 64kb chunks
  // * Since this is memory above 16MB, the 16MB must be added
  //   into the calculation to get the total memory size.
  //

  Cmos0x34 = (UINT8)PlatformCmosRead8 (0x34);
  Cmos0x35 = (UINT8)PlatformCmosRead8 (0x35);

  PlatformInfoHob->LowMemory = (UINT32)(((UINTN)((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}

STATIC
UINT64
PlatformGetSystemMemorySizeAbove4gb (
  )
{
  UINT32  Size;
  UINTN   CmosIndex;

  //
  // CMOS 0x5b-0x5d specifies the system memory above 4GB MB.
  // * CMOS(0x5d) is the most significant size byte
  // * CMOS(0x5c) is the middle size byte
  // * CMOS(0x5b) is the least significant size byte
  // * The size is specified in 64kb chunks
  //

  Size = 0;
  for (CmosIndex = 0x5d; CmosIndex >= 0x5b; CmosIndex--) {
    Size = (UINT32)(Size << 8) + (UINT32)PlatformCmosRead8 (CmosIndex);
  }

  return LShiftU64 (Size, 16);
}

/**
  Return the highest address that DXE could possibly use, plus one.
**/
STATIC
VOID
PlatformGetFirstNonAddress (
  IN OUT  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT32                FwCfgPciMmio64Mb;
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  UINT64                HotPlugMemoryEnd;

  //
  // If QEMU presents an E820 map, then get the highest exclusive >=4GB RAM
  // address from it. This can express an address >= 4GB+1TB.
  //
  // Otherwise, get the flat size of the memory above 4GB from the CMOS (which
  // can only express a size smaller than 1TB), and add it to 4GB.
  //
  PlatformInfoHob->FirstNonAddress = BASE_4GB;
  Status                           = PlatformScanE820 (PlatformGetFirstNonAddressCB, PlatformInfoHob);
  if (EFI_ERROR (Status)) {
    PlatformInfoHob->FirstNonAddress = BASE_4GB + PlatformGetSystemMemorySizeAbove4gb ();
  }

  //
  // If DXE is 32-bit, then we're done; PciBusDxe will degrade 64-bit MMIO
  // resources to 32-bit anyway. See DegradeResource() in
  // "PciResourceSupport.c".
  //
 #ifdef MDE_CPU_IA32
  if (!FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    return;
  }

 #endif

  //
  // See if the user specified the number of megabytes for the 64-bit PCI host
  // aperture. Accept an aperture size up to 16TB.
  //
  // As signaled by the "X-" prefix, this knob is experimental, and might go
  // away at any time.
  //
  Status = QemuFwCfgParseUint32 (
             "opt/ovmf/X-PciMmio64Mb",
             FALSE,
             &FwCfgPciMmio64Mb
             );
  switch (Status) {
    case EFI_UNSUPPORTED:
    case EFI_NOT_FOUND:
      break;
    case EFI_SUCCESS:
      if (FwCfgPciMmio64Mb <= 0x1000000) {
        PlatformInfoHob->PcdPciMmio64Size = LShiftU64 (FwCfgPciMmio64Mb, 20);
        break;
      }

    //
    // fall through
    //
    default:
      DEBUG ((
        DEBUG_WARN,
        "%a: ignoring malformed 64-bit PCI host aperture size from fw_cfg\n",
        __FUNCTION__
        ));
      break;
  }

  if (PlatformInfoHob->PcdPciMmio64Size == 0) {
    if (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME) {
      DEBUG ((
        DEBUG_INFO,
        "%a: disabling 64-bit PCI host aperture\n",
        __FUNCTION__
        ));
    }

    //
    // There's nothing more to do; the amount of memory above 4GB fully
    // determines the highest address plus one. The memory hotplug area (see
    // below) plays no role for the firmware in this case.
    //
    return;
  }

  //
  // The "etc/reserved-memory-end" fw_cfg file, when present, contains an
  // absolute, exclusive end address for the memory hotplug area. This area
  // starts right at the end of the memory above 4GB. The 64-bit PCI host
  // aperture must be placed above it.
  //
  Status = QemuFwCfgFindFile (
             "etc/reserved-memory-end",
             &FwCfgItem,
             &FwCfgSize
             );
  if (!EFI_ERROR (Status) && (FwCfgSize == sizeof HotPlugMemoryEnd)) {
    QemuFwCfgSelectItem (FwCfgItem);
    QemuFwCfgReadBytes (FwCfgSize, &HotPlugMemoryEnd);
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: HotPlugMemoryEnd=0x%Lx\n",
      __FUNCTION__,
      HotPlugMemoryEnd
      ));

    ASSERT (HotPlugMemoryEnd >= PlatformInfoHob->FirstNonAddress);
    PlatformInfoHob->FirstNonAddress = HotPlugMemoryEnd;
  }

  //
  // SeaBIOS aligns both boundaries of the 64-bit PCI host aperture to 1GB, so
  // that the host can map it with 1GB hugepages. Follow suit.
  //
  PlatformInfoHob->PcdPciMmio64Base = ALIGN_VALUE (PlatformInfoHob->FirstNonAddress, (UINT64)SIZE_1GB);
  PlatformInfoHob->PcdPciMmio64Size = ALIGN_VALUE (PlatformInfoHob->PcdPciMmio64Size, (UINT64)SIZE_1GB);

  //
  // The 64-bit PCI host aperture should also be "naturally" aligned. The
  // alignment is determined by rounding the size of the aperture down to the
  // next smaller or equal power of two. That is, align the aperture by the
  // largest BAR size that can fit into it.
  //
  PlatformInfoHob->PcdPciMmio64Base = ALIGN_VALUE (PlatformInfoHob->PcdPciMmio64Base, GetPowerOfTwo64 (PlatformInfoHob->PcdPciMmio64Size));

  //
  // The useful address space ends with the 64-bit PCI host aperture.
  //
  PlatformInfoHob->FirstNonAddress = PlatformInfoHob->PcdPciMmio64Base + PlatformInfoHob->PcdPciMmio64Size;
  return;
}

/*
 * Use CPUID to figure physical address width.
 *
 * Does *not* work reliable on qemu.  For historical reasons qemu
 * returns phys-bits=40 by default even in case the host machine
 * supports less than that.
 *
 * So we apply the following rules (which can be enabled/disabled
 * using the QemuQuirk parameter) to figure whenever we can work with
 * the returned physical address width or not:
 *
 *   (1) If it is 41 or higher consider it valid.
 *   (2) If it is 40 or lower consider it valid in case it matches a
 *       known-good value for the CPU vendor, which is:
 *         ->  36 or 39 for Intel
 *         ->  40 for AMD
 *   (3) Otherwise consider it invalid.
 *
 * Recommendation: Run qemu with host-phys-bits=on.  That will make
 * sure guest phys-bits is not larger than host phys-bits.  Some
 * distro builds do that by default.
 */
VOID
EFIAPI
PlatformAddressWidthFromCpuid (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob,
  IN     BOOLEAN                QemuQuirk
  )
{
  UINT32   RegEax, RegEbx, RegEcx, RegEdx, Max;
  UINT8    PhysBits;
  CHAR8    Signature[13] = { 0 };
  BOOLEAN  Valid         = FALSE;
  BOOLEAN  Page1GSupport = FALSE;

  AsmCpuid (0x80000000, &RegEax, &RegEbx, &RegEcx, &RegEdx);
  *(UINT32 *)(Signature + 0) = RegEbx;
  *(UINT32 *)(Signature + 4) = RegEdx;
  *(UINT32 *)(Signature + 8) = RegEcx;
  Max                        = RegEax;

  if (Max >= 0x80000001) {
    AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
    if ((RegEdx & BIT26) != 0) {
      Page1GSupport = TRUE;
    }
  }

  if (Max >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysBits = (UINT8)RegEax;
  } else {
    PhysBits = 36;
  }

  if (!QemuQuirk) {
    Valid = TRUE;
  } else if (PhysBits >= 41) {
    Valid = TRUE;
  } else if (AsciiStrCmp (Signature, "GenuineIntel") == 0) {
    if ((PhysBits == 36) || (PhysBits == 39)) {
      Valid = TRUE;
    }
  } else if (AsciiStrCmp (Signature, "AuthenticAMD") == 0) {
    if (PhysBits == 40) {
      Valid = TRUE;
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Signature: '%a', PhysBits: %d, QemuQuirk: %a, Valid: %a\n",
    __FUNCTION__,
    Signature,
    PhysBits,
    QemuQuirk ? "On" : "Off",
    Valid ? "Yes" : "No"
    ));

  if (Valid) {
    if (PhysBits > 47) {
      /*
       * Avoid 5-level paging altogether for now, which limits
       * PhysBits to 48.  Also avoid using address bit 48, due to sign
       * extension we can't identity-map these addresses (and lots of
       * places in edk2 assume we have everything identity-mapped).
       * So the actual limit is 47.
       */
      DEBUG ((DEBUG_INFO, "%a: limit PhysBits to 47 (avoid 5-level paging)\n", __func__));
      PhysBits = 47;
    }

    if (!Page1GSupport && (PhysBits > 40)) {
      DEBUG ((DEBUG_INFO, "%a: limit PhysBits to 40 (no 1G pages available)\n", __func__));
      PhysBits = 40;
    }

    PlatformInfoHob->PhysMemAddressWidth = PhysBits;
    PlatformInfoHob->FirstNonAddress     = LShiftU64 (1, PlatformInfoHob->PhysMemAddressWidth);
  }
}

VOID
EFIAPI
PlatformDynamicMmioWindow (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64  AddrSpace, MmioSpace;

  AddrSpace = LShiftU64 (1, PlatformInfoHob->PhysMemAddressWidth);
  MmioSpace = LShiftU64 (1, PlatformInfoHob->PhysMemAddressWidth - 3);

  if ((PlatformInfoHob->PcdPciMmio64Size < MmioSpace) &&
      (PlatformInfoHob->PcdPciMmio64Base + MmioSpace < AddrSpace))
  {
    DEBUG ((DEBUG_INFO, "%a: using dynamic mmio window\n", __func__));
    DEBUG ((DEBUG_INFO, "%a:   Addr Space 0x%Lx (%Ld GB)\n", __func__, AddrSpace, RShiftU64 (AddrSpace, 30)));
    DEBUG ((DEBUG_INFO, "%a:   MMIO Space 0x%Lx (%Ld GB)\n", __func__, MmioSpace, RShiftU64 (MmioSpace, 30)));
    PlatformInfoHob->PcdPciMmio64Size = MmioSpace;
    PlatformInfoHob->PcdPciMmio64Base = AddrSpace - MmioSpace;
    PlatformScanE820 (PlatformReservationConflictCB, PlatformInfoHob);
  } else {
    DEBUG ((DEBUG_INFO, "%a: using classic mmio window\n", __func__));
  }

  DEBUG ((DEBUG_INFO, "%a:   Pci64 Base 0x%Lx\n", __func__, PlatformInfoHob->PcdPciMmio64Base));
  DEBUG ((DEBUG_INFO, "%a:   Pci64 Size 0x%Lx\n", __func__, PlatformInfoHob->PcdPciMmio64Size));
}

/**
  Iterate over the PCI host bridges resources information optionally provided
  in fw-cfg and find the highest address contained in the PCI MMIO windows. If
  the information is found, return the exclusive end; one past the last usable
  address.

  @param[out] PciMmioAddressEnd Pointer to one-after End Address updated with
                                information extracted from host-provided data
                                or zero if no information available or an
                                error happened

  @retval EFI_SUCCESS               PCI information was read and the output
                                    parameter updated with the last valid
                                    address in the 64-bit MMIO range.
  @retval EFI_INVALID_PARAMETER     Pointer parameter is invalid
  @retval EFI_INCOMPATIBLE_VERSION  Hardware information found in fw-cfg
                                    has an incompatible format
  @retval EFI_UNSUPPORTED           Fw-cfg is not supported, thus host
                                    provided information, if any, cannot be
                                    read
  @retval EFI_NOT_FOUND             No PCI host bridge information provided
                                    by the host.
**/
STATIC
EFI_STATUS
PlatformScanHostProvided64BitPciMmioEnd (
  OUT UINT64  *PciMmioAddressEnd
  )
{
  EFI_STATUS            Status;
  HOST_BRIDGE_INFO      HostBridge;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  UINTN                 FwCfgReadIndex;
  UINTN                 ReadDataSize;
  UINT64                Above4GMmioEnd;

  if (PciMmioAddressEnd == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *PciMmioAddressEnd = 0;
  Above4GMmioEnd     = 0;

  Status = QemuFwCfgFindFile ("etc/hardware-info", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  QemuFwCfgSelectItem (FwCfgItem);

  FwCfgReadIndex = 0;
  while (FwCfgReadIndex < FwCfgSize) {
    Status = QemuFwCfgReadNextHardwareInfoByType (
               HardwareInfoTypeHostBridge,
               sizeof (HostBridge),
               FwCfgSize,
               &HostBridge,
               &ReadDataSize,
               &FwCfgReadIndex
               );

    if (Status != EFI_SUCCESS) {
      //
      // No more data available to read in the file, break
      // loop and finish process
      //
      break;
    }

    Status = HardwareInfoPciHostBridgeLastMmioAddress (
               &HostBridge,
               ReadDataSize,
               TRUE,
               &Above4GMmioEnd
               );

    if (Status != EFI_SUCCESS) {
      //
      // Error parsing MMIO apertures and extracting last MMIO
      // address, reset PciMmioAddressEnd as if no information was
      // found, to avoid moving forward with incomplete data, and
      // bail out
      //
      DEBUG ((
        DEBUG_ERROR,
        "%a: ignoring malformed hardware information from fw_cfg\n",
        __FUNCTION__
        ));
      *PciMmioAddressEnd = 0;
      return Status;
    }

    if (Above4GMmioEnd > *PciMmioAddressEnd) {
      *PciMmioAddressEnd = Above4GMmioEnd;
    }
  }

  if (*PciMmioAddressEnd > 0) {
    //
    // Host-provided PCI information was found and a MMIO window end
    // derived from it.
    // Increase the End address by one to have the output pointing to
    // one after the address in use (exclusive end).
    //
    *PciMmioAddressEnd += 1;

    DEBUG ((
      DEBUG_INFO,
      "%a: Pci64End=0x%Lx\n",
      __FUNCTION__,
      *PciMmioAddressEnd
      ));

    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Initialize the PhysMemAddressWidth field in PlatformInfoHob based on guest RAM size.
**/
VOID
EFIAPI
PlatformAddressWidthInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT8       PhysMemAddressWidth;
  EFI_STATUS  Status;

  if (PlatformInfoHob->HostBridgeDevId == 0xffff /* microvm */) {
    PlatformAddressWidthFromCpuid (PlatformInfoHob, FALSE);
    return;
  }

  //
  // First scan host-provided hardware information to assess if the address
  // space is already known. If so, guest must use those values.
  //
  Status = PlatformScanHostProvided64BitPciMmioEnd (&PlatformInfoHob->FirstNonAddress);

  if (EFI_ERROR (Status)) {
    //
    // If the host did not provide valid hardware information leading to a
    // hard-defined 64-bit MMIO end, fold back to calculating the minimum range
    // needed.
    // As guest-physical memory size grows, the permanent PEI RAM requirements
    // are dominated by the identity-mapping page tables built by the DXE IPL.
    // The DXL IPL keys off of the physical address bits advertized in the CPU
    // HOB. To conserve memory, we calculate the minimum address width here.
    //
    PlatformGetFirstNonAddress (PlatformInfoHob);
  }

  PlatformAddressWidthFromCpuid (PlatformInfoHob, TRUE);
  if (PlatformInfoHob->PhysMemAddressWidth != 0) {
    // physical address width is known
    PlatformDynamicMmioWindow (PlatformInfoHob);
    return;
  }

  //
  // physical address width is NOT known
  //   -> do some guess work, mostly based on installed memory
  //   -> try be conservstibe to stay below the guaranteed minimum of
  //      36 phys bits (aka 64 GB).
  //
  PhysMemAddressWidth = (UINT8)HighBitSet64 (PlatformInfoHob->FirstNonAddress);

  //
  // If FirstNonAddress is not an integral power of two, then we need an
  // additional bit.
  //
  if ((PlatformInfoHob->FirstNonAddress & (PlatformInfoHob->FirstNonAddress - 1)) != 0) {
    ++PhysMemAddressWidth;
  }

  //
  // The minimum address width is 36 (covers up to and excluding 64 GB, which
  // is the maximum for Ia32 + PAE). The theoretical architecture maximum for
  // X64 long mode is 52 bits, but the DXE IPL clamps that down to 48 bits. We
  // can simply assert that here, since 48 bits are good enough for 256 TB.
  //
  if (PhysMemAddressWidth <= 36) {
    PhysMemAddressWidth = 36;
  }

 #if defined (MDE_CPU_X64)
  if (TdIsEnabled ()) {
    if (TdSharedPageMask () == (1ULL << 47)) {
      PhysMemAddressWidth = 48;
    } else {
      PhysMemAddressWidth = 52;
    }
  }

  ASSERT (PhysMemAddressWidth <= 52);
 #else
  ASSERT (PhysMemAddressWidth <= 48);
 #endif

  PlatformInfoHob->PhysMemAddressWidth = PhysMemAddressWidth;
}

STATIC
VOID
QemuInitializeRamBelow1gb (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (PlatformInfoHob->SmmSmramRequire && PlatformInfoHob->Q35SmramAtDefaultSmbase) {
    PlatformAddMemoryRangeHob (0, SMM_DEFAULT_SMBASE);
    PlatformAddReservedMemoryBaseSizeHob (
      SMM_DEFAULT_SMBASE,
      MCH_DEFAULT_SMBASE_SIZE,
      TRUE /* Cacheable */
      );
    STATIC_ASSERT (
      SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE < BASE_512KB + BASE_128KB,
      "end of SMRAM at default SMBASE ends at, or exceeds, 640KB"
      );
    PlatformAddMemoryRangeHob (
      SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE,
      BASE_512KB + BASE_128KB
      );
  } else {
    PlatformAddMemoryRangeHob (0, BASE_512KB + BASE_128KB);
  }
}

/**
  Peform Memory Detection for QEMU / KVM

**/
VOID
EFIAPI
PlatformQemuInitializeRam (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT64         UpperMemorySize;
  MTRR_SETTINGS  MtrrSettings;
  EFI_STATUS     Status;

  DEBUG ((DEBUG_INFO, "%a called\n", __FUNCTION__));

  //
  // Determine total memory size available
  //
  PlatformGetSystemMemorySizeBelow4gb (PlatformInfoHob);

  if (PlatformInfoHob->BootMode == BOOT_ON_S3_RESUME) {
    //
    // Create the following memory HOB as an exception on the S3 boot path.
    //
    // Normally we'd create memory HOBs only on the normal boot path. However,
    // CpuMpPei specifically needs such a low-memory HOB on the S3 path as
    // well, for "borrowing" a subset of it temporarily, for the AP startup
    // vector.
    //
    // CpuMpPei saves the original contents of the borrowed area in permanent
    // PEI RAM, in a backup buffer allocated with the normal PEI services.
    // CpuMpPei restores the original contents ("returns" the borrowed area) at
    // End-of-PEI. End-of-PEI in turn is emitted by S3Resume2Pei before
    // transferring control to the OS's wakeup vector in the FACS.
    //
    // We expect any other PEIMs that "borrow" memory similarly to CpuMpPei to
    // restore the original contents. Furthermore, we expect all such PEIMs
    // (CpuMpPei included) to claim the borrowed areas by producing memory
    // allocation HOBs, and to honor preexistent memory allocation HOBs when
    // looking for an area to borrow.
    //
    QemuInitializeRamBelow1gb (PlatformInfoHob);
  } else {
    //
    // Create memory HOBs
    //
    QemuInitializeRamBelow1gb (PlatformInfoHob);

    if (PlatformInfoHob->SmmSmramRequire) {
      UINT32  TsegSize;

      TsegSize = PlatformInfoHob->Q35TsegMbytes * SIZE_1MB;
      PlatformAddMemoryRangeHob (BASE_1MB, PlatformInfoHob->LowMemory - TsegSize);
      PlatformAddReservedMemoryBaseSizeHob (
        PlatformInfoHob->LowMemory - TsegSize,
        TsegSize,
        TRUE
        );
    } else {
      PlatformAddMemoryRangeHob (BASE_1MB, PlatformInfoHob->LowMemory);
    }

    //
    // If QEMU presents an E820 map, then create memory HOBs for the >=4GB RAM
    // entries. Otherwise, create a single memory HOB with the flat >=4GB
    // memory size read from the CMOS.
    //
    Status = PlatformScanE820 (PlatformAddHobCB, PlatformInfoHob);
    if (EFI_ERROR (Status)) {
      UpperMemorySize = PlatformGetSystemMemorySizeAbove4gb ();
      if (UpperMemorySize != 0) {
        PlatformAddMemoryBaseSizeHob (BASE_4GB, UpperMemorySize);
      }
    }
  }

  //
  // We'd like to keep the following ranges uncached:
  // - [640 KB, 1 MB)
  // - [Uc32Base, 4 GB)
  //
  // Everything else should be WB. Unfortunately, programming the inverse (ie.
  // keeping the default UC, and configuring the complement set of the above as
  // WB) is not reliable in general, because the end of the upper RAM can have
  // practically any alignment, and we may not have enough variable MTRRs to
  // cover it exactly.
  //
  if (IsMtrrSupported () && (PlatformInfoHob->HostBridgeDevId != CLOUDHV_DEVICE_ID)) {
    MtrrGetAllMtrrs (&MtrrSettings);

    //
    // MTRRs disabled, fixed MTRRs disabled, default type is uncached
    //
    ASSERT ((MtrrSettings.MtrrDefType & BIT11) == 0);
    ASSERT ((MtrrSettings.MtrrDefType & BIT10) == 0);
    ASSERT ((MtrrSettings.MtrrDefType & 0xFF) == 0);

    //
    // flip default type to writeback
    //
    SetMem (&MtrrSettings.Fixed, sizeof MtrrSettings.Fixed, 0x06);
    ZeroMem (&MtrrSettings.Variables, sizeof MtrrSettings.Variables);
    MtrrSettings.MtrrDefType |= BIT11 | BIT10 | 6;
    MtrrSetAllMtrrs (&MtrrSettings);

    //
    // Set memory range from 640KB to 1MB to uncacheable
    //
    Status = MtrrSetMemoryAttribute (
               BASE_512KB + BASE_128KB,
               BASE_1MB - (BASE_512KB + BASE_128KB),
               CacheUncacheable
               );
    ASSERT_EFI_ERROR (Status);

    //
    // Set the memory range from the start of the 32-bit MMIO area (32-bit PCI
    // MMIO aperture on i440fx, PCIEXBAR on q35) to 4GB as uncacheable.
    //
    Status = MtrrSetMemoryAttribute (
               PlatformInfoHob->Uc32Base,
               SIZE_4GB - PlatformInfoHob->Uc32Base,
               CacheUncacheable
               );
    ASSERT_EFI_ERROR (Status);
  }
}

VOID
EFIAPI
PlatformQemuInitializeRamForS3 (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (PlatformInfoHob->S3Supported && (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME)) {
    //
    // This is the memory range that will be used for PEI on S3 resume
    //
    BuildMemoryAllocationHob (
      PlatformInfoHob->S3AcpiReservedMemoryBase,
      PlatformInfoHob->S3AcpiReservedMemorySize,
      EfiACPIMemoryNVS
      );

    //
    // Cover the initial RAM area used as stack and temporary PEI heap.
    //
    // This is reserved as ACPI NVS so it can be used on S3 resume.
    //
    BuildMemoryAllocationHob (
      PcdGet32 (PcdOvmfSecPeiTempRamBase),
      PcdGet32 (PcdOvmfSecPeiTempRamSize),
      EfiACPIMemoryNVS
      );

    //
    // SEC stores its table of GUIDed section handlers here.
    //
    BuildMemoryAllocationHob (
      PcdGet64 (PcdGuidedExtractHandlerTableAddress),
      PcdGet32 (PcdGuidedExtractHandlerTableSize),
      EfiACPIMemoryNVS
      );

 #ifdef MDE_CPU_X64
    //
    // Reserve the initial page tables built by the reset vector code.
    //
    // Since this memory range will be used by the Reset Vector on S3
    // resume, it must be reserved as ACPI NVS.
    //
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecPageTablesBase),
      (UINT64)(UINTN)PcdGet32 (PcdOvmfSecPageTablesSize),
      EfiACPIMemoryNVS
      );

    if (PlatformInfoHob->SevEsIsEnabled) {
      //
      // If SEV-ES is enabled, reserve the GHCB-related memory area. This
      // includes the extra page table used to break down the 2MB page
      // mapping into 4KB page entries where the GHCB resides and the
      // GHCB area itself.
      //
      // Since this memory range will be used by the Reset Vector on S3
      // resume, it must be reserved as ACPI NVS.
      //
      BuildMemoryAllocationHob (
        (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecGhcbPageTableBase),
        (UINT64)(UINTN)PcdGet32 (PcdOvmfSecGhcbPageTableSize),
        EfiACPIMemoryNVS
        );
      BuildMemoryAllocationHob (
        (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecGhcbBase),
        (UINT64)(UINTN)PcdGet32 (PcdOvmfSecGhcbSize),
        EfiACPIMemoryNVS
        );
      BuildMemoryAllocationHob (
        (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecGhcbBackupBase),
        (UINT64)(UINTN)PcdGet32 (PcdOvmfSecGhcbBackupSize),
        EfiACPIMemoryNVS
        );
    }

 #endif
  }

  if (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME) {
    if (!PlatformInfoHob->SmmSmramRequire) {
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
        PlatformInfoHob->S3Supported ? EfiACPIMemoryNVS : EfiBootServicesData
        );
    }

    if (PlatformInfoHob->SmmSmramRequire) {
      UINT32  TsegSize;

      //
      // Make sure the TSEG area that we reported as a reserved memory resource
      // cannot be used for reserved memory allocations.
      //
      PlatformGetSystemMemorySizeBelow4gb (PlatformInfoHob);
      TsegSize = PlatformInfoHob->Q35TsegMbytes * SIZE_1MB;
      BuildMemoryAllocationHob (
        PlatformInfoHob->LowMemory - TsegSize,
        TsegSize,
        EfiReservedMemoryType
        );
      //
      // Similarly, allocate away the (already reserved) SMRAM at the default
      // SMBASE, if it exists.
      //
      if (PlatformInfoHob->Q35SmramAtDefaultSmbase) {
        BuildMemoryAllocationHob (
          SMM_DEFAULT_SMBASE,
          MCH_DEFAULT_SMBASE_SIZE,
          EfiReservedMemoryType
          );
      }
    }

 #ifdef MDE_CPU_X64
    if (FixedPcdGet32 (PcdOvmfWorkAreaSize) != 0) {
      //
      // Reserve the work area.
      //
      // Since this memory range will be used by the Reset Vector on S3
      // resume, it must be reserved as ACPI NVS.
      //
      // If S3 is unsupported, then various drivers might still write to the
      // work area. We ought to prevent DXE from serving allocation requests
      // such that they would overlap the work area.
      //
      BuildMemoryAllocationHob (
        (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaBase),
        (UINT64)(UINTN)FixedPcdGet32 (PcdOvmfWorkAreaSize),
        PlatformInfoHob->S3Supported ? EfiACPIMemoryNVS : EfiBootServicesData
        );
    }

 #endif
  }
}
