/** @file
  The header file of HII-to-Redfish BIOS example driver.

  (C) Copyright 2022 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_2_REDFISH_BIOS_DATA_H_
#define HII_2_REDFISH_BIOS_DATA_H_

#include <Uefi/UefiMultiPhase.h>
#include <Guid/HiiPlatformSetupFormset.h>

#define HII_2_REDFISH_BIOS_FORMSET_GUID \
  { \
    0xC2724AD1, 0x4049, 0x2404, { 0xF8, 0xCE, 0x01, 0xA7, 0x9C, 0xEC, 0x16, 0xF6 } \
    }

extern EFI_GUID  gHii2RedfishBiosFormsetGuid;

#define FORM_ID                0x001
#define FROM_ID_BIOS_OPTION_1  0x002
#define FROM_ID_BIOS_OPTION_2  0x003
#define FROM_ID_BIOS_OPTION_3  0x004
#define FROM_ID_BIOS_OPTION_4  0x005

#define ID_STRING_MIN                  0
#define ID_STRING_MAX                  15
#define ID_STRING_MAX_WITH_TERMINATOR  16

#pragma pack()

//
// Definiton of HII_2_REDFISH_BIOS_VARSTORE_DATA
//
typedef struct {
  UINT8     RefishBiosOption1Data;
  CHAR16    RefishBiosOption2Data[ID_STRING_MAX_WITH_TERMINATOR];
  UINT8     RefishBiosOption3Data;
  UINT8     RefishBiosOption4Data;
  UINT8     Reserved;
} HII_2_REDFISH_BIOS_EFI_VARSTORE_DATA;

#endif
