/** @file

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI_H_
#define _EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI_H_

#include <Protocol/DevicePath.h>

///
/// Global ID for the EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI.
///
#define EDKII_NVME_EXPRESS_HOST_CONTROLLER_PPI_GUID \
  { \
    0xcae3aa63, 0x676f, 0x4da3, { 0xbd, 0x50, 0x6c, 0xc5, 0xed, 0xde, 0x9a, 0xad } \
  }

//
// Forward declaration for the EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI.
//
typedef struct _EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI;

/**
  Get the MMIO base address of NVM Express host controller.

  @param[in]  This                 The PPI instance pointer.
  @param[in]  ControllerId         The ID of the NVM Express host controller.
  @param[out] MmioBar              The MMIO base address of the controller.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.
  @retval EFI_NOT_FOUND            The specified NVM Express host controller not
                                   found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_NVM_EXPRESS_HC_GET_MMIO_BAR)(
  IN  EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI    *This,
  IN  UINT8                                    ControllerId,
  OUT UINTN                                    *MmioBar
  );

/**
  Get the device path of NVM Express host controller.

  @param[in]  This                 The PPI instance pointer.
  @param[in]  ControllerId         The ID of the NVM Express host controller.
  @param[out] DevicePathLength     The length of the device path in bytes specified
                                   by DevicePath.
  @param[out] DevicePath           The device path of NVM Express host controller.
                                   This field re-uses EFI Device Path Protocol as
                                   defined by Section 10.2 EFI Device Path Protocol
                                   of UEFI 2.7 Specification.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.
  @retval EFI_NOT_FOUND            The specified NVM Express host controller not
                                   found.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_NVM_EXPRESS_HC_GET_DEVICE_PATH)(
  IN  EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI    *This,
  IN  UINT8                                    ControllerId,
  OUT UINTN                                    *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL                 **DevicePath
  );

//
// This PPI contains a set of services to interact with the NVM Express host
// controller.
//
struct _EDKII_NVM_EXPRESS_HOST_CONTROLLER_PPI {
  EDKII_NVM_EXPRESS_HC_GET_MMIO_BAR       GetNvmeHcMmioBar;
  EDKII_NVM_EXPRESS_HC_GET_DEVICE_PATH    GetNvmeHcDevicePath;
};

extern EFI_GUID  gEdkiiPeiNvmExpressHostControllerPpiGuid;

#endif
