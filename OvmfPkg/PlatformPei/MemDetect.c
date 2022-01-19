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
#include <PiPei.h>
#include <Register/Intel/SmramSaveStateMap.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
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
#include "Platform.h"

UINT8  mPhysMemAddressWidth;

STATIC UINT32  mS3AcpiReservedMemoryBase;
STATIC UINT32  mS3AcpiReservedMemorySize;

STATIC UINT16  mQ35TsegMbytes;

BOOLEAN  mQ35SmramAtDefaultSmbase;

UINT32  mQemuUc32Base;
UINT32  mLowerMemorySize = 0;

VOID
Q35TsegMbytesInitialization (
  VOID
  )
{
  UINT16         ExtendedTsegMbytes;
  RETURN_STATUS  PcdStatus;

  ASSERT (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID);

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

VOID
Q35SmramAtDefaultSmbaseInitialization (
  VOID
  )
{
  RETURN_STATUS  PcdStatus;

  ASSERT (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID);

  mQ35SmramAtDefaultSmbase = FALSE;
  if (FeaturePcdGet (PcdCsmEnable)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: SMRAM at default SMBASE not checked due to CSM\n",
      __FUNCTION__
      ));
  } else {
    UINTN  CtlReg;
    UINT8  CtlRegVal;

    CtlReg = DRAMC_REGISTER_Q35 (MCH_DEFAULT_SMBASE_CTL);
    PciWrite8 (CtlReg, MCH_DEFAULT_SMBASE_QUERY);
    CtlRegVal                = PciRead8 (CtlReg);
    mQ35SmramAtDefaultSmbase = (BOOLEAN)(CtlRegVal ==
                                         MCH_DEFAULT_SMBASE_IN_RAM);
    DEBUG ((
      DEBUG_INFO,
      "%a: SMRAM at default SMBASE %a\n",
      __FUNCTION__,
      mQ35SmramAtDefaultSmbase ? "found" : "not found"
      ));
  }

  PcdStatus = PcdSetBoolS (
                PcdQ35SmramAtDefaultSmbase,
                mQ35SmramAtDefaultSmbase
                );
  ASSERT_RETURN_ERROR (PcdStatus);
}

VOID
QemuUc32BaseInitialization (
  VOID
  )
{
  if (mHostBridgeDevId == 0xffff /* microvm */) {
    return;
  }

  mQemuUc32Base = PlatformQemuUc32BaseInitialization (mHostBridgeDevId, mLowerMemorySize);
}

/**
  Iterate over the RAM entries in QEMU's fw_cfg E820 RAM map that start outside
  of the 32-bit address range.

  Find the highest exclusive >=4GB RAM address, or produce memory resource
  descriptor HOBs for RAM entries that start at or above 4GB.

  @param[out] MaxAddress  If MaxAddress is NULL, then ScanOrAdd64BitE820Ram()
                          produces memory resource descriptor HOBs for RAM
                          entries that start at or above 4GB.

                          Otherwise, MaxAddress holds the highest exclusive
                          >=4GB RAM address on output. If QEMU's fw_cfg E820
                          RAM map contains no RAM entry that starts outside of
                          the 32-bit address range, then MaxAddress is exactly
                          4GB on output.

  @retval EFI_SUCCESS         The fw_cfg E820 RAM map was found and processed.

  @retval EFI_PROTOCOL_ERROR  The RAM map was found, but its size wasn't a
                              whole multiple of sizeof(EFI_E820_ENTRY64). No
                              RAM entry was processed.

  @return                     Error codes from QemuFwCfgFindFile(). No RAM
                              entry was processed.
**/
STATIC
EFI_STATUS
ScanOrAdd64BitE820Ram (
  IN BOOLEAN  AddHighHob,
  OUT UINT64  *LowMemory OPTIONAL,
  OUT UINT64  *MaxAddress OPTIONAL
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

  if (LowMemory != NULL) {
    *LowMemory = 0;
  }

  if (MaxAddress != NULL) {
    *MaxAddress = BASE_4GB;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  for (Processed = 0; Processed < FwCfgSize; Processed += sizeof E820Entry) {
    QemuFwCfgReadBytes (sizeof E820Entry, &E820Entry);
    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Base=0x%Lx Length=0x%Lx Type=%u\n",
      __FUNCTION__,
      E820Entry.BaseAddr,
      E820Entry.Length,
      E820Entry.Type
      ));
    if (E820Entry.Type == EfiAcpiAddressRangeMemory) {
      if (AddHighHob && (E820Entry.BaseAddr >= BASE_4GB)) {
        UINT64  Base;
        UINT64  End;

        //
        // Round up the start address, and round down the end address.
        //
        Base = ALIGN_VALUE (E820Entry.BaseAddr, (UINT64)EFI_PAGE_SIZE);
        End  = (E820Entry.BaseAddr + E820Entry.Length) &
               ~(UINT64)EFI_PAGE_MASK;
        if (Base < End) {
          PlatformAddMemoryRangeHob (Base, End);
          DEBUG ((
            DEBUG_VERBOSE,
            "%a: PlatformAddMemoryRangeHob [0x%Lx, 0x%Lx)\n",
            __FUNCTION__,
            Base,
            End
            ));
        }
      }

      if (MaxAddress || LowMemory) {
        UINT64  Candidate;

        Candidate = E820Entry.BaseAddr + E820Entry.Length;
        if (MaxAddress && (Candidate > *MaxAddress)) {
          *MaxAddress = Candidate;
          DEBUG ((
            DEBUG_VERBOSE,
            "%a: MaxAddress=0x%Lx\n",
            __FUNCTION__,
            *MaxAddress
            ));
        }

        if (LowMemory && (Candidate > *LowMemory) && (Candidate < BASE_4GB)) {
          *LowMemory = Candidate;
          DEBUG ((
            DEBUG_VERBOSE,
            "%a: LowMemory=0x%Lx\n",
            __FUNCTION__,
            *LowMemory
            ));
        }
      }
    }
  }

  return EFI_SUCCESS;
}

STATIC
UINT64
GetSystemMemorySizeAbove4gb (
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
  Initialize the mPhysMemAddressWidth variable, based on guest RAM size.
**/
VOID
AddressWidthInitialization (
  VOID
  )
{
  UINT64         Pci64Base;
  UINT64         Pci64Size;
  UINT64         FirstNonAddress;
  RETURN_STATUS  PcdStatus;

  Pci64Base            = 0;
  Pci64Size            = 0;
  FirstNonAddress      = PlatformGetFirstNonAddress (&Pci64Base, &Pci64Size, PcdGet64 (PcdPciMmio64Size));
  mPhysMemAddressWidth = PlatformAddressWidthInitialization (FirstNonAddress);
  PcdStatus            = PcdSet64S (PcdPciMmio64Base, Pci64Base);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdPciMmio64Size, Pci64Size);
  ASSERT_RETURN_ERROR (PcdStatus);
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
  UINT32   Pml5Entries;
  UINT32   PdpEntries;
  UINTN    TotalPages;
  UINT8    PhysicalAddressBits;

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

  PhysicalAddressBits = mPhysMemAddressWidth;
  Pml5Entries         = 1;

  if (PhysicalAddressBits > 48) {
    Pml5Entries         = (UINT32)LShiftU64 (1, PhysicalAddressBits - 48);
    PhysicalAddressBits = 48;
  }

  Pml4Entries = 1;
  if (PhysicalAddressBits > 39) {
    Pml4Entries         = (UINT32)LShiftU64 (1, PhysicalAddressBits - 39);
    PhysicalAddressBits = 39;
  }

  PdpEntries = 1;
  ASSERT (PhysicalAddressBits > 30);
  PdpEntries = (UINT32)LShiftU64 (1, PhysicalAddressBits - 30);

  //
  // Pre-allocate big pages to avoid later allocations.
  //
  if (!Page1GSupport) {
    TotalPages = ((PdpEntries + 1) * Pml4Entries + 1) * Pml5Entries + 1;
  } else {
    TotalPages = (Pml4Entries + 1) * Pml5Entries + 1;
  }

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

  LowerMemorySize = PlatformGetSystemMemorySizeBelow4gb ();
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
    LowerMemorySize           = mS3AcpiReservedMemoryBase;
  }

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
  // MEMFD_BASE_ADDRESS separates the SMRAM at the default SMBASE from the
  // normal boot permanent PEI RAM. Regarding the S3 boot path, the S3
  // permanent PEI RAM is located even higher.
  //
  if (FeaturePcdGet (PcdSmmSmramRequire) && mQ35SmramAtDefaultSmbase) {
    ASSERT (SMM_DEFAULT_SMBASE + MCH_DEFAULT_SMBASE_SIZE <= MemoryBase);
  }

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory (MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

STATIC
VOID
QemuInitializeRamBelow1gb (
  VOID
  )
{
  if (FeaturePcdGet (PcdSmmSmramRequire) && mQ35SmramAtDefaultSmbase) {
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
STATIC
VOID
QemuInitializeRam (
  VOID
  )
{
  UINT64         LowerMemorySize;
  UINT64         UpperMemorySize;
  MTRR_SETTINGS  MtrrSettings;
  EFI_STATUS     Status;

  DEBUG ((DEBUG_INFO, "%a called\n", __FUNCTION__));

  //
  // Determine total memory size available
  //
  LowerMemorySize = PlatformGetSystemMemorySizeBelow4gb ();

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
    QemuInitializeRamBelow1gb ();
  } else {
    //
    // Create memory HOBs
    //
    QemuInitializeRamBelow1gb ();

    if (FeaturePcdGet (PcdSmmSmramRequire)) {
      UINT32  TsegSize;

      TsegSize = mQ35TsegMbytes * SIZE_1MB;
      PlatformAddMemoryRangeHob (BASE_1MB, LowerMemorySize - TsegSize);
      PlatformAddReservedMemoryBaseSizeHob (
        LowerMemorySize - TsegSize,
        TsegSize,
        TRUE
        );
    } else {
      PlatformAddMemoryRangeHob (BASE_1MB, LowerMemorySize);
    }

    //
    // If QEMU presents an E820 map, then create memory HOBs for the >=4GB RAM
    // entries. Otherwise, create a single memory HOB with the flat >=4GB
    // memory size read from the CMOS.
    //
    Status = ScanOrAdd64BitE820Ram (TRUE, NULL, NULL);
    if (EFI_ERROR (Status)) {
      UpperMemorySize = GetSystemMemorySizeAbove4gb ();
      if (UpperMemorySize != 0) {
        PlatformAddMemoryBaseSizeHob (BASE_4GB, UpperMemorySize);
      }
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
  if (IsMtrrSupported () && (mHostBridgeDevId != CLOUDHV_DEVICE_ID)) {
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
               mQemuUc32Base,
               SIZE_4GB - mQemuUc32Base,
               CacheUncacheable
               );
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
  if (TdIsEnabled ()) {
    PlatformTdxPublishRamRegions ();
    return;
  }

  PlatformInitializeRamRegions (
    mQemuUc32Base,
    mHostBridgeDevId,
    FeaturePcdGet (PcdSmmSmramRequire),
    mBootMode,
    mS3Supported,
    mLowerMemorySize,
    mQ35TsegMbytes
    );

  SevInitializeRam ();

  if (mS3Supported && (mBootMode != BOOT_ON_S3_RESUME)) {
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
      (EFI_PHYSICAL_ADDRESS)(UINTN)PcdGet32 (PcdOvmfSecPageTablesBase),
      (UINT64)(UINTN)PcdGet32 (PcdOvmfSecPageTablesSize),
      EfiACPIMemoryNVS
      );

    if (MemEncryptSevEsIsEnabled ()) {
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

  if (mBootMode != BOOT_ON_S3_RESUME) {
    if (FeaturePcdGet (PcdSmmSmramRequire)) {
      UINT32  TsegSize;

      //
      // Make sure the TSEG area that we reported as a reserved memory resource
      // cannot be used for reserved memory allocations.
      //
      TsegSize = mQ35TsegMbytes * SIZE_1MB;
      BuildMemoryAllocationHob (
        PlatformGetSystemMemorySizeBelow4gb () - TsegSize,
        TsegSize,
        EfiReservedMemoryType
        );
      //
      // Similarly, allocate away the (already reserved) SMRAM at the default
      // SMBASE, if it exists.
      //
      if (mQ35SmramAtDefaultSmbase) {
        BuildMemoryAllocationHob (
          SMM_DEFAULT_SMBASE,
          MCH_DEFAULT_SMBASE_SIZE,
          EfiReservedMemoryType
          );
      }
    }
  }
}
