/** @file
  Header file for NV data structure definition.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __TCG2_CONFIG_NV_DATA_H__
#define __TCG2_CONFIG_NV_DATA_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/Tcg2ConfigHii.h>
#include <IndustryStandard/TcgPhysicalPresence.h>

//
// BUGBUG: In order to pass VfrCompiler, we have to redefine below MACRO, which already in <Protocol/Tcg2Protocol.h>.
//
#ifndef __TCG2_H__
#define EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2       0x00000001
#define EFI_TCG2_EVENT_LOG_FORMAT_TCG_2         0x00000002
#endif
#define EFI_TCG2_EVENT_LOG_FORMAT_ALL           (EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2 | EFI_TCG2_EVENT_LOG_FORMAT_TCG_2)

#define TCG2_CONFIGURATION_VARSTORE_ID  0x0001
#define TCG2_CONFIGURATION_FORM_ID      0x0001

#define KEY_TPM_DEVICE                                 0x2000
#define KEY_TPM2_OPERATION                             0x2001
#define KEY_TPM2_OPERATION_PARAMETER            0x2002
#define KEY_TPM2_PCR_BANKS_REQUEST_0            0x2003
#define KEY_TPM2_PCR_BANKS_REQUEST_1            0x2004
#define KEY_TPM2_PCR_BANKS_REQUEST_2            0x2005
#define KEY_TPM2_PCR_BANKS_REQUEST_3            0x2006
#define KEY_TPM2_PCR_BANKS_REQUEST_4            0x2007

#define TPM_DEVICE_NULL           0
#define TPM_DEVICE_1_2            1
#define TPM_DEVICE_2_0_DTPM       2
#define TPM_DEVICE_MIN            TPM_DEVICE_1_2
#define TPM_DEVICE_MAX            TPM_DEVICE_2_0_DTPM
#define TPM_DEVICE_DEFAULT        TPM_DEVICE_1_2

#define TCG2_PROTOCOL_VERSION_DEFAULT        0x0001
#define EFI_TCG2_EVENT_LOG_FORMAT_DEFAULT    EFI_TCG2_EVENT_LOG_FORMAT_TCG_1_2

//
// Nv Data structure referenced by IFR, TPM device user desired
//
typedef struct {
  UINT8   TpmDevice;
} TCG2_CONFIGURATION;

//
// Variable saved for S3, TPM detected, only valid in S3 path.
// This variable is ReadOnly.
//
typedef struct {
  UINT8   TpmDeviceDetected;
} TCG2_DEVICE_DETECTION;

#define TCG2_STORAGE_NAME  L"TCG2_CONFIGURATION"
#define TCG2_DEVICE_DETECTION_NAME  L"TCG2_DEVICE_DETECTION"

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
