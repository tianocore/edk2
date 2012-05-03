/** @file
  The form data for user profile manager driver.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __USER_PROFILE_MANAGER_DATA_H__
#define __USER_PROFILE_MANAGER_DATA_H__

#include <Guid/UserProfileManagerHii.h>

//
// Form ID
//
#define FORMID_USER_MANAGE          0x0001
#define FORMID_MODIFY_USER          0x0002
#define FORMID_DEL_USER             0x0003
#define FORMID_USER_INFO            0x0004
#define FORMID_MODIFY_IP            0x0005
#define FORMID_MODIFY_AP            0x0006
#define FORMID_LOAD_DP              0x0007
#define FORMID_CONNECT_DP           0x0008
#define FORMID_PERMIT_LOAD_DP       0x0009
#define FORMID_FORBID_LOAD_DP       0x000A
#define FORMID_PERMIT_CONNECT_DP    0x000B
#define FORMID_FORBID_CONNECT_DP    0x000C

//
// Label ID
//
#define  LABEL_USER_MANAGE_FUNC     0x0010
#define  LABEL_USER_DEL_FUNC        0x0020
#define  LABEL_USER_MOD_FUNC        0x0030
#define  LABEL_USER_INFO_FUNC       0x0040
#define  LABEL_IP_MOD_FUNC          0x0050
#define  LABEL_AP_MOD_FUNC          0x0060
#define  LABEL_PERMIT_LOAD_FUNC     0x0070
#define  LABLE_FORBID_LOAD_FUNC     0x0080
#define  LABEL_END                  0x00F0

//
// First form key (Add/modify/del user profile). 
// First 2 bits (bit 16~15).
//
#define  KEY_MODIFY_USER            0x4000
#define  KEY_DEL_USER               0x8000
#define  KEY_ADD_USER               0xC000
#define  KEY_FIRST_FORM_MASK        0xC000

//
// Second form key (Display new form /Select user / modify device path in access policy).
// Next 2 bits (bit 14~13).
//
#define  KEY_ENTER_NEXT_FORM        0x0000
#define  KEY_SELECT_USER            0x1000
#define  KEY_MODIFY_AP_DP           0x2000
#define  KEY_OPEN_CLOSE_FORM_ACTION 0x3000
#define  KEY_SECOND_FORM_MASK       0x3000

//
// User profile information form key.
// Next 3 bits (bit 12~10).
//
#define  KEY_MODIFY_NAME            0x0200
#define  KEY_MODIFY_IP              0x0400
#define  KEY_MODIFY_AP              0x0600
#define  KEY_MODIFY_INFO_MASK       0x0E00

//
// Specified key, used in VFR (KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_NAME).
//
#define  KEY_MODIFY_USER_NAME       0x5200 

//
// Modify identity policy form key.
// Next 3 bits (bit 9~7).
//
#define  KEY_MODIFY_PROV            0x0040
#define  KEY_MODIFY_MTYPE           0x0080
#define  KEY_MODIFY_CONN            0x00C0
#define  KEY_ADD_IP_OP              0x0100
#define  KEY_IP_RETURN_UIF          0x0140
#define  KEY_MODIFY_IP_MASK         0x01C0

//
// Specified key.
//
#define  KEY_ADD_LOGICAL_OP         0x5500
#define  KEY_IP_RETURN              0x5540

//
// Modify access policy form key.
// Next 3 bits (bit 9~7).
//
#define  KEY_MODIFY_RIGHT           0x0040
#define  KEY_MODIFY_SETUP           0x0080
#define  KEY_MODIFY_BOOT            0x00C0
#define  KEY_MODIFY_LOAD            0x0100
#define  KEY_MODIFY_CONNECT         0x0140
#define  KEY_AP_RETURN_UIF          0x0180
#define  KEY_MODIFY_AP_MASK         0x01C0

//
// Specified key.
//
#define  KEY_LOAD_DP                0x5700
#define  KEY_CONN_DP                0x5740
#define  KEY_AP_RETURN              0x5780

//
// Device path form key.
// Next 2 bits (bit 6~5).
//
#define  KEY_PERMIT_MODIFY          0x0010
#define  KEY_FORBID_MODIFY          0x0020
#define  KEY_DISPLAY_DP_MASK        0x0030

//
// Specified key.
//
#define  KEY_LOAD_PERMIT            0x5710
#define  KEY_LOAD_FORBID            0x5720
#define  KEY_CONNECT_PERMIT         0x5750
#define  KEY_CONNECT_FORBID         0x5760

//
// Device path modify key.
// 2 bits (bit 12~11).
// 
#define KEY_LOAD_PERMIT_MODIFY      0x0000
#define KEY_LOAD_FORBID_MODIFY      0x0400
#define KEY_CONNECT_PERMIT_MODIFY   0x0800
#define KEY_CONNECT_FORBID_MODIFY   0x0C00
#define KEY_MODIFY_DP_MASK          0x0C00


//
// The permissions usable when configuring the platform.
//
#define  ACCESS_SETUP_RESTRICTED   1
#define  ACCESS_SETUP_NORMAL       2
#define  ACCESS_SETUP_ADMIN        3

//
// Question ID for the question used in each form (KEY_OPEN_CLOSE_FORM_ACTION | FORMID_FORM_USER_MANAGE)
// This ID is used in FORM OPEN/CLOSE CallBack action.
//
#define QUESTIONID_USER_MANAGE      0x3001

#endif
