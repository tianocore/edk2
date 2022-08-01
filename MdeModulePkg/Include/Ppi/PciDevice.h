/** @file

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_PCI_DEVICE_PPI_H_
#define EDKII_PCI_DEVICE_PPI_H_

#include <Protocol/PciIo.h>
#include <Protocol/DevicePath.h>

///
/// Global ID for the EDKII_PCI_DEVICE_PPI_GUID.
///
#define EDKII_PCI_DEVICE_PPI_GUID \
  { \
    0x1597ab4f, 0xd542, 0x4efe, { 0x9a, 0xf7, 0xb2, 0x44, 0xec, 0x54, 0x4c, 0x0b } \
  }

///
/// PCI Device PPI structure.
///
typedef struct {
  EFI_PCI_IO_PROTOCOL         PciIo;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
} EDKII_PCI_DEVICE_PPI;

extern EFI_GUID  gEdkiiPeiPciDevicePpiGuid;

#endif // EDKII_PCI_DEVICE_PPI_H_
