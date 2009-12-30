/** @file
  Header file for NV data structure definition.

Copyright (c) 2009, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The full
text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VLAN_CONFIG_NV_DATA_H__
#define __VLAN_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>


#define VLAN_CONFIG_PRIVATE_GUID \
  { \
    0xd79df6b0, 0xef44, 0x43bd, {0x97, 0x97, 0x43, 0xe9, 0x3b, 0xcf, 0x5f, 0xa8 } \
  }

#define VLAN_CONFIGURATION_VARSTORE_ID  0x0001
#define VLAN_CONFIGURATION_FORM_ID      0x0001

#define VLAN_ADD_QUESTION_ID            0x1000
#define VLAN_REMOVE_QUESTION_ID         0x2000

#define LABEL_VLAN_LIST                 0x0001
#define LABEL_END                       0xffff

//
// The maximum number of VLAN that will be displayed on the menu
//
#define MAX_VLAN_NUMBER                 100

//
// Nv Data structure referenced by IFR
//
typedef struct {
  UINT16  VlanId;
  UINT8   Priority;
  UINT8   VlanList[MAX_VLAN_NUMBER];
} VLAN_CONFIGURATION;

#endif
