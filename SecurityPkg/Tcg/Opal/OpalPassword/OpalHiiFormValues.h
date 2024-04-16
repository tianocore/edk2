/** @file
  Defines Opal HII form ids, structures and values.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_HII_FORM_VALUES_H_
#define _OPAL_HII_FORM_VALUES_H_

// ID's for various forms that will be used by HII
#define FORMID_VALUE_MAIN_MENU            0x01
#define FORMID_VALUE_DISK_INFO_FORM_MAIN  0x02

#pragma pack(1)
typedef struct {
  UINT16    Lock           : 1;
  UINT16    Unlock         : 1;
  UINT16    SetAdminPwd    : 1;
  UINT16    SetUserPwd     : 1;
  UINT16    SecureErase    : 1;
  UINT16    Revert         : 1;
  UINT16    PsidRevert     : 1;
  UINT16    DisableUser    : 1;
  UINT16    DisableFeature : 1;
  UINT16    EnableFeature  : 1;
  UINT16    Reserved       : 5;
  UINT16    KeepUserData   : 1;
} OPAL_REQUEST;

typedef struct {
  UINT8           NumDisks;
  UINT8           SelectedDiskIndex;
  UINT16          SelectedDiskAvailableActions;
  UINT16          SupportedDisks;
  BOOLEAN         KeepUserDataForced;
  OPAL_REQUEST    OpalRequest;
  UINT8           EnableBlockSid;
} OPAL_HII_CONFIGURATION;

#pragma pack()

/* Action Flags */
#define HII_ACTION_NONE             0x0000
#define HII_ACTION_LOCK             0x0001
#define HII_ACTION_UNLOCK           0x0002
#define HII_ACTION_SET_ADMIN_PWD    0x0004
#define HII_ACTION_SET_USER_PWD     0x0008
#define HII_ACTION_SECURE_ERASE     0x0010
#define HII_ACTION_REVERT           0x0020
#define HII_ACTION_PSID_REVERT      0x0040
#define HII_ACTION_DISABLE_USER     0x0080
#define HII_ACTION_DISABLE_FEATURE  0x0100
#define HII_ACTION_ENABLE_FEATURE   0x0200

/* Number of bits allocated for each part of a unique key for an HII_ITEM
 * all bits together must be <= 16 (EFI_QUESTION_ID is UINT16)
 * 1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1
 * |   |-----------------------|   |---------------------------|
 * FLG INDEX                       ID
 */
#define HII_KEY_ID_BITS     8
#define HII_KEY_INDEX_BITS  7
#define HII_KEY_FLAG_BITS   1

#define HII_KEY_FLAG  0x8000                                       // bit 15 (zero based)

/***********/
/* Key IDs */
/***********/

#define HII_KEY_ID_GOTO_DISK_INFO  1

#define HII_KEY_ID_VAR_SUPPORTED_DISKS                  2
#define HII_KEY_ID_VAR_SELECTED_DISK_AVAILABLE_ACTIONS  3

#define HII_KEY_ID_BLOCKSID        4
#define HII_KEY_ID_SET_ADMIN_PWD   5
#define HII_KEY_ID_SET_USER_PWD    6
#define HII_KEY_ID_SECURE_ERASE    7
#define HII_KEY_ID_REVERT          8
#define HII_KEY_ID_KEEP_USER_DATA  9
#define HII_KEY_ID_PSID_REVERT     0xA
#define HII_KEY_ID_DISABLE_USER    0xB
#define HII_KEY_ID_ENABLE_FEATURE  0xC

#define HII_KEY_ID_MAX  0xC                                 // !!Update each time a new ID is added!!

#define HII_KEY_WITH_INDEX(id, index) \
    ( \
        HII_KEY_FLAG | \
        (id) | \
        ((index) << HII_KEY_ID_BITS) \
    )

#define HII_KEY(id)  HII_KEY_WITH_INDEX(id, 0)

/* Label */
#define OPAL_MAIN_MENU_LABEL_START  0x6100
#define OPAL_MAIN_MENU_LABEL_END    0x6101
#define OPAL_DISK_INFO_LABEL_START  0x6200
#define OPAL_DISK_INFO_LABEL_END    0x6201

#define PACKAGE_LIST_GUID  { 0xf0308176, 0x9058, 0x4153, { 0x93, 0x3d, 0xda, 0x2f, 0xdc, 0xc8, 0x3e, 0x44 } }

/* {410483CF-F4F9-4ece-848A-1958FD31CEB7} */
#define SETUP_FORMSET_GUID  { 0x410483cf, 0xf4f9, 0x4ece, { 0x84, 0x8a, 0x19, 0x58, 0xfd, 0x31, 0xce, 0xb7 } }

// {BBF1ACD2-28D8-44ea-A291-58A237FEDF1A}
#define SETUP_VARIABLE_GUID  { 0xbbf1acd2, 0x28d8, 0x44ea, { 0xa2, 0x91, 0x58, 0xa2, 0x37, 0xfe, 0xdf, 0x1a } }

#endif //_HII_FORM_VALUES_H_
