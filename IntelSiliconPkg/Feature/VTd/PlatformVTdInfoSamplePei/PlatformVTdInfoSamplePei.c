/** @file
  Platform VTd Info Sample PEI driver.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/VtdInfo.h>

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>

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

EFI_GUID gEdkiiSiliconInitializedPpiGuid = {0x82a72dc8, 0x61ec, 0x403e, {0xb1, 0x5a, 0x8d, 0x7a, 0x3a, 0x71, 0x84, 0x98}};

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

typedef struct {
  EFI_ACPI_DMAR_HEADER                         DmarHeader;
  //
  // VTd engine 2 - all rest
  //
  EFI_ACPI_DMAR_DRHD_HEADER                    Drhd2;
} MY_VTD_INFO_NO_IGD_PPI;

MY_VTD_INFO_NO_IGD_PPI  mPlatformVTdNoIgdSample = {
  { // DmarHeader
    { // Header
      EFI_ACPI_4_0_DMA_REMAPPING_TABLE_SIGNATURE,
      sizeof(MY_VTD_INFO_NO_IGD_PPI),
      EFI_ACPI_DMAR_REVISION,
    },
    0x26, // HostAddressWidth
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
};

EFI_PEI_PPI_DESCRIPTOR mPlatformVTdNoIgdInfoSampleDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiVTdInfoPpiGuid,
  &mPlatformVTdNoIgdSample
};

/**
  Initialize VTd register.
**/
VOID
InitDmar (
  VOID
  )
{
  UINT32              MchBar;

  DEBUG ((DEBUG_INFO, "InitDmar\n"));

  MchBar = PciRead32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_MCHBAR)) & ~BIT0;
  PciWrite32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_MCHBAR), 0xFED10000 | BIT0);
  DEBUG ((DEBUG_INFO, "MchBar - %x\n", MchBar));

  MmioWrite32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET, (UINT32)mPlatformVTdSample.Drhd2.RegisterBaseAddress | 1);
  DEBUG ((DEBUG_INFO, "VTd2 - %x\n", (MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET))));
}

/**
  Patch Graphic UMA address in RMRR and base address.
**/
EFI_PEI_PPI_DESCRIPTOR *
PatchDmar (
  VOID
  )
{
  UINT32                  MchBar;
  UINT16                  IgdMode;
  UINT16                  GttMode;
  UINT32                  IgdMemSize;
  UINT32                  GttMemSize;
  MY_VTD_INFO_PPI         *PlatformVTdSample;
  EFI_PEI_PPI_DESCRIPTOR  *PlatformVTdInfoSampleDesc;
  MY_VTD_INFO_NO_IGD_PPI  *PlatformVTdNoIgdSample;
  EFI_PEI_PPI_DESCRIPTOR  *PlatformVTdNoIgdInfoSampleDesc;

  DEBUG ((DEBUG_INFO, "PatchDmar\n"));

  if (PciRead16 (PCI_LIB_ADDRESS(0, 2, 0, 0)) != 0xFFFF) {
    PlatformVTdSample = AllocateCopyPool (sizeof(MY_VTD_INFO_PPI), &mPlatformVTdSample);
    ASSERT(PlatformVTdSample != NULL);
    PlatformVTdInfoSampleDesc = AllocateCopyPool (sizeof(EFI_PEI_PPI_DESCRIPTOR), &mPlatformVTdInfoSampleDesc);
    ASSERT(PlatformVTdInfoSampleDesc != NULL);
    PlatformVTdInfoSampleDesc->Ppi = PlatformVTdSample;

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

    PlatformVTdSample->Rmrr1.ReservedMemoryRegionBaseAddress  = (PciRead32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_TOLUD)) & ~(0x01)) - IgdMemSize - GttMemSize;
    PlatformVTdSample->Rmrr1.ReservedMemoryRegionLimitAddress = PlatformVTdSample->Rmrr1.ReservedMemoryRegionBaseAddress + IgdMemSize + GttMemSize - 1;

    ///
    /// Update DRHD structures of DmarTable
    ///
    MchBar = PciRead32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_MCHBAR)) & ~BIT0;

    if ((MmioRead32 (MchBar + R_SA_MCHBAR_VTD1_OFFSET) &~1) != 0) {
      PlatformVTdSample->Drhd1.RegisterBaseAddress = (MmioRead32 (MchBar + R_SA_MCHBAR_VTD1_OFFSET) &~1);
    } else {
      MmioWrite32 (MchBar + R_SA_MCHBAR_VTD1_OFFSET, (UINT32)PlatformVTdSample->Drhd1.RegisterBaseAddress | 1);
    }
    DEBUG ((DEBUG_INFO, "VTd1 - %x\n", (MmioRead32 (MchBar + R_SA_MCHBAR_VTD1_OFFSET))));

    if ((MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET) &~1) != 0) {
      PlatformVTdSample->Drhd2.RegisterBaseAddress = (MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET) &~1);
    } else {
      MmioWrite32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET, (UINT32)PlatformVTdSample->Drhd2.RegisterBaseAddress | 1);
    }
    DEBUG ((DEBUG_INFO, "VTd2 - %x\n", (MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET))));

    return PlatformVTdInfoSampleDesc;
  } else {
    PlatformVTdNoIgdSample = AllocateCopyPool (sizeof(MY_VTD_INFO_NO_IGD_PPI), &mPlatformVTdNoIgdSample);
    ASSERT(PlatformVTdNoIgdSample != NULL);
    PlatformVTdNoIgdInfoSampleDesc = AllocateCopyPool (sizeof(EFI_PEI_PPI_DESCRIPTOR), &mPlatformVTdNoIgdInfoSampleDesc);
    ASSERT(PlatformVTdNoIgdInfoSampleDesc != NULL);
    PlatformVTdNoIgdInfoSampleDesc->Ppi = PlatformVTdNoIgdSample;

    ///
    /// Update DRHD structures of DmarTable
    ///
    MchBar = PciRead32 (PCI_LIB_ADDRESS(0, 0, 0, R_SA_MCHBAR)) & ~BIT0;

    if ((MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET) &~1) != 0) {
      PlatformVTdNoIgdSample->Drhd2.RegisterBaseAddress = (MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET) &~1);
    } else {
      MmioWrite32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET, (UINT32)PlatformVTdNoIgdSample->Drhd2.RegisterBaseAddress | 1);
    }
    DEBUG ((DEBUG_INFO, "VTd2 - %x\n", (MmioRead32 (MchBar + R_SA_MCHBAR_VTD2_OFFSET))));

    return PlatformVTdNoIgdInfoSampleDesc;
  }
}

/**
  The callback function for SiliconInitializedPpi.
  It reinstalls VTD_INFO_PPI.

  @param[in]  PeiServices       General purpose services available to every PEIM.
  @param[in]  NotifyDescriptor  Notify that this module published.
  @param[in]  Ppi               PPI that was installed.

  @retval     EFI_SUCCESS       The function completed successfully.
**/
EFI_STATUS
EFIAPI
SiliconInitializedPpiNotifyCallback (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS               Status;
  EFI_PEI_PPI_DESCRIPTOR   *PpiDesc;

  PpiDesc = PatchDmar ();

  Status = PeiServicesReInstallPpi (&mPlatformVTdNoIgdInfoSampleDesc, PpiDesc);
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR mSiliconInitializedNotifyList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiSiliconInitializedPpiGuid,
  (EFI_PEIM_NOTIFY_ENTRY_POINT) SiliconInitializedPpiNotifyCallback
};

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
  EFI_STATUS               Status;
  BOOLEAN                  SiliconInitialized;
  VOID                     *SiliconInitializedPpi;
  EFI_PEI_PPI_DESCRIPTOR   *PpiDesc;

  SiliconInitialized = FALSE;
  //
  // Check if silicon is initialized.
  //
  Status = PeiServicesLocatePpi (
             &gEdkiiSiliconInitializedPpiGuid,
             0,
             NULL,
             &SiliconInitializedPpi
             );
  if (!EFI_ERROR(Status)) {
    SiliconInitialized = TRUE;
  }
  DEBUG ((DEBUG_INFO, "SiliconInitialized - %x\n", SiliconInitialized));
  if (!SiliconInitialized) {
    Status = PeiServicesNotifyPpi (&mSiliconInitializedNotifyList);
    InitDmar ();

    Status = PeiServicesInstallPpi (&mPlatformVTdNoIgdInfoSampleDesc);
    ASSERT_EFI_ERROR (Status);
  } else {
    PpiDesc = PatchDmar ();

    Status = PeiServicesInstallPpi (PpiDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
