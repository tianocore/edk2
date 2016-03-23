/** @file

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SD_MMC_PCI_HOST_CONTROLLER_PEI_H_
#define _SD_MMC_PCI_HOST_CONTROLLER_PEI_H_

#include <PiPei.h>

#include <Ppi/MasterBootMode.h>
#include <Ppi/SdMmcHostController.h>

#include <IndustryStandard/Pci.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>

#define SD_MMC_HC_PEI_SIGNATURE    SIGNATURE_32 ('S', 'D', 'M', 'C')

#define MAX_SD_MMC_HCS             8
#define MAX_SD_MMC_SLOTS           6

//
// SD Host Controller SlotInfo Register Offset
//
#define SD_MMC_HC_PEI_SLOT_OFFSET  0x40

typedef struct {
  UINT8    FirstBar:3;        // bit 0:2
  UINT8    Reserved:1;        // bit 3
  UINT8    SlotNum:3;         // bit 4:6
  UINT8    Reserved1:1;       // bit 7
} SD_MMC_HC_PEI_SLOT_INFO;

typedef struct {
  UINTN                            SlotNum;
  UINTN                            MmioBarAddr[MAX_SD_MMC_SLOTS];
} SD_MMC_HC_PEI_BAR;

typedef struct {
  UINTN                            Signature;
  EDKII_SD_MMC_HOST_CONTROLLER_PPI SdMmcHostControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR           PpiList;
  UINTN                            TotalSdMmcHcs;
  SD_MMC_HC_PEI_BAR                MmioBar[MAX_SD_MMC_HCS];
} SD_MMC_HC_PEI_PRIVATE_DATA;

#define SD_MMC_HC_PEI_PRIVATE_DATA_FROM_THIS(a)  CR (a, SD_MMC_HC_PEI_PRIVATE_DATA, SdMmcHostControllerPpi, SD_MMC_HC_PEI_SIGNATURE)

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
  IN     EDKII_SD_MMC_HOST_CONTROLLER_PPI *This,
  IN     UINT8                            ControllerId,
  IN OUT UINTN                            **MmioBar,
     OUT UINT8                            *BarNum
  );

#endif
