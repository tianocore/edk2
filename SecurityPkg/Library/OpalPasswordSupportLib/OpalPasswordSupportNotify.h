/** @file
  Implementation of Opal password support library.

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DXE_OPAL_NOTIFY_H_
#define _DXE_OPAL_NOTIFY_H_

#include <PiDxe.h>
#include <PiSmm.h>

#include <Guid/PiSmmCommunicationRegionTable.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/OpalPasswordSupportLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmBase2.h>


#pragma pack(1)

typedef struct {
  UINTN       Function;
  EFI_STATUS  ReturnStatus;
  UINT8       Data[1];
} OPAL_SMM_COMMUNICATE_HEADER;

typedef struct {
  UINT8                      Password[32];
  UINT8                      PasswordLength;

  EFI_DEVICE_PATH_PROTOCOL   OpalDevicePath;
} OPAL_COMM_DEVICE_LIST;

#pragma pack()

#define SMM_FUNCTION_SET_OPAL_PASSWORD        1

#define OPAL_PASSWORD_NOTIFY_PROTOCOL_GUID {0x0ff2ddd0, 0xefc9, 0x4f49, { 0x99, 0x7a, 0xcb, 0x59, 0x44, 0xe6, 0x97, 0xd3 } }

#endif
