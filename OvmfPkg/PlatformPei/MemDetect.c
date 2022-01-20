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
    if (TdIsEnabled ()) {
      Pml4Entries = 0x200;
    } else {
      Pml4Entries = 1 << (mPhysMemAddressWidth - 39);
    }

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
