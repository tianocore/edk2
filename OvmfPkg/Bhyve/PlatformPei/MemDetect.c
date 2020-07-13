/**@file
  Memory Detection for Virtual Machines.

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <IndustryStandard/E820.h>
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
#include <Library/MtrrLib.h>

#include "Platform.h"
#include "Cmos.h"

UINT8 mPhysMemAddressWidth;

STATIC UINT32 mS3AcpiReservedMemoryBase;
STATIC UINT32 mS3AcpiReservedMemorySize;

STATIC UINT16 mQ35TsegMbytes;

BOOLEAN mQ35SmramAtDefaultSmbase = FALSE;

VOID
Q35TsegMbytesInitialization (
  VOID
  )
{
  UINT16        ExtendedTsegMbytes;
  RETURN_STATUS PcdStatus;

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


UINT32
GetSystemMemorySizeBelow4gb (
  VOID
  )
{
  UINT8 Cmos0x34;
  UINT8 Cmos0x35;

  //
  // CMOS 0x34/0x35 specifies the system memory above 16 MB.
  // * CMOS(0x35) is the high byte
  // * CMOS(0x34) is the low byte
  // * The size is specified in 64kb chunks
  // * Since this is memory above 16MB, the 16MB must be added
  //   into the calculation to get the total memory size.
  //

  Cmos0x34 = (UINT8) CmosRead8 (0x34);
  Cmos0x35 = (UINT8) CmosRead8 (0x35);

  return (UINT32) (((UINTN)((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}


STATIC
UINT64
GetSystemMemorySizeAbove4gb (
  )
{
  UINT32 Size;
  UINTN  CmosIndex;

  //
  // CMOS 0x5b-0x5d specifies the system memory above 4GB MB.
  // * CMOS(0x5d) is the most significant size byte
  // * CMOS(0x5c) is the middle size byte
  // * CMOS(0x5b) is the least significant size byte
  // * The size is specified in 64kb chunks
  //

  Size = 0;
  for (CmosIndex = 0x5d; CmosIndex >= 0x5b; CmosIndex--) {
    Size = (UINT32) (Size << 8) + (UINT32) CmosRead8 (CmosIndex);
  }

  return LShiftU64 (Size, 16);
}


/**
  Return the highest address that DXE could possibly use, plus one.
**/
STATIC
UINT64
GetFirstNonAddress (
  VOID
  )
{
  UINT64               FirstNonAddress;
  UINT64               Pci64Base, Pci64Size;
  RETURN_STATUS        PcdStatus;

  FirstNonAddress = BASE_4GB + GetSystemMemorySizeAbove4gb ();

  //
  // If DXE is 32-bit, then we're done; PciBusDxe will degrade 64-bit MMIO
  // resources to 32-bit anyway. See DegradeResource() in
  // "PciResourceSupport.c".
  //
#ifdef MDE_CPU_IA32
  if (!FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    return FirstNonAddress;
  }
#endif

  //
  // Otherwise, in order to calculate the highest address plus one, we must
  // consider the 64-bit PCI host aperture too. Fetch the default size.
  //
  Pci64Size = PcdGet64 (PcdPciMmio64Size);

  if (Pci64Size == 0) {
    if (mBootMode != BOOT_ON_S3_RESUME) {
      DEBUG ((DEBUG_INFO, "%a: disabling 64-bit PCI host aperture\n",
        __FUNCTION__));
      PcdStatus = PcdSet64S (PcdPciMmio64Size, 0);
      ASSERT_RETURN_ERROR (PcdStatus);
    }

    //
    // There's nothing more to do; the amount of memory above 4GB fully
    // determines the highest address plus one. The memory hotplug area (see
    // below) plays no role for the firmware in this case.
    //
    return FirstNonAddress;
  }

  //
  // SeaBIOS aligns both boundaries of the 64-bit PCI host aperture to 1GB, so
  // that the host can map it with 1GB hugepages. Follow suit.
  //
  Pci64Base = ALIGN_VALUE (FirstNonAddress, (UINT64)SIZE_1GB);
  Pci64Size = ALIGN_VALUE (Pci64Size, (UINT64)SIZE_1GB);

  //
  // The 64-bit PCI host aperture should also be "naturally" aligned. The
  // alignment is determined by rounding the size of the aperture down to the
  // next smaller or equal power of two. That is, align the aperture by the
  // largest BAR size that can fit into it.
  //
  Pci64Base = ALIGN_VALUE (Pci64Base, GetPowerOfTwo64 (Pci64Size));

  if (mBootMode != BOOT_ON_S3_RESUME) {
    //
    // The core PciHostBridgeDxe driver will automatically add this range to
    // the GCD memory space map through our PciHostBridgeLib instance; here we
    // only need to set the PCDs.
    //
    PcdStatus = PcdSet64S (PcdPciMmio64Base, Pci64Base);
    ASSERT_RETURN_ERROR (PcdStatus);
    PcdStatus = PcdSet64S (PcdPciMmio64Size, Pci64Size);
    ASSERT_RETURN_ERROR (PcdStatus);

    DEBUG ((DEBUG_INFO, "%a: Pci64Base=0x%Lx Pci64Size=0x%Lx\n",
      __FUNCTION__, Pci64Base, Pci64Size));
  }

  //
  // The useful address space ends with the 64-bit PCI host aperture.
  //
  FirstNonAddress = Pci64Base + Pci64Size;
  return FirstNonAddress;
}


/**
  Initialize the mPhysMemAddressWidth variable, based on guest RAM size.
**/
VOID
AddressWidthInitialization (
  VOID
  )
{
  UINT64 FirstNonAddress;

  //
  // As guest-physical memory size grows, the permanent PEI RAM requirements
  // are dominated by the identity-mapping page tables built by the DXE IPL.
  // The DXL IPL keys off of the physical address bits advertized in the CPU
  // HOB. To conserve memory, we calculate the minimum address width here.
  //
  FirstNonAddress      = GetFirstNonAddress ();
  mPhysMemAddressWidth = (UINT8)HighBitSet64 (FirstNonAddress);

  //
  // If FirstNonAddress is not an integral power of two, then we need an
  // additional bit.
  //
  if ((FirstNonAddress & (FirstNonAddress - 1)) != 0) {
    ++mPhysMemAddressWidth;
  }

  //
  // The minimum address width is 36 (covers up to and excluding 64 GB, which
  // is the maximum for Ia32 + PAE). The theoretical architecture maximum for
  // X64 long mode is 52 bits, but the DXE IPL clamps that down to 48 bits. We
  // can simply assert that here, since 48 bits are good enough for 256 TB.
  //
  if (mPhysMemAddressWidth <= 36) {
    mPhysMemAddressWidth = 36;
  }
  ASSERT (mPhysMemAddressWidth <= 48);
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
  BOOLEAN Page1GSupport;
  UINT32  RegEax;
  UINT32  RegEdx;
  UINT32  Pml4Entries;
  UINT32  PdpEntries;
  UINTN   TotalPages;

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
    PdpEntries = 1 << (mPhysMemAddressWidth - 30);
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
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  UINT64                      MemorySize;
  UINT32                      LowerMemorySize;
  UINT32                      PeiMemoryCap;

  LowerMemorySize = GetSystemMemorySizeBelow4gb ();
  if (FeaturePcdGet (PcdSmmSmramRequire)) {
    //
    // TSEG is chipped from the end of low RAM
    //
    LowerMemorySize -= mQ35TsegMbytes * SIZE_1MB;
  }

  //
  // If S3 is supported, then the S3 permanent PEI memory is placed next,
  // downwards. Its size is primarily dictated by CpuMpPei. The formula below
  // is an approximation.
  //
  if (mS3Supported) {
    mS3AcpiReservedMemorySize = SIZE_512KB +
      mMaxCpuCount *
      PcdGet32 (PcdCpuApStackSize);
    mS3AcpiReservedMemoryBase = LowerMemorySize - mS3AcpiReservedMemorySize;
    LowerMemorySize = mS3AcpiReservedMemoryBase;
  }

  if (mBootMode == BOOT_ON_S3_RESUME) {
    MemoryBase = mS3AcpiReservedMemoryBase;
    MemorySize = mS3AcpiReservedMemorySize;
  } else {
    PeiMemoryCap = GetPeiMemoryCap ();
    DEBUG ((DEBUG_INFO, "%a: mPhysMemAddressWidth=%d PeiMemoryCap=%u KB\n",
      __FUNCTION__, mPhysMemAddressWidth, PeiMemoryCap >> 10));

    //
    // Determine the range of memory to use during PEI
    //
    // Technically we could lay the permanent PEI RAM over SEC's temporary
    // decompression and scratch buffer even if "secure S3" is needed, since
    // their lifetimes don't overlap. However, PeiFvInitialization() will cover
    // RAM up to PcdOvmfDecompressionScratchEnd with an EfiACPIMemoryNVS memory
    // allocation HOB, and other allocations served from the permanent PEI RAM
    // shouldn't overlap with that HOB.
    //
    MemoryBase = mS3Supported && FeaturePcdGet (PcdSmmSmramRequire) ?
      PcdGet32 (PcdOvmfDecompressionScratchEnd) :
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
  Status = PublishSystemMemory(MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);

  return Status;
}


/**
  Peform Memory Detection for QEMU / KVM

**/
STATIC
VOID
QemuInitializeRam (
  VOID
  )
{
  UINT64                      LowerMemorySize;
  UINT64                      UpperMemorySize;
  MTRR_SETTINGS               MtrrSettings;
  EFI_STATUS                  Status;

  DEBUG ((DEBUG_INFO, "%a called\n", __FUNCTION__));

  //
  // Determine total memory size available
  //
  LowerMemorySize = GetSystemMemorySizeBelow4gb ();
  UpperMemorySize = GetSystemMemorySizeAbove4gb ();

  if (mBootMode == BOOT_ON_S3_RESUME) {
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
    AddMemoryRangeHob (0, BASE_512KB + BASE_128KB);
  } else {
    //
    // Create memory HOBs
    //
    AddMemoryRangeHob (0, BASE_512KB + BASE_128KB);

    if (FeaturePcdGet (PcdSmmSmramRequire)) {
      UINT32 TsegSize;

      TsegSize = mQ35TsegMbytes * SIZE_1MB;
      AddMemoryRangeHob (BASE_1MB, LowerMemorySize - TsegSize);
      AddReservedMemoryBaseSizeHob (LowerMemorySize - TsegSize, TsegSize,
        TRUE);
    } else {
      AddMemoryRangeHob (BASE_1MB, LowerMemorySize);
    }

    if (UpperMemorySize != 0) {
      AddMemoryBaseSizeHob (BASE_4GB, UpperMemorySize);
    }
  }

  //
  // We'd like to keep the following ranges uncached:
  // - [640 KB, 1 MB)
  // - [LowerMemorySize, 4 GB)
  //
  // Everything else should be WB. Unfortunately, programming the inverse (ie.
  // keeping the default UC, and configuring the complement set of the above as
  // WB) is not reliable in general, because the end of the upper RAM can have
  // practically any alignment, and we may not have enough variable MTRRs to
  // cover it exactly.
  //
  if (IsMtrrSupported ()) {
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
    Status = MtrrSetMemoryAttribute (BASE_512KB + BASE_128KB,
               BASE_1MB - (BASE_512KB + BASE_128KB), CacheUncacheable);
    ASSERT_EFI_ERROR (Status);

    //
    // Set memory range from the "top of lower RAM" (RAM below 4GB) to 4GB as
    // uncacheable
    //
    Status = MtrrSetMemoryAttribute (LowerMemorySize,
               SIZE_4GB - LowerMemorySize, CacheUncacheable);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Publish system RAM and reserve memory regions

**/
VOID
InitializeRamRegions (
  VOID
  )
{
  QemuInitializeRam ();

  if (mS3Supported && mBootMode != BOOT_ON_S3_RESUME) {
    //
    // This is the memory range that will be used for PEI on S3 resume
    //
    BuildMemoryAllocationHob (
      mS3AcpiReservedMemoryBase,
      mS3AcpiReservedMemorySize,
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
      (EFI_PHYSICAL_ADDRESS)(UINTN) PcdGet32 (PcdOvmfSecPageTablesBase),
      (UINT64)(UINTN) PcdGet32 (PcdOvmfSecPageTablesSize),
      EfiACPIMemoryNVS
      );
#endif
  }

  if (mBootMode != BOOT_ON_S3_RESUME) {
    if (!FeaturePcdGet (PcdSmmSmramRequire)) {
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
        (VOID*)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageBase),
        (UINTN) PcdGet32 (PcdOvmfLockBoxStorageSize)
        );
      BuildMemoryAllocationHob (
        (EFI_PHYSICAL_ADDRESS)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageBase),
        (UINT64)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageSize),
        mS3Supported ? EfiACPIMemoryNVS : EfiBootServicesData
        );
    }

    if (FeaturePcdGet (PcdSmmSmramRequire)) {
      UINT32 TsegSize;

      //
      // Make sure the TSEG area that we reported as a reserved memory resource
      // cannot be used for reserved memory allocations.
      //
      TsegSize = mQ35TsegMbytes * SIZE_1MB;
      BuildMemoryAllocationHob (
        GetSystemMemorySizeBelow4gb() - TsegSize,
        TsegSize,
        EfiReservedMemoryType
        );
    }
  }
}
