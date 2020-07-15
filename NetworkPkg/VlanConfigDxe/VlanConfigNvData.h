/** @file
  Header file for NV data structure definition.

Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VLAN_CONFIG_NV_DATA_H__
#define __VLAN_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/VlanConfigHii.h>

#define VLAN_CONFIGURATION_VARSTORE_ID  0x0001
#define VLAN_CONFIGURATION_FORM_ID      0x0001
#define VLAN_HEAD_FORM_ID               0x0002

#define VLAN_ADD_QUESTION_ID            0x1000
#define VLAN_REMOVE_QUESTION_ID         0x2000
#define VLAN_UPDATE_QUESTION_ID         0x3000

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
