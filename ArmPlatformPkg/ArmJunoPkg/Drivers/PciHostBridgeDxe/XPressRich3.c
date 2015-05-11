/** @file
*  Initialize the XPress-RICH3 PCIe Root complex
*
*  Copyright (c) 2011-2015, ARM Ltd. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "PciHostBridge.h"

#include <Protocol/Cpu.h>

#include "ArmPlatform.h"

EFI_CPU_ARCH_PROTOCOL   *mCpu;

#define PCI_BRIDGE_REVISION_ID                        1
#define CLASS_CODE_REGISTER(Class, SubClass, ProgIf)  ((Class << 16) | (SubClass << 8) | ProgIf)
#define PLDA_BRIDGE_CCR                               CLASS_CODE_REGISTER(PCI_CLASS_BRIDGE, \
                                                                          PCI_CLASS_BRIDGE_P2P, \
                                                                          PCI_IF_BRIDGE_P2P)

STATIC
VOID
SetTranslationAddressEntry (
  IN  EFI_CPU_IO2_PROTOCOL *CpuIo,
  IN UINTN                  Entry,
  IN UINT64                 SourceAddress,
  IN UINT64                 TranslatedAddress,
  IN UINT64                 TranslationSize,
  IN UINT64                 TranslationParameter
  )
{
  UINTN Log2Size = HighBitSet64 (TranslationSize);

  // Ensure the size is a power of two. Restriction form the AXI Translation logic
  // Othwerwise we increase the translation size
  if (TranslationSize != (1ULL << Log2Size)) {
    DEBUG ((EFI_D_WARN, "PCI: The size 0x%lX of the region 0x%lx has been increased to "
                        "be a power of two for the AXI translation table.\n",
                        TranslationSize, SourceAddress));
    Log2Size++;
  }

  PCIE_ROOTPORT_WRITE32 (Entry + PCI_ATR_SRC_ADDR_LOW_SIZE,
      (UINT32)SourceAddress | ((Log2Size - 1) << 1) | 0x1);
  PCIE_ROOTPORT_WRITE32 (Entry + PCI_ATR_SRC_ADDR_HI, SourceAddress >> 32);

  PCIE_ROOTPORT_WRITE32 (Entry + PCI_ATR_TRSL_ADDR_LOW, (UINT32)TranslatedAddress);
  PCIE_ROOTPORT_WRITE32 (Entry + PCI_ATR_TRSL_ADDR_HI, TranslatedAddress >> 32);

  PCIE_ROOTPORT_WRITE32 (Entry + PCI_ATR_TRSL_PARAM, TranslationParameter);
}

EFI_STATUS
HWPciRbInit (
  IN  EFI_CPU_IO2_PROTOCOL *CpuIo
  )
{
  UINT32 Value;
  UINT32 Index;
  UINTN  TranslationTable;

  PCI_TRACE ("VExpressPciRbInit()");

  PCI_TRACE ("PCIe Setting up Address Translation");

  PCIE_ROOTPORT_WRITE32 (PCIE_BAR_WIN, PCIE_BAR_WIN_SUPPORT_IO | PCIE_BAR_WIN_SUPPORT_MEM | PCIE_BAR_WIN_SUPPORT_MEM64);

  // Setup the PCI Configuration Registers
  // Offset 0a: SubClass       04 PCI-PCI Bridge
  // Offset 0b: BaseClass      06 Bridge Device
  // The Class Code register is a 24 bit and can be configured by setting up the PCIE_PCI_IDS
  // Refer [1] Chapter 13
  PCIE_ROOTPORT_WRITE32 (PCIE_PCI_IDS + PCIE_PCI_IDS_CLASSCODE_OFFSET, ((PLDA_BRIDGE_CCR << 8) | PCI_BRIDGE_REVISION_ID));

  //
  // PCIE Window 0 -> AXI4 Slave 0 Address Translations
  //
  TranslationTable = VEXPRESS_ATR_PCIE_WIN0;

  // MSI Support
  SetTranslationAddressEntry (CpuIo, TranslationTable, ARM_JUNO_GIV2M_MSI_BASE, ARM_JUNO_GIV2M_MSI_BASE,
      ARM_JUNO_GIV2M_MSI_SZ, PCI_ATR_TRSLID_AXIDEVICE);
  TranslationTable += PCI_ATR_ENTRY_SIZE;

  // System Memory Support
  SetTranslationAddressEntry (CpuIo, TranslationTable, PcdGet64 (PcdSystemMemoryBase), PcdGet64 (PcdSystemMemoryBase),
      PcdGet64 (PcdSystemMemorySize), PCI_ATR_TRSLID_AXIMEMORY);
  TranslationTable += PCI_ATR_ENTRY_SIZE;
  SetTranslationAddressEntry (CpuIo, TranslationTable, ARM_JUNO_EXTRA_SYSTEM_MEMORY_BASE, ARM_JUNO_EXTRA_SYSTEM_MEMORY_BASE,
      ARM_JUNO_EXTRA_SYSTEM_MEMORY_SZ, PCI_ATR_TRSLID_AXIMEMORY);

  //
  // PCIE Window 0 -> AXI4 Slave 0 Address Translations
  //
  TranslationTable = VEXPRESS_ATR_AXI4_SLV1;

  // PCI ECAM Support
  SetTranslationAddressEntry (CpuIo, TranslationTable, PCI_ECAM_BASE, PCI_ECAM_BASE, PCI_ECAM_SIZE, PCI_ATR_TRSLID_PCIE_CONF);
  TranslationTable += PCI_ATR_ENTRY_SIZE;

  // PCI IO Support
  SetTranslationAddressEntry (CpuIo, TranslationTable, PCI_IO_BASE, PCI_IO_BASE, PCI_IO_SIZE, PCI_ATR_TRSLID_PCIE_IO);
  TranslationTable += PCI_ATR_ENTRY_SIZE;

  // PCI MEM32 Support
  SetTranslationAddressEntry (CpuIo, TranslationTable, PCI_MEM32_BASE, PCI_MEM32_BASE, PCI_MEM32_SIZE, PCI_ATR_TRSLID_PCIE_MEMORY);
  TranslationTable += PCI_ATR_ENTRY_SIZE;

  // PCI MEM64 Support
  SetTranslationAddressEntry (CpuIo, TranslationTable, PCI_MEM64_BASE, PCI_MEM64_BASE, PCI_MEM64_SIZE, PCI_ATR_TRSLID_PCIE_MEMORY);

  // Add credits
  PCIE_ROOTPORT_WRITE32 (PCIE_VC_CRED, 0x00f0b818);
  PCIE_ROOTPORT_WRITE32 (PCIE_VC_CRED + 4, 0x1);

  // Allow ECRC
  PCIE_ROOTPORT_WRITE32 (PCIE_PEX_SPC2, 0x6006);

  // Reset controller
  PCIE_CONTROL_WRITE32 (PCIE_CONTROL_RST_CTL, PCIE_CONTROL_RST_CTL_RCPHY_REL);

  // Wait for reset
  for (Index = 0; Index < 1000; Index++) {
    gBS->Stall (1000);
    PCIE_CONTROL_READ32 (PCIE_CONTROL_RST_STS, Value);
    if ((Value & PCIE_CONTROL_RST_STS_RCPHYPLL_OUT) == PCIE_CONTROL_RST_STS_RCPHYPLL_OUT) {
      break;
    }
  }

  // Check for reset
  if (!(Value & PCIE_CONTROL_RST_STS_RCPHYPLL_OUT)) {
    DEBUG ((EFI_D_ERROR, "PCIe failed to come out of reset: %x.\n", Value));
    return EFI_NOT_READY;
  }

  gBS->Stall (1000);
  PCI_TRACE ("Checking link Status...");

  // Wait for Link Up
  for (Index = 0; Index < 1000; Index++) {
    gBS->Stall (1000);
    PCIE_ROOTPORT_READ32 (VEXPRESS_BASIC_STATUS, Value);
    if (Value & LINK_UP) {
      break;
    }
  }

  // Check for link up
  if (!(Value & LINK_UP)) {
    DEBUG ((EFI_D_ERROR, "PCIe link not up: %x.\n", Value));
    return EFI_NOT_READY;
  }

  PCIE_ROOTPORT_WRITE32 (PCIE_IMASK_LOCAL, PCIE_INT_MSI | PCIE_INT_INTx);

  return EFI_SUCCESS;
}
