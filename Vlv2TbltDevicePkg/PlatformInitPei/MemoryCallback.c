/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  MemoryCallback.c

Abstract:

  EFI 2.0 PEIM to provide the platform support functionality on the Bridgeport.

--*/

#include "PlatformEarlyInit.h"


VOID
UpdateDefaultSetupValue (
  IN  EFI_PLATFORM_INFO_HOB       *PlatformInfo
  )
{
return;
}

/**
  PEI termination callback.

  @param PeiServices         General purpose services available to every PEIM.
  @param NotifyDescriptor    Not uesed.
  @param Ppi                 Not uesed.

  @retval EFI_SUCCESS        If the interface could be successfully
                             installed.

**/
EFI_STATUS
EFIAPI    
EndOfPeiPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                  Status;
  UINT64                      MemoryTop;
  UINT64                      LowUncableBase;
  EFI_PLATFORM_INFO_HOB       *PlatformInfo;
  UINT32                      HecBaseHigh;
  EFI_BOOT_MODE               BootMode;
  EFI_PEI_HOB_POINTERS        Hob;

  Status = (*PeiServices)->GetBootMode(
                             PeiServices,
                             &BootMode
                             );

  ASSERT_EFI_ERROR (Status);

  //
  // Set the some PCI and chipset range as UC
  // And align to 1M at leaset
  //
  Hob.Raw = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  ASSERT (Hob.Raw != NULL);
  PlatformInfo = GET_GUID_HOB_DATA(Hob.Raw);

  UpdateDefaultSetupValue (PlatformInfo);

  DEBUG ((EFI_D_ERROR, "Memory TOLM: %X\n", PlatformInfo->MemData.MemTolm));
  DEBUG ((EFI_D_ERROR, "PCIE OSBASE: %lX\n", PlatformInfo->PciData.PciExpressBase));
  DEBUG (
    (EFI_D_ERROR,
    "PCIE   BASE: %lX     Size : %X\n",
    PlatformInfo->PciData.PciExpressBase,
    PlatformInfo->PciData.PciExpressSize)
    );
  DEBUG (
    (EFI_D_ERROR,
    "PCI32  BASE: %X     Limit: %X\n",
    PlatformInfo->PciData.PciResourceMem32Base,
    PlatformInfo->PciData.PciResourceMem32Limit)
    );
  DEBUG (
    (EFI_D_ERROR,
    "PCI64  BASE: %lX     Limit: %lX\n",
    PlatformInfo->PciData.PciResourceMem64Base,
    PlatformInfo->PciData.PciResourceMem64Limit)
    );
  DEBUG ((EFI_D_ERROR, "UC    START: %lX     End  : %lX\n", PlatformInfo->MemData.MemMir0, PlatformInfo->MemData.MemMir1));

  LowUncableBase = PlatformInfo->MemData.MemMaxTolm;
  LowUncableBase &= (0x0FFF00000);
  MemoryTop = (0x100000000);

  if (BootMode != BOOT_ON_S3_RESUME) {
    //
    // In BIOS, HECBASE will be always below 4GB
    //
    HecBaseHigh = (UINT32) RShiftU64 (PlatformInfo->PciData.PciExpressBase, 28);
    ASSERT (HecBaseHigh < 16);
  }

  return Status;
}

/**
  Install Firmware Volume Hob's once there is main memory

  @param PeiServices       General purpose services available to every PEIM.
  @param NotifyDescriptor  Notify that this module published.
  @param Ppi               PPI that was installed.

  @retval EFI_SUCCESS     The function completed successfully.

**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                  Status;
  EFI_BOOT_MODE               BootMode;
  EFI_CPUID_REGISTER          FeatureInfo;
  UINT8                       CpuAddressWidth;
  UINT16                      Pm1Cnt;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PLATFORM_INFO_HOB       *PlatformInfo;
  UINT32                      RootComplexBar;
  UINT32                      PmcBase;
  UINT32                      IoBase;
  UINT32                      IlbBase;
  UINT32                      SpiBase;
  UINT32                      MphyBase;

  //
  // Get Platform Info HOB
  //
  Hob.Raw = GetFirstGuidHob (&gEfiPlatformInfoGuid);
  ASSERT (Hob.Raw != NULL);
  PlatformInfo = GET_GUID_HOB_DATA(Hob.Raw);

  Status = (*PeiServices)->GetBootMode (PeiServices, &BootMode);

  //
  // Check if user wants to turn off in PEI phase
  //
  if ((BootMode != BOOT_ON_S3_RESUME) && (BootMode != BOOT_ON_FLASH_UPDATE)) {
    CheckPowerOffNow();
  } else {
    Pm1Cnt  = IoRead16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_CNT);
    Pm1Cnt &= ~B_PCH_ACPI_PM1_CNT_SLP_TYP;
    IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_CNT, Pm1Cnt);
  }

  #ifndef MINNOW2_FSP_BUILD
  //
  // Set PEI cache mode here
  //
  SetPeiCacheMode (PeiServices);
  #endif

  //
  //  Pulish memory tyoe info
  //
  PublishMemoryTypeInfo ();

  //
  // Work done if on a S3 resume
  //
  if (BootMode == BOOT_ON_S3_RESUME) {
    //
    //Program the side band packet register to send a sideband message to Punit
    //To indicate that DRAM has been initialized and PUNIT FW base address in memory.
    //
    return EFI_SUCCESS;
  }

  RootComplexBar = MmPci32( 0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, R_PCH_LPC_RCBA ) & B_PCH_LPC_RCBA_BAR;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    RootComplexBar,
    0x1000
    );
  DEBUG ((EFI_D_INFO, "RootComplexBar     : 0x%x\n", RootComplexBar));

  PmcBase = MmPci32( 0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, R_PCH_LPC_PMC_BASE ) & B_PCH_LPC_PMC_BASE_BAR;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    PmcBase,
    0x1000
    );
  DEBUG ((EFI_D_INFO, "PmcBase            : 0x%x\n", PmcBase));

  IoBase = MmPci32( 0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, R_PCH_LPC_IO_BASE ) & B_PCH_LPC_IO_BASE_BAR;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    IoBase,
    0x4000
    );
  DEBUG ((EFI_D_INFO, "IoBase             : 0x%x\n", IoBase));

  IlbBase = MmPci32( 0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, R_PCH_LPC_ILB_BASE ) & B_PCH_LPC_ILB_BASE_BAR;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    IlbBase,
    0x1000
    );
  DEBUG ((EFI_D_INFO, "IlbBase            : 0x%x\n", IlbBase));

  SpiBase = MmPci32( 0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, R_PCH_LPC_SPI_BASE ) & B_PCH_LPC_SPI_BASE_BAR;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    SpiBase,
    0x1000
    );
  DEBUG ((EFI_D_INFO, "SpiBase            : 0x%x\n", SpiBase));

  MphyBase = MmPci32( 0, DEFAULT_PCI_BUS_NUMBER_PCH, PCI_DEVICE_NUMBER_PCH_LPC, 0, R_PCH_LPC_MPHY_BASE ) & B_PCH_LPC_MPHY_BASE_BAR;
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    MphyBase,
    0x100000
    );
  DEBUG ((EFI_D_INFO, "MphyBase           : 0x%x\n", MphyBase));

  //
  // Local APIC
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    LOCAL_APIC_ADDRESS,
    0x1000
  );
  DEBUG ((EFI_D_INFO, "LOCAL_APIC_ADDRESS : 0x%x\n", LOCAL_APIC_ADDRESS));

  //
  // IO APIC
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    IO_APIC_ADDRESS,
    0x1000
  );
  DEBUG ((EFI_D_INFO, "IO_APIC_ADDRESS    : 0x%x\n", IO_APIC_ADDRESS));

  //
  // Adding the PCIE Express area to the E820 memory table as type 2 memory.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    PlatformInfo->PciData.PciExpressBase,
    PlatformInfo->PciData.PciExpressSize
    );
  DEBUG ((EFI_D_INFO, "PciExpressBase     : 0x%x\n", PlatformInfo->PciData.PciExpressBase));

  //
  // Adding the Flashpart to the E820 memory table as type 2 memory.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_FIRMWARE_DEVICE,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    FixedPcdGet32 (PcdFlashAreaBaseAddress),
    FixedPcdGet32 (PcdFlashAreaSize)
    );
  DEBUG ((EFI_D_INFO, "FLASH_BASE_ADDRESS : 0x%x\n", FixedPcdGet32 (PcdFlashAreaBaseAddress)));

  //
  // Create a CPU hand-off information
  //
  CpuAddressWidth = 32;
  AsmCpuid (EFI_CPUID_EXTENDED_FUNCTION, &FeatureInfo.RegEax, &FeatureInfo.RegEbx, &FeatureInfo.RegEcx, &FeatureInfo.RegEdx);
  if (FeatureInfo.RegEax >= EFI_CPUID_VIRT_PHYS_ADDRESS_SIZE) {
    AsmCpuid (EFI_CPUID_VIRT_PHYS_ADDRESS_SIZE, &FeatureInfo.RegEax, &FeatureInfo.RegEbx, &FeatureInfo.RegEcx, &FeatureInfo.RegEdx);
    CpuAddressWidth = (UINT8) (FeatureInfo.RegEax & 0xFF);
  }

  BuildCpuHob(CpuAddressWidth, 16);
  ASSERT_EFI_ERROR (Status);

  return Status;

}


EFI_STATUS
ValidateFvHeader (
  IN EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader
  )
{
  UINT16  *Ptr;
  UINT16  HeaderLength;
  UINT16  Checksum;

  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  if ((FwVolHeader->Revision != EFI_FVH_REVISION) ||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINT64) -1)) ||
      ((FwVolHeader->HeaderLength & 0x01) != 0)
      ) {
    return EFI_NOT_FOUND;
  }

  //
  // Verify the header checksum
  //
  HeaderLength  = (UINT16) (FwVolHeader->HeaderLength / 2);
  Ptr           = (UINT16 *) FwVolHeader;
  Checksum      = 0;
  while (HeaderLength > 0) {
    Checksum = *Ptr++;
    HeaderLength--;
  }

  if (Checksum != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
