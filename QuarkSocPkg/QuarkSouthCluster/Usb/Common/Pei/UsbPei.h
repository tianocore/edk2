/** @file
Define private data structure for UHCI and EHCI.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _USB_PEI_H
#define _USB_PEI_H

#include "Ioh.h"

#define PEI_IOH_OHCI_SIGNATURE          SIGNATURE_32 ('O', 'H', 'C', 'I')
#define PEI_IOH_EHCI_SIGNATURE          SIGNATURE_32 ('E', 'H', 'C', 'I')

typedef struct {
  UINTN                   Signature;
  PEI_USB_CONTROLLER_PPI  UsbControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;
  UINTN                   MmioBase[IOH_MAX_OHCI_USB_CONTROLLERS];
} IOH_OHCI_DEVICE;

typedef struct {
  UINTN                   Signature;
  PEI_USB_CONTROLLER_PPI  UsbControllerPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;
  UINTN                   MmioBase[IOH_MAX_EHCI_USB_CONTROLLERS];
} IOH_EHCI_DEVICE;

#define IOH_OHCI_DEVICE_FROM_THIS(a) \
  CR(a, IOH_OHCI_DEVICE, UsbControllerPpi, PEI_IOH_OHCI_SIGNATURE)

#define IOH_EHCI_DEVICE_FROM_THIS(a) \
  CR (a, IOH_EHCI_DEVICE, UsbControllerPpi, PEI_IOH_EHCI_SIGNATURE)

#endif
