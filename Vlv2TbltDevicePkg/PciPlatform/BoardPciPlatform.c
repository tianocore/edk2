/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


    BoardPciPlatform.c

Abstract:

Revision History:
--*/


#include "PciPlatform.h"
#include "PchRegs.h"
#include "VlvAccess.h"

#define R_INTEL_LAN_VENDOR_ID           0x00
#define   V_INTEL_LAN_VENDOR_ID         0x8086     // INTEL 82574 Gbe Controller Vendor ID
#define R_INTEL_LAN_DEVICE_ID           0x02
#define   V_INTEL_LAN_DEVICE_ID         0x153E     // INTEL 82574 Gbe Controller Device ID

//
// Global variables for Option ROMs
//

#define ONBOARD_VIDEO_OPTION_ROM_FILE_GUID \
{ 0xF2FE1FAA, 0xF04A, 0x4ba1, 0xAE, 0x73, 0xD1, 0x84, 0x6A, 0x6C, 0xD6, 0xD8 }

#define PXE_UNDI_OPTION_ROM_INTELPXE_GUID \
{0x49F2C48B, 0x4D8E, 0x4238, 0x8D, 0x82, 0x9B, 0x27, 0xF4, 0x38, 0x44, 0xB0}

#define SATA_AHCI_ROM_GUID \
  {0x592bfc62, 0xd817, 0x4d1a, 0x86, 0xf8, 0x33, 0x33, 0x4c, 0x9e, 0x90, 0xd8}

#define NULL_ROM_FILE_GUID \
{ 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

PCI_OPTION_ROM_TABLE mPciOptionRomTable[] = {
  { ONBOARD_VIDEO_OPTION_ROM_FILE_GUID,     0, 0, 0x02, 0, IGD_VID, IGD_DID_VLV_A0, 0},
  { ONBOARD_VIDEO_OPTION_ROM_FILE_GUID,     0, 0, 0x02, 0, IGD_VID, IGD_DID, 0},  // Desktop/Mobile IGD
  { ONBOARD_VIDEO_OPTION_ROM_FILE_GUID,     0, 0, 0x02, 0, IGD_VID, IGD_DID_II, 0},  // Desktop/Mobile IGD
  { ONBOARD_VIDEO_OPTION_ROM_FILE_GUID,     0, 0, 0x02, 0, IGD_VID, IGD_DID_QS, 0},
  { NULL_ROM_FILE_GUID,                     0, 0, 0, 0, 0xffff, 0xffff, 0}
};

UINTN mSizeOptionRomTable =  sizeof(mPciOptionRomTable)/sizeof(PCI_OPTION_ROM_TABLE);


