/** @file
  SdMmcPciHcPei driver is used to provide platform-dependent info, mainly SD/MMC
  host controller MMIO base, to upper layer SD/MMC drivers.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciHcPei.h"

EDKII_SD_MMC_HOST_CONTROLLER_PPI  mSdMmcHostControllerPpi = { GetSdMmcHcMmioBar };

EFI_PEI_PPI_DESCRIPTOR  mPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiPeiSdMmcHostControllerPpiGuid,
  &mSdMmcHostControllerPpi
};

/**
  Get the MMIO base address of SD/MMC host controller.

  @param[in]     This            The protocol instance pointer.
  @param[in]     ControllerId    The ID of the SD/MMC host controller.
  @param[in,out] MmioBar         The pointer to store the array of available
                                 SD/MMC host controller slot MMIO base addresses.
                                 The entry number of the array is specified by BarNum.
  @param[out]    BarNum          The pointer to store the supported bar number.

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
EFI_STATUS
EFIAPI
GetSdMmcHcMmioBar (
  IN     EDKII_SD_MMC_HOST_CONTROLLER_PPI  *This,
  IN     UINT8                             ControllerId,
  IN OUT UINTN                             **MmioBar,
  OUT UINT8                                *BarNum
  )
{
  SD_MMC_HC_PEI_PRIVATE_DATA  *Private;

  if ((This == NULL) || (MmioBar == NULL) || (BarNum == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Private = SD_MMC_HC_PEI_PRIVATE_DATA_FROM_THIS (This);

  if (ControllerId >= Private->TotalSdMmcHcs) {
    return EFI_INVALID_PARAMETER;
  }

  *MmioBar = &Private->MmioBar[ControllerId].MmioBarAddr[0];
  *BarNum  = (UINT8)Private->MmioBar[ControllerId].SlotNum;
  return EFI_SUCCESS;
}

/**
  The user code starts with this function.

  @param  FileHandle             Handle of the file being invoked.
  @param  PeiServices            Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            The driver is successfully initialized.
  @retval Others                 Can't initialize the driver.

**/
EFI_STATUS
EFIAPI
InitializeSdMmcHcPeim (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_BOOT_MODE               BootMode;
  EFI_STATUS                  Status;
  UINT16                      Bus;
  UINT16                      Device;
  UINT16                      Function;
  UINT32                      Size;
  UINT64                      MmioSize;
  UINT8                       SubClass;
  UINT8                       BaseClass;
  UINT8                       SlotInfo;
  UINT8                       SlotNum;
  UINT8                       FirstBar;
  UINT8                       Index;
  UINT8                       Slot;
  UINT32                      BarAddr;
  SD_MMC_HC_PEI_PRIVATE_DATA  *Private;

  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  Status = PeiServicesGetBootMode (&BootMode);
  ///
  /// We do not expose this in S3 boot path, because it is only for recovery.
  ///
  if (BootMode == BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  }

  Private = (SD_MMC_HC_PEI_PRIVATE_DATA *)AllocateZeroPool (sizeof (SD_MMC_HC_PEI_PRIVATE_DATA));
  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory for SD_MMC_HC_PEI_PRIVATE_DATA! \n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Private->Signature              = SD_MMC_HC_PEI_SIGNATURE;
  Private->SdMmcHostControllerPpi = mSdMmcHostControllerPpi;
  Private->PpiList                = mPpiList;
  Private->PpiList.Ppi            = &Private->SdMmcHostControllerPpi;

  BarAddr = PcdGet32 (PcdSdMmcPciHostControllerMmioBase);
  for (Bus = 0; Bus < 256; Bus++) {
    for (Device = 0; Device < 32; Device++) {
      for (Function = 0; Function < 8; Function++) {
        SubClass  = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x0A));
        BaseClass = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, 0x0B));

        if ((SubClass == PCI_SUBCLASS_SD_HOST_CONTROLLER) && (BaseClass == PCI_CLASS_SYSTEM_PERIPHERAL)) {
          //
          // Get the SD/MMC Pci host controller's Slot Info.
          //
          SlotInfo = PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, SD_MMC_HC_PEI_SLOT_OFFSET));
          FirstBar = (*(SD_MMC_HC_PEI_SLOT_INFO *)&SlotInfo).FirstBar;
          SlotNum  = (*(SD_MMC_HC_PEI_SLOT_INFO *)&SlotInfo).SlotNum + 1;
          ASSERT ((FirstBar + SlotNum) < MAX_SD_MMC_SLOTS);

          for (Index = 0, Slot = FirstBar; Slot < (FirstBar + SlotNum); Index++, Slot++) {
            //
            // Get the SD/MMC Pci host controller's MMIO region size.
            //
            PciAnd16 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_COMMAND_OFFSET), (UINT16) ~(EFI_PCI_COMMAND_BUS_MASTER | EFI_PCI_COMMAND_MEMORY_SPACE));
            PciWrite32 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_BASE_ADDRESSREG_OFFSET + 4 * Slot), 0xFFFFFFFF);
            Size = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_BASE_ADDRESSREG_OFFSET + 4 * Slot));

            switch (Size & 0x07) {
              case 0x0:
                //
                // Memory space: anywhere in 32 bit address space
                //
                MmioSize = (~(Size & 0xFFFFFFF0)) + 1;
                break;
              case 0x4:
                //
                // Memory space: anywhere in 64 bit address space
                //
                MmioSize = Size & 0xFFFFFFF0;
                PciWrite32 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_BASE_ADDRESSREG_OFFSET + 4), 0xFFFFFFFF);
                Size = PciRead32 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_BASE_ADDRESSREG_OFFSET + 4));
                //
                // Fix the length to support some spefic 64 bit BAR
                //
                Size |= ((UINT32)(-1) << HighBitSet32 (Size));
                //
                // Calculate the size of 64bit bar
                //
                MmioSize |= LShiftU64 ((UINT64)Size, 32);
                MmioSize  = (~(MmioSize)) + 1;
                //
                // Clean the high 32bits of this 64bit BAR to 0 as we only allow a 32bit BAR.
                //
                PciWrite32 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_BASE_ADDRESSREG_OFFSET + 4 * Slot + 4), 0);
                break;
              default:
                //
                // Unknown BAR type
                //
                ASSERT (FALSE);
                continue;
            }

            //
            // Assign resource to the SdMmc Pci host controller's MMIO BAR.
            // Enable the SdMmc Pci host controller by setting BME and MSE bits of PCI_CMD register.
            //
            PciWrite32 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_BASE_ADDRESSREG_OFFSET + 4 * Slot), BarAddr);
            PciOr16 (PCI_LIB_ADDRESS (Bus, Device, Function, PCI_COMMAND_OFFSET), (EFI_PCI_COMMAND_BUS_MASTER | EFI_PCI_COMMAND_MEMORY_SPACE));
            //
            // Record the allocated Mmio base address.
            //
            Private->MmioBar[Private->TotalSdMmcHcs].SlotNum++;
            Private->MmioBar[Private->TotalSdMmcHcs].MmioBarAddr[Index] = BarAddr;
            BarAddr                                                    += (UINT32)MmioSize;
          }

          Private->TotalSdMmcHcs++;
          ASSERT (Private->TotalSdMmcHcs < MAX_SD_MMC_HCS);
        }
      }
    }
  }

  ///
  /// Install SdMmc Host Controller PPI
  ///
  Status = PeiServicesInstallPpi (&Private->PpiList);

  ASSERT_EFI_ERROR (Status);
  return Status;
}
