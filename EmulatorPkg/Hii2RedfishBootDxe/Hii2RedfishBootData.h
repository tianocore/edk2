/** @file
  The header file of HII-to-Redfish boot driver.

  (C) Copyright 2022 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_2_REDFISH_BOOT_DATA_H_
#define HII_2_REDFISH_BOOT_DATA_H_

#include <Uefi/UefiMultiPhase.h>
#include <Guid/HiiPlatformSetupFormset.h>

#define HII_2_REDFISH_BOOT_FORMSET_GUID \
  { \
    0x8399a787, 0x108e, 0x4e53, { 0x9e, 0xde, 0x4b, 0x18, 0xcc, 0x9e, 0xab, 0x3b } \
  }

extern EFI_GUID  gHii2RedfishBootFormsetGuid;

#define FORM_ID                                   0x001
#define QUESTION_ID_BOOT_SOURCE_OVERRIDE_ENABLED  0x100
#define QUESTION_ID_BOOT_SOURCE_OVERRIDE_MODE     0x101
#define QUESTION_ID_BOOT_SOURCE_OVERRIDE_TARGET   0x102
#define LABEL_BOOT_OPTION                         0x200
#define LABEL_BOOT_OPTION_END                     0x201
#define BOOT_ORDER_LIST                           0x300
#define MAX_BOOT_OPTIONS                          100
#define BOOT_OPTION_VAR_STORE_ID                  0x800
//
// VarOffset that will be used to create question
// all these values are computed from the structure
// defined below
//
#define VAR_OFFSET(Field)  ((UINT16) ((UINTN) &(((HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA *) 0)->Field)))

#pragma pack(1)

//
// Definiton of HII_2_REDFISH_BOOT_EFI_VARSTORE_DATA
//
typedef struct {
  UINT8    BootSourceOverrideEnabled;
  UINT8    BootSourceOverrideMode;
  UINT8    BootSourceOverrideTarget;
  UINT8    Reversed;
} HII_2_REDFISH_BOOT_EFI_VARSTORE_DATA;

//
// Definiton of HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA
//
typedef struct {
  UINT32    BootOptionOrder[MAX_BOOT_OPTIONS];
} HII_2_REDFISH_BOOT_OPTION_VARSTORE_DATA;

#pragma pack()

#endif
