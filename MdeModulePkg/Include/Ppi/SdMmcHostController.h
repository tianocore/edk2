/** @file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_PEI_SD_MMC_HOST_CONTROLLER_PPI_H_
#define _EDKII_PEI_SD_MMC_HOST_CONTROLLER_PPI_H_

///
/// Global ID for the EDKII_SD_MMC_HOST_CONTROLLER_PPI.
///
#define EDKII_SD_MMC_HOST_CONTROLLER_PPI_GUID \
  { \
    0xb30dfeed, 0x947f, 0x4396, { 0xb1, 0x5a, 0xdf, 0xbd, 0xb9, 0x16, 0xdc, 0x24 } \
  }

///
/// Forward declaration for the SD_MMC_HOST_CONTROLLER_PPI.
///
typedef struct _EDKII_SD_MMC_HOST_CONTROLLER_PPI EDKII_SD_MMC_HOST_CONTROLLER_PPI;

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
typedef
EFI_STATUS
(EFIAPI *EDKII_SD_MMC_HC_GET_MMIO_BAR)(
  IN     EDKII_SD_MMC_HOST_CONTROLLER_PPI *This,
  IN     UINT8                            ControllerId,
  IN OUT UINTN                            **MmioBar,
  OUT UINT8                            *BarNum
  );

///
/// This PPI contains a set of services to interact with the SD_MMC host controller.
///
struct _EDKII_SD_MMC_HOST_CONTROLLER_PPI {
  EDKII_SD_MMC_HC_GET_MMIO_BAR    GetSdMmcHcMmioBar;
};

extern EFI_GUID  gEdkiiPeiSdMmcHostControllerPpiGuid;

#endif
