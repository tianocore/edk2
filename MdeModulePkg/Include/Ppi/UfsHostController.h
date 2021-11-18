/** @file

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_PEI_UFS_HOST_CONTROLLER_PPI_H_
#define _EDKII_PEI_UFS_HOST_CONTROLLER_PPI_H_

///
/// Global ID for the EDKII_UFS_HOST_CONTROLLER_PPI.
///
#define EDKII_UFS_HOST_CONTROLLER_PPI_GUID \
  { \
    0xdc54b283, 0x1a77, 0x4cd6, { 0x83, 0xbb, 0xfd, 0xda, 0x46, 0x9a, 0x2e, 0xc6 } \
  }

///
/// Forward declaration for the UFS_HOST_CONTROLLER_PPI.
///
typedef struct _EDKII_UFS_HOST_CONTROLLER_PPI EDKII_UFS_HOST_CONTROLLER_PPI;

/**
  Get the MMIO base address of UFS host controller.

  @param[in]  This               The protocol instance pointer.
  @param[in]  ControllerId       The ID of the UFS host controller.
  @param[out] MmioBar            Pointer to the UFS host controller MMIO base address.

  @retval EFI_SUCCESS            The operation succeeds.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_UFS_HC_GET_MMIO_BAR)(
  IN     EDKII_UFS_HOST_CONTROLLER_PPI    *This,
  IN     UINT8                            ControllerId,
  OUT UINTN                            *MmioBar
  );

///
/// This PPI contains a set of services to interact with the UFS host controller.
///
struct _EDKII_UFS_HOST_CONTROLLER_PPI {
  EDKII_UFS_HC_GET_MMIO_BAR    GetUfsHcMmioBar;
};

extern EFI_GUID  gEdkiiPeiUfsHostControllerPpiGuid;

#endif
