/** @file
Header file for Platform Initialization Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SETUP_PLATFORM_H
#define _SETUP_PLATFORM_H

//
// Data
//
#define PLATFORM_NUM_SMBUS_RSVD_ADDRESSES 4
#define VAR_OFFSET(Field)     ((UINT16) ((UINTN) &(((SYSTEM_CONFIGURATION *) 0)->Field)))
#define QUESTION_ID(Field)    (VAR_OFFSET (Field) + 1)

#define SMBUS_ADDR_CH_A_1     0xA0
#define SMBUS_ADDR_CK505      0xD2
#define SMBUS_ADDR_THERMAL_SENSOR1 0x4C
#define SMBUS_ADDR_THERMAL_SENSOR2 0x4D

///
/// HII specific Vendor Device Path Node definition.
///
#pragma pack(1)

typedef struct {
  VENDOR_DEVICE_PATH             VendorDevicePath;
  UINT16                         UniqueId;
} HII_VENDOR_DEVICE_PATH_NODE;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  HII_VENDOR_DEVICE_PATH_NODE    Node;
  EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

//
// Prototypes
//
VOID
ProducePlatformCpuData (
  VOID
  );

VOID
PlatformInitQNCRegs (
  VOID
  );

EFI_STATUS
InitKeyboardLayout (
  VOID
  );

//
// Global externs
//
extern UINT8 UefiSetupDxeStrings[];

extern EFI_HII_DATABASE_PROTOCOL        *mHiiDataBase;
extern EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting;

#endif
