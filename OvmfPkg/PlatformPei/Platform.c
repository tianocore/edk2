/**@file
  Platform PEI driver

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/ResourcePublicationLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/Pci22.h>

#include "Platform.h"
#include "Cmos.h"

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIMemoryNVS,       0x004 },
  { EfiACPIReclaimMemory,   0x008 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};


EFI_PEI_PPI_DESCRIPTOR   mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};


VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddIoMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}


VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
AddUntestedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE,
    MemoryBase,
    MemorySize
    );
}


VOID
AddUntestedMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddUntestedMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
MemMapInitialization (
  EFI_PHYSICAL_ADDRESS  TopOfMemory
  )
{
  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof(mDefaultMemoryTypeInformation)
    );

  //
  // Add PCI IO Port space available for PCI resource allocations.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
    0xC000,
    0x4000
    );

  //
  // Video memory + Legacy BIOS region
  //
  AddIoMemoryRangeHob (0x0A0000, BASE_1MB);

  //
  // address       purpose   size
  // ------------  --------  -------------------------
  // max(top, 2g)  PCI MMIO  0xFC000000 - max(top, 2g)
  // 0xFC000000    gap                           44 MB
  // 0xFEC00000    IO-APIC                        4 KB
  // 0xFEC01000    gap                         1020 KB
  // 0xFED00000    HPET                           1 KB
  // 0xFED00400    gap                         1023 KB
  // 0xFEE00000    LAPIC                          1 MB
  //
  AddIoMemoryRangeHob (TopOfMemory < BASE_2GB ? BASE_2GB : TopOfMemory, 0xFC000000);
  AddIoMemoryBaseSizeHob (0xFEC00000, SIZE_4KB);
  AddIoMemoryBaseSizeHob (0xFED00000, SIZE_1KB);
  AddIoMemoryBaseSizeHob (PcdGet32(PcdCpuLocalApicBaseAddress), SIZE_1MB);
}


VOID
MiscInitialization (
  VOID
  )
{
  //
  // Disable A20 Mask
  //
  IoOr8 (0x92, BIT1);

  //
  // Build the CPU hob with 36-bit addressing and 16-bits of IO space.
  //
  BuildCpuHob (36, 16);

  //
  // If PMREGMISC/PMIOSE is set, assume the ACPI PMBA has been configured (for
  // example by Xen) and skip the setup here. This matches the logic in
  // AcpiTimerLibConstructor ().
  //
  if ((PciRead8 (PCI_LIB_ADDRESS (0, 1, 3, 0x80)) & 0x01) == 0) {
    //
    // The PEI phase should be exited with fully accessibe PIIX4 IO space:
    // 1. set PMBA
    //
    PciAndThenOr32 (
      PCI_LIB_ADDRESS (0, 1, 3, 0x40),
      (UINT32) ~0xFFC0,
      PcdGet16 (PcdAcpiPmBaseAddress)
      );

    //
    // 2. set PCICMD/IOSE
    //
    PciOr8 (
      PCI_LIB_ADDRESS (0, 1, 3, PCI_COMMAND_OFFSET),
      EFI_PCI_COMMAND_IO_SPACE
      );

    //
    // 3. set PMREGMISC/PMIOSE
    //
    PciOr8 (PCI_LIB_ADDRESS (0, 1, 3, 0x80), 0x01);
  }
}


VOID
BootModeInitialization (
  )
{
  EFI_STATUS Status;

  Status = PeiServicesSetBootMode (BOOT_WITH_FULL_CONFIGURATION);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (mPpiBootMode);
  ASSERT_EFI_ERROR (Status);
}


VOID
ReserveEmuVariableNvStore (
  )
{
  EFI_PHYSICAL_ADDRESS VariableStore;

  //
  // Allocate storage for NV variables early on so it will be
  // at a consistent address.  Since VM memory is preserved
  // across reboots, this allows the NV variable storage to survive
  // a VM reboot.
  //
  VariableStore =
    (EFI_PHYSICAL_ADDRESS)(UINTN)
      AllocateRuntimePool (
        2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize)
        );
  DEBUG ((EFI_D_INFO,
          "Reserved variable store memory: 0x%lX; size: %dkb\n",
          VariableStore,
          (2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize)) / 1024
        ));
  PcdSet64 (PcdEmuVariableNvStoreReserved, VariableStore);
}


VOID
DebugDumpCmos (
  VOID
  )
{
  UINTN  Loop;

  DEBUG ((EFI_D_INFO, "CMOS:\n"));

  for (Loop = 0; Loop < 0x80; Loop++) {
    if ((Loop % 0x10) == 0) {
      DEBUG ((EFI_D_INFO, "%02x:", Loop));
    }
    DEBUG ((EFI_D_INFO, " %02x", CmosRead8 (Loop)));
    if ((Loop % 0x10) == 0xf) {
      DEBUG ((EFI_D_INFO, "\n"));
    }
  }
}


/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_PHYSICAL_ADDRESS  TopOfMemory;

  DEBUG ((EFI_D_ERROR, "Platform PEIM Loaded\n"));

  DebugDumpCmos ();

  TopOfMemory = MemDetect ();

  InitializeXen ();

  ReserveEmuVariableNvStore ();

  PeiFvInitialization ();

  MemMapInitialization (TopOfMemory);

  MiscInitialization ();

  BootModeInitialization ();

  return EFI_SUCCESS;
}
