/** @file
  Defines Opal HII form ids, structures and values.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef _OPAL_HII_FORM_VALUES_H_
#define _OPAL_HII_FORM_VALUES_H_

// Maximum Opal password Length
#define MAX_PASSWORD_CHARACTER_LENGTH                      0x14

// PSID Length
#define PSID_CHARACTER_LENGTH                              0x20
#define PSID_CHARACTER_STRING_END_LENGTH                   0x21

// ID's for various forms that will be used by HII
#define FORMID_VALUE_MAIN_MENU                             0x01
#define FORMID_VALUE_DISK_INFO_FORM_MAIN                   0x02
#define FORMID_VALUE_DISK_ACTION_FORM                      0x03

// Structure defining the OPAL_HII_CONFIGURATION
#pragma pack(1)
typedef struct {
    UINT8   NumDisks;
    UINT8   SelectedDiskIndex;
    UINT8   SelectedAction;
    UINT16  SelectedDiskAvailableActions;
    UINT16  SupportedDisks;
    UINT8   KeepUserData;
    UINT16  AvailableFields;
    UINT16  Password[MAX_PASSWORD_CHARACTER_LENGTH];
    UINT16  Psid[PSID_CHARACTER_STRING_END_LENGTH];
    UINT8   EnableBlockSid;
} OPAL_HII_CONFIGURATION;
#pragma pack()

/* Action Flags */
#define HII_ACTION_NONE                                        0x0000
#define HII_ACTION_LOCK                                        0x0001
#define HII_ACTION_UNLOCK                                      0x0002
#define HII_ACTION_SET_ADMIN_PWD                               0x0004
#define HII_ACTION_SET_USER_PWD                                0x0008
#define HII_ACTION_SECURE_ERASE                                0x0010
#define HII_ACTION_PSID_REVERT                                 0x0020
#define HII_ACTION_DISABLE_USER                                0x0040
#define HII_ACTION_REVERT                                      0x0080
#define HII_ACTION_DISABLE_FEATURE                             0x0100
#define HII_ACTION_ENABLE_FEATURE                              0x0200

/* Flags for diskActionAvailableFields */
#define HII_FIELD_PASSWORD                      0x0001
#define HII_FIELD_PSID                          0x0002
#define HII_FIELD_KEEP_USER_DATA                0x0004
#define HII_FIELD_KEEP_USER_DATA_FORCED         0x0008

/* Number of bits allocated for each part of a unique key for an HII_ITEM
 * all bits together must be <= 16 (EFI_QUESTION_ID is UINT16)
 * 1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1
 * |   |-----------------------|   |---------------------------|
 * FLG INDEX                       ID
 */
#define HII_KEY_ID_BITS                                     8
#define HII_KEY_INDEX_BITS                                  7
#define HII_KEY_FLAG_BITS                                   1

#define HII_KEY_FLAG                                        0x8000 // bit 15 (zero based)

/***********/
/* Key IDs */
/***********/

#define HII_KEY_ID_GOTO_MAIN_MENU                       0
#define HII_KEY_ID_GOTO_DISK_INFO                       1
#define HII_KEY_ID_GOTO_LOCK                            2
#define HII_KEY_ID_GOTO_UNLOCK                          3
#define HII_KEY_ID_GOTO_SET_ADMIN_PWD                   4
#define HII_KEY_ID_GOTO_SET_USER_PWD                    5
#define HII_KEY_ID_GOTO_SECURE_ERASE                    6
#define HII_KEY_ID_GOTO_PSID_REVERT                     7
#define HII_KEY_ID_GOTO_REVERT                          8
#define HII_KEY_ID_GOTO_DISABLE_USER                    9
#define HII_KEY_ID_GOTO_ENABLE_FEATURE                  0xA //10
#define HII_KEY_ID_GOTO_CONFIRM_TO_MAIN_MENU            0xB //11
#define HII_KEY_ID_ENTER_PASSWORD                       0xC //12
#define HII_KEY_ID_ENTER_PSID                           0xD //13
#define HII_KEY_ID_VAR_SUPPORTED_DISKS                  0xE //14
#define HII_KEY_ID_VAR_SELECTED_DISK_AVAILABLE_ACTIONS  0xF //15

#define HII_KEY_ID_BLOCKSID                             0x17 //23
#define HII_KEY_ID_MAX                                  0x17 //23 // !!Update each time a new ID is added!!

#define HII_KEY_WITH_INDEX(id, index) \
    ( \
        HII_KEY_FLAG | \
        (id) | \
        ((index) << HII_KEY_ID_BITS) \
    )

#define HII_KEY(id) HII_KEY_WITH_INDEX(id, 0)

#define PACKAGE_LIST_GUID { 0xf0308176, 0x9058, 0x4153, { 0x93, 0x3d, 0xda, 0x2f, 0xdc, 0xc8, 0x3e, 0x44 } }

/* {410483CF-F4F9-4ece-848A-1958FD31CEB7} */
#define SETUP_FORMSET_GUID { 0x410483cf, 0xf4f9, 0x4ece, { 0x84, 0x8a, 0x19, 0x58, 0xfd, 0x31, 0xce, 0xb7 } }

// {BBF1ACD2-28D8-44ea-A291-58A237FEDF1A}
#define SETUP_VARIABLE_GUID { 0xbbf1acd2, 0x28d8, 0x44ea, { 0xa2, 0x91, 0x58, 0xa2, 0x37, 0xfe, 0xdf, 0x1a } }

#endif //_HII_FORM_VALUES_H_

