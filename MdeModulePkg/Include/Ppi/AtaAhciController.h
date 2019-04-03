/** @file

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EDKII_ATA_AHCI_HOST_CONTROLLER_PPI_H_
#define _EDKII_ATA_AHCI_HOST_CONTROLLER_PPI_H_

#include <Protocol/DevicePath.h>

///
/// Global ID for the EDKII_ATA_AHCI_HOST_CONTROLLER_PPI.
///
#define EDKII_ATA_AHCI_HOST_CONTROLLER_PPI_GUID \
  { \
    0x61dd33ea, 0x421f, 0x4cc0, { 0x89, 0x29, 0xff, 0xee, 0xa9, 0xa1, 0xa2, 0x61 } \
  }

//
// Forward declaration for the EDKII_ATA_AHCI_HOST_CONTROLLER_PPI.
//
typedef struct _EDKII_ATA_AHCI_HOST_CONTROLLER_PPI  EDKII_ATA_AHCI_HOST_CONTROLLER_PPI;

/**
  Get the MMIO base address of ATA AHCI host controller.

  @param[in]  This                 The PPI instance pointer.
  @param[in]  ControllerId         The ID of the ATA AHCI host controller.
  @param[out] MmioBar              The MMIO base address of the controller.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.
  @retval EFI_NOT_FOUND            The specified ATA AHCI host controller not found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_ATA_AHCI_HC_GET_MMIO_BAR) (
  IN  EDKII_ATA_AHCI_HOST_CONTROLLER_PPI    *This,
  IN  UINT8                                 ControllerId,
  OUT UINTN                                 *MmioBar
  );

/**
  Get the device path of ATA AHCI host controller.

  @param[in]  This                 The PPI instance pointer.
  @param[in]  ControllerId         The ID of the ATA AHCI host controller.
  @param[out] DevicePathLength     The length of the device path in bytes specified
                                   by DevicePath.
  @param[out] DevicePath           The device path of ATA AHCI host controller.
                                   This field re-uses EFI Device Path Protocol as
                                   defined by Section 10.2 EFI Device Path Protocol
                                   of UEFI 2.7 Specification.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.
  @retval EFI_NOT_FOUND            The specified ATA AHCI host controller not found.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_ATA_AHCI_HC_GET_DEVICE_PATH) (
  IN  EDKII_ATA_AHCI_HOST_CONTROLLER_PPI    *This,
  IN  UINT8                                 ControllerId,
  OUT UINTN                                 *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL              **DevicePath
  );

//
// This PPI contains a set of services to interact with the ATA AHCI host controller.
//
struct _EDKII_ATA_AHCI_HOST_CONTROLLER_PPI {
  EDKII_ATA_AHCI_HC_GET_MMIO_BAR       GetAhciHcMmioBar;
  EDKII_ATA_AHCI_HC_GET_DEVICE_PATH    GetAhciHcDevicePath;
};

extern EFI_GUID gEdkiiPeiAtaAhciHostControllerPpiGuid;

#endif
