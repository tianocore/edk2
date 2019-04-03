/** @file
Misc class required EFI Device Path definitions (Ports, slots &
onboard devices)

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _MISC_DEVICE_PATH_H
#define _MISC_DEVICE_PATH_H

#pragma pack(1)

//USB
/* For reference:
#define USB1_1_STR  "ACPI(PNP0A03,0)/PCI(1D,0)."
#define USB1_2_STR  "ACPI(PNP0A03,0)/PCI(1D,1)."
#define USB1_3_STR  "ACPI(PNP0A03,0)/PCI(1D,2)."
#define USB2_1_STR  "ACPI(PNP0A03,0)/PCI(1D,7)."
*/

#define DP_ACPI { ACPI_DEVICE_PATH,\
    ACPI_DP, (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),\
   (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8), EISA_PNP_ID(0x0A03), 0 }
#define DP_PCI( device,function)  { HARDWARE_DEVICE_PATH,\
    HW_PCI_DP, (UINT8) (sizeof (PCI_DEVICE_PATH)),\
    (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8), function, device }
#define DP_END  { END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0 }}

#define DP_LPC(eisaid,function ){ ACPI_DEVICE_PATH, \
ACPI_DP,(UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),\
(UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8),EISA_PNP_ID(eisaid), function }


#pragma pack()


#endif
