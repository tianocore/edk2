/** @file
  Header file for NV data structure definition.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TREE_CONFIG_NV_DATA_H__
#define __TREE_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/TrEEPhysicalPresenceData.h>
#include <Guid/TrEEConfigHii.h>

#define TREE_CONFIGURATION_VARSTORE_ID  0x0001
#define TREE_CONFIGURATION_FORM_ID      0x0001

#define KEY_TPM_DEVICE                                 0x2000
#define KEY_TPM2_OPERATION                             0x2001

#define TPM_DEVICE_NULL           0
#define TPM_DEVICE_1_2            1
#define TPM_DEVICE_2_0_DTPM       2
#define TPM_DEVICE_MIN            TPM_DEVICE_1_2
#define TPM_DEVICE_MAX            TPM_DEVICE_2_0_DTPM
#define TPM_DEVICE_DEFAULT        TPM_DEVICE_1_2

//
// Nv Data structure referenced by IFR, TPM device user desired
//
typedef struct {
  UINT8   TpmDevice;
} TREE_CONFIGURATION;

//
// Variable saved for S3, TPM detected, only valid in S3 path.
// This variable is ReadOnly.
//
typedef struct {
  UINT8   TpmDeviceDetected;
} TREE_DEVICE_DETECTION;

#define TREE_STORAGE_NAME  L"TREE_CONFIGURATION"
#define TREE_DEVICE_DETECTION_NAME  L"TREE_DEVICE_DETECTION"

#define TPM_INSTANCE_ID_LIST  { \
  {TPM_DEVICE_INTERFACE_NONE,           TPM_DEVICE_NULL},      \
  {TPM_DEVICE_INTERFACE_TPM12,          TPM_DEVICE_1_2},       \
  {TPM_DEVICE_INTERFACE_TPM20_DTPM,     TPM_DEVICE_2_0_DTPM},  \
}

//
// BUGBUG: In order to pass VfrCompiler, we have to redefine GUID here.
//
#ifndef __BASE_H__
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} GUID;
#endif

typedef struct {
  GUID       TpmInstanceGuid;
  UINT8      TpmDevice;
} TPM_INSTANCE_ID;

#endif
