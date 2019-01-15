/** @file
  HDD Password common header file.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HDD_PASSWORD_COMMON_H_
#define _HDD_PASSWORD_COMMON_H_

//
// The payload length of HDD related ATA commands
//
#define HDD_PAYLOAD                     512

#define ATA_SECURITY_SET_PASSWORD_CMD   0xF1
#define ATA_SECURITY_UNLOCK_CMD         0xF2
#define ATA_SECURITY_FREEZE_LOCK_CMD    0xF5
#define ATA_SECURITY_DIS_PASSWORD_CMD   0xF6

//
// The max retry count specified in ATA 8 spec.
//
#define MAX_HDD_PASSWORD_RETRY_COUNT    5

//
// According to ATA spec, the max length of hdd password is 32 bytes
//
#define HDD_PASSWORD_MAX_LENGTH         32

#define HDD_PASSWORD_DEVICE_INFO_GUID { 0x96d877ad, 0x48af, 0x4b39, { 0x9b, 0x27, 0x4d, 0x97, 0x43, 0x9, 0xae, 0x47 } }

typedef struct {
  UINT8             Bus;
  UINT8             Device;
  UINT8             Function;
  UINT8             Reserved;
  UINT16            Port;
  UINT16            PortMultiplierPort;
} HDD_PASSWORD_DEVICE;

//
// It will be used to unlock HDD password for S3.
//
typedef struct {
  HDD_PASSWORD_DEVICE         Device;
  CHAR8                       Password[HDD_PASSWORD_MAX_LENGTH];
  UINT32                      DevicePathLength;
  EFI_DEVICE_PATH_PROTOCOL    DevicePath[];
} HDD_PASSWORD_DEVICE_INFO;

#endif // _HDD_PASSWORD_COMMON_H_
