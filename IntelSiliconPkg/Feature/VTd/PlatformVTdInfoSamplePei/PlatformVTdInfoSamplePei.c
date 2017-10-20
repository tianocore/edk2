/** @file
  Platform VTd Info Sample PEI driver.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Ppi/VtdInfo.h>

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>

#define R_SA_MCHBAR               (0x48)
#define R_SA_GGC                  (0x50)
#define N_SKL_SA_GGC_GGMS_OFFSET  (0x6)
#define B_SKL_SA_GGC_GGMS_MASK    (0xc0)
#define N_SKL_SA_GGC_GMS_OFFSET   (0x8)
#define B_SKL_SA_GGC_GMS_MASK     (0xff00)
#define V_SKL_SA_GGC_GGMS_8MB     3
#define R_SA_TOLUD                (0xbc)

#define R_SA_MCHBAR_VTD1_OFFSET  0x5400  ///< HW UNIT for IGD
#define R_SA_MCHBAR_VTD2_OFFSET  0x5410  ///< HW UNIT for all other - PEG, USB, SATA etc

typedef struct {
  EFI_ACPI_DMAR_HEADER                         DmarHeader;
  //
  // VTd engine 1 - integrated graphic
  //
  EFI_ACPI_DMAR_DRHD_HEADER                    Drhd1;
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER  Drhd11;
  EFI_ACPI_DMAR_PCI_PATH                       Drhd111;
  //
  // VTd engine 2 - all rest
  //
  EFI_ACPI_DMAR_DRHD_HEADER                    Drhd2;
  //
  // RMRR 1 - integrated graphic
  //
  EFI_ACPI_DMAR_RMRR_HEADER                    Rmrr1;
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER  Rmrr11;
  EFI_ACPI_DMAR_PCI_PATH                       Rmrr111;
} MY_VTD_INFO_PPI;

MY_VTD_INFO_PPI  mPlatformVTdSample = {
  { // DmarHeader
    { // Header
      EFI_ACPI_4_0_DMA_REMAPPING_TABLE_SIGNATURE,
      sizeof(MY_VTD_INFO_PPI),
      EFI_ACPI_DMAR_REVISION,
    },
    0x26, // HostAddressWidth
  },

  { // Drhd1
    { // Header
      EFI_ACPI_DMAR_TYPE_DRHD,
      sizeof(EFI_ACPI_DMAR_DRHD_HEADER) +
        sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER) +
        sizeof(EFI_ACPI_DMAR_PCI_PATH)
    },
    0, // Flags
    0, // Reserved
    0, // SegmentNumber
    0xFED90000 // RegisterBaseAddress -- TO BE PATCHED
  },
  { // Drhd11
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT,
    sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER) +
      sizeof(EFI_ACPI_DMAR_PCI_PATH),
    0, // Reserved2
    0, // EnumerationId
    0  // StartBusNumber
  },
  { // Drhd111
    2,  // Device
    0   // Function
  },

  { // Drhd2
    { // Header
      EFI_ACPI_DMAR_TYPE_DRHD,
      sizeof(EFI_ACPI_DMAR_DRHD_HEADER)
    },
    EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL, // Flags
    0, // Reserved
    0, // SegmentNumber
    0xFED91000 // RegisterBaseAddress -- TO BE PATCHED
  },

  { // Rmrr1
    { // Header
      EFI_ACPI_DMAR_TYPE_RMRR,
      sizeof(EFI_ACPI_DMAR_RMRR_HEADER) +
        sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER) +
        sizeof(EFI_ACPI_DMAR_PCI_PATH)
    },
    {0}, // Reserved
    0, // SegmentNumber
    0x0, // ReservedMemoryRegionBaseAddress -- TO BE PATCHED
    0x0 // ReservedMemoryRegionLimitAddress -- TO BE PATCHED
  },
  { // Rmrr11
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT,
    sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER) +
      sizeof(EFI_ACPI_DMAR_PCI_PATH),
    0, // Reserved2
    0, // EnumerationId
    0  // StartBusNumber
  },
  { // Rmrr111
    2,  // Device
    0   // Function
  },
};

EFI_PEI_PPI_DESCRIPTOR mPlatformVTdInfoSampleDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiVTdInfoPpiGuid,
  &mPlatformVTdSample
};

/**
  Patch Graphic UMA address in RMRR and base address.
**/
VOID
PatchDmar (
  VOID
  )
{
  UINT32              MchBar;
  UINT16              IgdMode;
  UINT16              GttMode;
  UINT32              IgdMemSize;
  UINT32              GttMemSize;

  ///
  /// Calculate IGD memsize
  ///
  IgdMode = ((PciRead16 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_GGC)) & B_SKL_SA_GGC_GMS_MASK) >> N_SKL_SA_GGC_GMS_OFFSET) & 0xFF;
  if (IgdMode < 0xF0) {
    IgdMemSize = IgdMode * 32 * (1024) * (1024);
  } else {
    IgdMemSize = 4 * (IgdMode - 0xF0 + 1) * (1024) * (1024);
  }

  ///
  /// Calculate GTT mem size
  ///
  GttMemSize = 0;
  GttMode = (PciRead16 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_GGC)) & B_SKL_SA_GGC_GGMS_MASK) >> N_SKL_SA_GGC_GGMS_OFFSET;
  if (GttMode <= V_SKL_SA_GGC_GGMS_8MB) {
    GttMemSize = (1 << GttMode) * (1024) * (1024);
  }

  mPlatformVTdSample.Rmrr1.ReservedMemoryRegionBaseAddress  = (PciRead32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_TOLUD)) & ~(0x01)) - IgdMemSize - GttMemSize;
  mPlatformVTdSample.Rmrr1.ReservedMemoryRegionLimitAddress = mPlatformVTdSample.Rmrr1.ReservedMemoryRegionBaseAddress + IgdMemSize + GttMemSize - 1;

  ///
  /// Update DRHD structures of DmarTable
  ///
  MchBar = PciRead32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_MCHBAR)) & ~BIT0;
  mPlatformVTdSample.Drhd1.RegisterBaseAddress = (MmioRead32 (MchBar + R_SA_MCHBAR_VTD1_OFFSET) &~1);
  mPlatformVTdSample.Drhd2.RegisterBaseAddress = (MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET) &~1);
}

/**
  Platform VTd Info sample driver.

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
PlatformVTdInfoSampleInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  PatchDmar ();

  Status = PeiServicesInstallPpi (&mPlatformVTdInfoSampleDesc);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
