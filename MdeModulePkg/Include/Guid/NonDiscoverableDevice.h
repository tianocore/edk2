/** @file
  GUIDs to identify devices that are not on a discoverable bus but can be
  controlled by a standard class driver

  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __NON_DISCOVERABLE_DEVICE_GUID_H__
#define __NON_DISCOVERABLE_DEVICE_GUID_H__

#define EDKII_NON_DISCOVERABLE_AHCI_DEVICE_GUID \
  { 0xC7D35798, 0xE4D2, 0x4A93, {0xB1, 0x45, 0x54, 0x88, 0x9F, 0x02, 0x58, 0x4B } }

#define EDKII_NON_DISCOVERABLE_AMBA_DEVICE_GUID \
  { 0x94440339, 0xCC93, 0x4506, {0xB4, 0xC6, 0xEE, 0x8D, 0x0F, 0x4C, 0xA1, 0x91 } }

#define EDKII_NON_DISCOVERABLE_EHCI_DEVICE_GUID \
  { 0xEAEE5615, 0x0CFD, 0x45FC, {0x87, 0x69, 0xA0, 0xD8, 0x56, 0x95, 0xAF, 0x85 } }

#define EDKII_NON_DISCOVERABLE_NVME_DEVICE_GUID \
  { 0xC5F25542, 0x2A79, 0x4A26, {0x81, 0xBB, 0x4E, 0xA6, 0x32, 0x33, 0xB3, 0x09 } }

#define EDKII_NON_DISCOVERABLE_OHCI_DEVICE_GUID \
  { 0xB20005B0, 0xBB2D, 0x496F, {0x86, 0x9C, 0x23, 0x0B, 0x44, 0x79, 0xE7, 0xD1 } }

#define EDKII_NON_DISCOVERABLE_SDHCI_DEVICE_GUID \
  { 0x1DD1D619, 0xF9B8, 0x463E, {0x86, 0x81, 0xD1, 0xDC, 0x7C, 0x07, 0xB7, 0x2C } }

#define EDKII_NON_DISCOVERABLE_UFS_DEVICE_GUID \
  { 0x2EA77912, 0x80A8, 0x4947, {0xBE, 0x69, 0xCD, 0xD0, 0x0A, 0xFB, 0xE5, 0x56 } }

#define EDKII_NON_DISCOVERABLE_UHCI_DEVICE_GUID \
  { 0xA8CDA0A2, 0x4F37, 0x4A1B, {0x8E, 0x10, 0x8E, 0xF3, 0xCC, 0x3B, 0xF3, 0xA8 } }

#define EDKII_NON_DISCOVERABLE_XHCI_DEVICE_GUID \
  { 0xB1BE0BC5, 0x6C28, 0x442D, {0xAA, 0x37, 0x15, 0x1B, 0x42, 0x57, 0xBD, 0x78 } }


extern EFI_GUID gEdkiiNonDiscoverableAhciDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableAmbaDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableEhciDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableNvmeDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableOhciDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableSdhciDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableUfsDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableUhciDeviceGuid;
extern EFI_GUID gEdkiiNonDiscoverableXhciDeviceGuid;

#endif
