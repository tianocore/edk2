/** @file
  The platform device manager reference implement

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DEVICE_MANAGER_VFR_H_
#define _DEVICE_MANAGER_VFR_H_

#include <Guid/BdsHii.h>

#define LABEL_DEVICES_LIST                   0x1100
#define LABEL_NETWORK_DEVICE_LIST_ID         0x1101
#define LABEL_NETWORK_DEVICE_ID              0x1102
#define LABEL_END                            0xffff
#define LABEL_FORM_ID_OFFSET                 0x0100

#define LABEL_DRIVER_HEALTH                  0x2000
#define LABEL_DRIVER_HEALTH_END              0x2001

#define LABEL_DRIVER_HEALTH_REAPIR_ALL       0x3000
#define LABEL_DRIVER_HEALTH_REAPIR_ALL_END   0x3001

#define LABEL_VBIOS                          0x0040

#define INVALID_FORM_ID                      0x0FFF
#define DEVICE_MANAGER_FORM_ID               0x1000
#define NETWORK_DEVICE_LIST_FORM_ID          0x1001
#define NETWORK_DEVICE_FORM_ID               0x1002
#define DRIVER_HEALTH_FORM_ID                0x1003
#define DEVICE_KEY_OFFSET                    0x4000
#define NETWORK_DEVICE_LIST_KEY_OFFSET       0x2000
#define DEVICE_MANAGER_KEY_VBIOS             0x3000
#define MAX_KEY_SECTION_LEN                  0x1000

#define DEVICE_MANAGER_KEY_DRIVER_HEALTH     0x1111
#define DRIVER_HEALTH_KEY_OFFSET             0x2000
#define DRIVER_HEALTH_REPAIR_ALL_KEY         0x3000
#define DRIVER_HEALTH_RETURN_KEY             0x4000

#define QUESTION_NETWORK_DEVICE_ID           0x3FFF
//
// These are the VFR compiler generated data representing our VFR data.
//
extern UINT8  DeviceManagerVfrBin[];
extern UINT8  DriverHealthVfrBin[];

#endif
