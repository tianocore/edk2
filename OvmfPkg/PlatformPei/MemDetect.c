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
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>

#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include "Platform.h"

VOID
Q35TsegMbytesInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  UINT16         ExtendedTsegMbytes;
  RETURN_STATUS  PcdStatus;

  ASSERT (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID);

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
    PlatformInfoHob->Q35TsegMbytes = PcdGet16 (PcdQ35TsegMbytes);
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
  PlatformInfoHob->Q35TsegMbytes = ExtendedTsegMbytes;
}

VOID
Q35SmramAtDefaultSmbaseInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  PcdStatus;

  ASSERT (PlatformInfoHob->HostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID);

  PlatformInfoHob->Q35SmramAtDefaultSmbase = FALSE;
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
    CtlRegVal                                = PciRead8 (CtlReg);
    PlatformInfoHob->Q35SmramAtDefaultSmbase = (BOOLEAN)(CtlRegVal ==
                                                         MCH_DEFAULT_SMBASE_IN_RAM);
    DEBUG ((
      DEBUG_INFO,
      "%a: SMRAM at default SMBASE %a\n",
      __FUNCTION__,
      PlatformInfoHob->Q35SmramAtDefaultSmbase ? "found" : "not found"
      ));
  }

  PcdStatus = PcdSetBoolS (
                PcdQ35SmramAtDefaultSmbase,
                PlatformInfoHob->Q35SmramAtDefaultSmbase
                );
  ASSERT_RETURN_ERROR (PcdStatus);
}

/**
  Initialize the PhysMemAddressWidth field in PlatformInfoHob based on guest RAM size.
**/
VOID
AddressWidthInitialization (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  RETURN_STATUS  PcdStatus;

  PlatformAddressWidthInitialization (PlatformInfoHob);

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

  if (PlatformInfoHob->PcdPciMmio64Size == 0) {
    if (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME) {
      DEBUG ((
        DEBUG_INFO,
        "%a: disabling 64-bit PCI host aperture\n",
        __FUNCTION__
        ));
      PcdStatus = PcdSet64S (PcdPciMmio64Size, 0);
      ASSERT_RETURN_ERROR (PcdStatus);
    }

    return;
  }

  if (PlatformInfoHob->BootMode != BOOT_ON_S3_RESUME) {
    //
    // The core PciHostBridgeDxe driver will automatically add this range to
    // the GCD memory space map through our PciHostBridgeLib instance; here we
    // only need to set the PCDs.
    //
    PcdStatus = PcdSet64S (PcdPciMmio64Base, PlatformInfoHob->PcdPciMmio64Base);
    ASSERT_RETURN_ERROR (PcdStatus);
    PcdStatus = PcdSet64S (PcdPciMmio64Size, PlatformInfoHob->PcdPciMmio64Size);
    ASSERT_RETURN_ERROR (PcdStatus);

    DEBUG ((
      DEBUG_INFO,
      "%a: Pci64Base=0x%Lx Pci64Size=0x%Lx\n",
      __FUNCTION__,
      PlatformInfoHob->PcdPciMmio64Base,
      PlatformInfoHob->PcdPciMmio64Size
      ));
  }
}

/**
  Calculate the cap for the permanent PEI memory.
**/
STATIC
UINT32
GetPeiMemoryCap (
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
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

  if (PlatformInfoHob->PhysMemAddressWidth <= 39) {
    Pml4Entries = 1;
    PdpEntries  = 1 << (PlatformInfoHob->PhysMemAddressWidth - 30);
    ASSERT (PdpEntries <= 0x200);
  } else {
    if (PlatformInfoHob->PhysMemAddressWidth > 48) {
      Pml4Entries = 0x200;
    } else {
      Pml4Entries = 1 << (PlatformInfoHob->PhysMemAddressWidth - 39);
    }

    ASSERT (Pml4Entries <= 0x200);
    PdpEntries = 512;
  }

  TotalPages = Page1GSupport ? Pml4Entries + 1 :
               (PdpEntries + 1) * Pml4Entries + 1;
  ASSERT (TotalPages <= 0x40201);

  //
  // Add 64 MB for miscellaneous allocations. Note that for
  // PhysMemAddressWidth values close to 36, the cap will actually be
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
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  MemoryBase;
  UINT64                MemorySize;
  UINT32                LowerMemorySize;
  UINT32                PeiMemoryCap;
  UINT32                S3AcpiReservedMemoryBase;
  UINT32                S3AcpiReservedMemorySize;

  PlatformGetSystemMemorySizeBelow4gb (PlatformInfoHob);
  LowerMemorySize = PlatformInfoHob->LowMemory;
  if (PlatformInfoHob->SmmSmramRequire) {
    //
    // TSEG is chipped from the end of low RAM
    //
    LowerMemorySize -= PlatformInfoHob->Q35TsegMbytes * SIZE_1MB;
  }

  S3AcpiReservedMemoryBase = 0;
  S3AcpiReservedMemorySize = 0;

  //
  // If S3 is supported, then the S3 permanent PEI memory is placed next,
  // downwards. Its size is primarily dictated by CpuMpPei. The formula below
  // is an approximation.
  //
  if (PlatformInfoHob->S3Supported) {
    S3AcpiReservedMemorySize = SIZE_512KB +
                               PlatformInfoHob->PcdCpuMaxLogicalProcessorNumber *
                               PcdGet32 (PcdCpuApStackSize);
    S3AcpiReservedMemoryBase = LowerMemorySize - S3AcpiReservedMemorySize;
    LowerMemorySize          = S3AcpiReservedMemoryBase;
  }

  PlatformInfoHob->S3AcpiReservedMemoryBase = S3AcpiReservedMemoryBase;
  PlatformInfoHob->S3AcpiReservedMemorySize = S3AcpiReservedMemorySize;

  if (PlatformInfoHob->BootMode == BOOT_ON_S3_RESUME) {
    MemoryBase = S3AcpiReservedMemoryBase;
    MemorySize = S3AcpiReservedMemorySize;
  } else {
    PeiMemoryCap = GetPeiMemoryCap (PlatformInfoHob);
    DEBUG ((
      DEBUG_INFO,
      "%a: PhysMemAddressWidth=%d PeiMemoryCap=%u KB\n",
      __FUNCTION__,
      PlatformInfoHob->PhysMemAddressWidth,
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
    MemoryBase = PlatformInfoHob->S3Supported && PlatformInfoHob->SmmSmramRequire ?
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
  if (PlatformInfoHob->SmmSmramRequire && PlatformInfoHob->Q35SmramAtDefaultSmbase) {
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
  IN EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  if (TdIsEnabled ()) {
    PlatformTdxPublishRamRegions ();
    return;
  }

  PlatformQemuInitializeRam (PlatformInfoHob);

  SevInitializeRam ();

  PlatformQemuInitializeRamForS3 (PlatformInfoHob);
}
