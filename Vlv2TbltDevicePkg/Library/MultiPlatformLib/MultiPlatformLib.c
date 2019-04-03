/** @file
  Multiplatform initialization.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#include <MultiPlatformLib.h>

/**
  Platform Type detection. Because the PEI globle variable
  is in the flash, it could not change directly.So use
  2 PPIs to distinguish the platform type.

  @param FfsHeader       Pointer to Firmware File System file header.
  @param PeiServices     General purpose services available to every PEIM.

  @retval EFI_SUCCESS    Memory initialization completed successfully.
  @retval Others         All other error conditions encountered result in an ASSERT.

**/
EFI_STATUS
MultiPlatformInfoInit (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB          *PlatformInfoHob
  )
{
  UINT32 PcieLength;


  PlatformInfoHob->IohSku = MmPci16(0, MC_BUS, MC_DEV, MC_FUN, PCI_DEVICE_ID_OFFSET);

  PlatformInfoHob->IohRevision = MmPci8(0, MC_BUS, MC_DEV, MC_FUN, PCI_REVISION_ID_OFFSET);

  //
  // Update ICH Type
  //
  //
  // Device ID
  //
  PlatformInfoHob->IchSku = PchLpcPciCfg16(PCI_DEVICE_ID_OFFSET);

  PlatformInfoHob->IchRevision = PchLpcPciCfg8(PCI_REVISION_ID_OFFSET);

  //
  //64MB
  //
  PcieLength = 0x04000000;

  //
  // Don't support BASE above 4GB currently.
  //
  PlatformInfoHob->PciData.PciExpressSize     = PcieLength;
  PlatformInfoHob->PciData.PciExpressBase     = PcdGet64 (PcdPciExpressBaseAddress);

  PlatformInfoHob->PciData.PciResourceMem32Base  = (UINT32) (PlatformInfoHob->PciData.PciExpressBase - RES_MEM32_MIN_LEN);
  PlatformInfoHob->PciData.PciResourceMem32Limit = (UINT32) (PlatformInfoHob->PciData.PciExpressBase -1);

  PlatformInfoHob->PciData.PciResourceMem64Base   = RES_MEM64_36_BASE;
  PlatformInfoHob->PciData.PciResourceMem64Limit  = RES_MEM64_36_LIMIT;
  PlatformInfoHob->CpuData.CpuAddressWidth        = 36;

  PlatformInfoHob->MemData.MemMir0 = PlatformInfoHob->PciData.PciResourceMem64Base;
  PlatformInfoHob->MemData.MemMir1 = PlatformInfoHob->PciData.PciResourceMem64Limit + 1;

  PlatformInfoHob->PciData.PciResourceMinSecBus  = 1;  //can be changed by SystemConfiguration->PciMinSecondaryBus;

  //
  // Set MemMaxTolm to the lowest address between PCIe Base and PCI32 Base.
  //
  if (PlatformInfoHob->PciData.PciExpressBase > PlatformInfoHob->PciData.PciResourceMem32Base ) {
    PlatformInfoHob->MemData.MemMaxTolm = (UINT32) PlatformInfoHob->PciData.PciResourceMem32Base;
  } else {
    PlatformInfoHob->MemData.MemMaxTolm = (UINT32) PlatformInfoHob->PciData.PciExpressBase;
  }
  PlatformInfoHob->MemData.MemTolm = PlatformInfoHob->MemData.MemMaxTolm;

  //
  // Platform PCI MMIO Size in unit of 1MB.
  //
  PlatformInfoHob->MemData.MmioSize = 0x1000 - (UINT16)(PlatformInfoHob->MemData.MemMaxTolm >> 20);

  //
  // Enable ICH IOAPIC
  //
  PlatformInfoHob->SysData.SysIoApicEnable  = ICH_IOAPIC;

  DEBUG ((EFI_D_ERROR, "PlatformFlavor is %x (%x=tablet,%x=mobile,%x=desktop)\n",
    PlatformInfoHob->PlatformFlavor,
    FlavorTablet,
    FlavorMobile,
    FlavorDesktop));

  //
  // Get Platform Info and fill the Hob.
  //
  PlatformInfoHob->RevisonId = PLATFORM_INFO_HOB_REVISION;

  //
  // Get GPIO table
  //
  MultiPlatformGpioTableInit (PeiServices, PlatformInfoHob);

  //
  // Program GPIO
  //
  MultiPlatformGpioProgram (PeiServices, PlatformInfoHob);

  //
  // Update OemId
  //
  InitializeBoardOemId (PeiServices, PlatformInfoHob);
  InitializeBoardSsidSvid (PeiServices, PlatformInfoHob);

  return EFI_SUCCESS;
}
