/** @file
  The header file of HII-to-Redfish memory driver.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_2_REDFISH_MEMORY_DATA_H_
#define HII_2_REDFISH_MEMORY_DATA_H_

#include <Uefi/UefiMultiPhase.h>
#include <Guid/HiiPlatformSetupFormset.h>

#define HII_2_REDFISH_MEMORY_FORMSET_GUID \
  { \
    0xC2BE579E, 0x3C57, 0x499C, { 0xA9, 0xDF, 0xE6, 0x23, 0x8A, 0x49, 0x64, 0xF8 } \
  }

extern EFI_GUID  gHii2RedfishMemoryFormsetGuid;

#define FORM_ID           0x001
#define FROM_ID_MEMORY_1  0x002
#define FROM_ID_MEMORY_2  0x003
#define FROM_ID_MEMORY_3  0x004
#define FROM_ID_MEMORY_4  0x005

#define QUESTION_ID_MEMORY_1_BASE_MODULE_TYPE      0x100
#define QUESTION_ID_MEMORY_1_BUS_WIDTH_BITS        0x101
#define QUESTION_ID_MEMORY_1_CONFIGURATION_LOCKED  0x102

#define QUESTION_ID_MEMORY_2_BASE_MODULE_TYPE      0x200
#define QUESTION_ID_MEMORY_2_BUS_WIDTH_BITS        0x201
#define QUESTION_ID_MEMORY_2_CONFIGURATION_LOCKED  0x202

#define QUESTION_ID_MEMORY_3_BASE_MODULE_TYPE      0x300
#define QUESTION_ID_MEMORY_3_BUS_WIDTH_BITS        0x301
#define QUESTION_ID_MEMORY_3_CONFIGURATION_LOCKED  0x302

#define QUESTION_ID_MEMORY_4_BASE_MODULE_TYPE      0x400
#define QUESTION_ID_MEMORY_4_BUS_WIDTH_BITS        0x401
#define QUESTION_ID_MEMORY_4_CONFIGURATION_LOCKED  0x402

#define MEMORY_MAX_NO                  0x04
#define ID_STRING_MIN                  0
#define ID_STRING_MAX                  15
#define ID_STRING_MAX_WITH_TERMINATOR  16

#pragma pack(1)
//
// Definiton of HII_2_REDFISH_MEMORY_SET
//
typedef struct {
  CHAR16    ModuleProductId[ID_STRING_MAX_WITH_TERMINATOR];
  UINT8     BaseModuleType;
  UINT8     BusWidthBits;
  UINT8     ConfigurationLocked;
  UINT8     Reserved;               // for 16 bit boundary of ModuleProductId
} HII_2_REDFISH_MEMORY_SET;

//
// Definiton of HII_2_REDFISH_MEMORY_EFI_VARSTORE_DATA
//
typedef struct {
  HII_2_REDFISH_MEMORY_SET    Memory[MEMORY_MAX_NO];
} HII_2_REDFISH_MEMORY_EFI_VARSTORE_DATA;

#pragma pack()

#endif
